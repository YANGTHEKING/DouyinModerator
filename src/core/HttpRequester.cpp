#include "HttpRequester.h"
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

HttpRequester::HttpRequester(QObject* parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

void HttpRequester::fetchTtwid(const QString& liveId) {
    QUrl url(QString("https://live.douyin.com/%1").arg(liveId));
    qDebug() << "[HTTP] fetchTtwid URL:" << url.toString();
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    req.setRawHeader("cookie", "__ac_nonce=0638a7b21a08e7d9a3");

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[HTTP] fetchTtwid ERROR:" << reply->errorString();
            emit error("获取ttwid失败: " + reply->errorString());
            return;
        }
        qDebug() << "[HTTP] fetchTtwid OK, status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Extract ttwid from Set-Cookie header
        auto cookies = reply->header(QNetworkRequest::SetCookieHeader).value<QList<QNetworkCookie>>();
        for (const auto& c : cookies) {
            if (c.name() == "ttwid") {
                emit ttwidReady(QString::fromUtf8(c.value()));
                return;
            }
        }
        // Fallback: parse from raw header
        for (const auto& sc : reply->rawHeaderList()) {
            if (!sc.contains("Set-Cookie")) continue;
            QString header = QString::fromUtf8(sc);
            QRegularExpression re("ttwid=([^;]+)");
            auto m = re.match(header);
            if (m.hasMatch()) {
                emit ttwidReady(m.captured(1));
                return;
            }
        }
        emit error("无法获取ttwid cookie");
    });
}

void HttpRequester::fetchRoomInfo(const QString& liveId, const QString& ttwid) {
    // 优先使用抖音 API 接口获取房间信息，比解析 HTML 更可靠
    QUrl url(QString("https://live.douyin.com/webcast/room/web/enter/?aid=6383&web_rid=%1&device_platform=web&browser_language=zh-CN&browser_platform=MacIntel&browser_name=Chrome&browser_version=120.0.0.0").arg(liveId));
    qDebug() << "[HTTP] fetchRoomInfo API URL:" << url.toString();
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    QString cookie = QString("ttwid=%1; __ac_nonce=0638a7b21a08e7d9a3").arg(ttwid);
    req.setRawHeader("cookie", cookie.toUtf8());
    req.setRawHeader("referer", QString("https://live.douyin.com/%1").arg(liveId).toUtf8());

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, liveId, ttwid]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[HTTP] fetchRoomInfo ERROR:" << reply->errorString();
            emit error("获取房间信息失败: " + reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        qDebug() << "[HTTP] fetchRoomInfo response size:" << data.size() << "first 200 chars:" << QString::fromUtf8(data.left(200));
        auto doc = QJsonDocument::fromJson(data);
        auto root = doc.object();

        RoomInfo info;
        info.liveId = liveId;
        info.isStreaming = true;

        // 解析 API JSON 返回
        // 实际结构: {"data":{"data":[{"id_str":"...","title":"...","room":{...},"owner":{...}}]}}
        if (root.contains("data")) {
            auto dataObj = root["data"].toObject();

            // data.data 是数组，取第一个元素
            QJsonObject roomObj;
            if (dataObj.contains("data") && dataObj["data"].isArray()) {
                auto arr = dataObj["data"].toArray();
                if (!arr.isEmpty()) {
                    roomObj = arr[0].toObject();
                }
            }

            // 也检查 data.room (备用路径)
            if (roomObj.isEmpty() && dataObj.contains("room")) {
                roomObj = dataObj["room"].toObject();
            }

            // 获取 roomId
            if (roomObj.contains("id_str")) {
                info.roomId = roomObj["id_str"].toString();
            } else if (roomObj.contains("id")) {
                info.roomId = QString::number(roomObj["id"].toVariant().toLongLong());
            }

            // 获取标题
            if (roomObj.contains("title")) {
                info.title = roomObj["title"].toString();
            }

            // 获取主播信息 - owner 或 anchor 字段
            if (roomObj.contains("owner")) {
                auto ownerObj = roomObj["owner"].toObject();
                if (ownerObj.contains("nickname")) {
                    info.streamerName = ownerObj["nickname"].toString();
                }
            } else if (roomObj.contains("anchor")) {
                auto anchorObj = roomObj["anchor"].toObject();
                if (anchorObj.contains("nickname")) {
                    info.streamerName = anchorObj["nickname"].toString();
                }
            }

            // 顶层 data 对象直接取 room_id / enter_room_id
            if (dataObj.contains("room_id")) {
                info.roomId = QString::number(dataObj["room_id"].toVariant().toLongLong());
            }
            if (dataObj.contains("enter_room_id")) {
                info.userUniqueId = QString::number(dataObj["enter_room_id"].toVariant().toLongLong());
            }

            qDebug() << "[HTTP] Parsed roomId:" << info.roomId << "title:" << info.title << "streamer:" << info.streamerName;
        }

        // 如果 API 接口失败，fallback 到解析 HTML
        if (info.roomId.isEmpty()) {
            qDebug() << "[HTTP] API returned no roomId, falling back to HTML parsing";
            fetchRoomInfoFromHtml(liveId, ttwid);
            return;
        }
        emit roomInfoReady(info);
    });
}

void HttpRequester::fetchRoomInfoFromHtml(const QString& liveId, const QString& ttwid) {
    QUrl url(QString("https://live.douyin.com/%1").arg(liveId));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    QString cookie = QString("ttwid=%1; __ac_nonce=0638a7b21a08e7d9a3").arg(ttwid);
    req.setRawHeader("cookie", cookie.toUtf8());

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, liveId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("获取房间信息失败: " + reply->errorString());
            return;
        }
        QString html = QString::fromUtf8(reply->readAll());
        RoomInfo info;
        info.liveId = liveId;
        info.isStreaming = true;

        // Extract roomId - 多种格式兼容
        static const QStringList roomIdPatterns = {
            "\"roomId\":\"(\\d+)\"",
            "\"room_id\":\"(\\d+)\"",
            "\"roomId\":(\\d+)",
            "\"room_id\":(\\d+)",
            "roomId[\"']?\\s*[:=]\\s*[\"']?(\\d+)",
            "room_id[\"']?\\s*[:=]\\s*[\"']?(\\d+)",
        };
        for (const auto& pat : roomIdPatterns) {
            QRegularExpression re(pat);
            auto m = re.match(html);
            if (m.hasMatch()) {
                info.roomId = m.captured(1);
                break;
            }
        }

        // Extract user_unique_id
        static const QStringList uidPatterns = {
            "\"user_unique_id\":\"(\\d+)\"",
            "\"user_unique_id\":(\\d+)",
            "user_unique_id[\"']?\\s*[:=]\\s*[\"']?(\\d+)",
        };
        for (const auto& pat : uidPatterns) {
            QRegularExpression re(pat);
            auto m = re.match(html);
            if (m.hasMatch()) {
                info.userUniqueId = m.captured(1);
                break;
            }
        }

        // Extract title
        QRegularExpression reTitle("\"title\":\"([^\"]+)\"");
        auto m3 = reTitle.match(html);
        if (m3.hasMatch()) {
            info.title = m3.captured(1);
        }

        // Extract nickname
        QRegularExpression reNick("\"nickname\":\"([^\"]+)\"");
        auto m4 = reNick.match(html);
        if (m4.hasMatch()) {
            info.streamerName = m4.captured(1);
        }

        if (info.roomId.isEmpty()) {
            emit error("无法解析房间ID，请确认直播间号是否正确");
            return;
        }
        emit roomInfoReady(info);
    });
}

void HttpRequester::fetchSignedWssUrl(const QString& roomId, const QString& userUniqueId,
                                       const QString& apiKey) {
    qDebug() << "[HTTP] fetchSignedWssUrl, roomId:" << roomId << "uid:" << userUniqueId;
    QUrl url(SIGN_API);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 使用默认测试 Key（来自 BarrageGrab 项目），用户也可在设置中自定义
    static const QString DEFAULT_API_KEY = "test-apikey-de9991ea-bf2b-454c-7982-adddfe0581ac-96c0642e-7d1d-87a7-08b2-eff81edae4d3";
    QString key = apiKey.isEmpty() ? DEFAULT_API_KEY : apiKey;

    QJsonObject body;
    body["ApiKey"] = key;
    body["RoomId"] = roomId;
    body["UserUniqueId"] = userUniqueId;
    body["BrowserName"] = "Chrome";
    body["BrowserVersion"] = "120.0.0.0";

    auto* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[HTTP] fetchSignedWssUrl ERROR:" << reply->errorString();
            emit error("签名API请求失败: " + reply->errorString());
            return;
        }
        QByteArray respData = reply->readAll();
        qDebug() << "[HTTP] fetchSignedWssUrl response:" << QString::fromUtf8(respData.left(200));
        auto doc = QJsonDocument::fromJson(respData);
        auto obj = doc.object();

        // 响应结构: {"Code":0,"Data":{"WssUrl":"wss://..."}}
        QJsonObject dataObj = obj.contains("Data") ? obj["Data"].toObject() : obj;
        if (dataObj.contains("WssUrl")) {
            qDebug() << "[HTTP] WssUrl obtained successfully";
            emit wssUrlReady(dataObj["WssUrl"].toString());
        } else if (dataObj.contains("wssUrl")) {
            emit wssUrlReady(dataObj["wssUrl"].toString());
        } else {
            qWarning() << "[HTTP] fetchSignedWssUrl: unexpected response format";
            emit error("签名API返回格式异常: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        }
    });
}
