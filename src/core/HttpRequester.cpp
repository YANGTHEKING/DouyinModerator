#include "HttpRequester.h"
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

HttpRequester::HttpRequester(QObject* parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

void HttpRequester::fetchTtwid(const QString& liveId) {
    QUrl url(QString("https://live.douyin.com/%1").arg(liveId));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    req.setRawHeader("cookie", "__ac_nonce=0638a7b21a08e7d9a3");

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("获取ttwid失败: " + reply->errorString());
            return;
        }
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
    QUrl url(QString("https://live.douyin.com/%1").arg(liveId));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    QString cookie = QString("ttwid=%1; __ac_nonce=0638a7b21a08e7d9a3").arg(ttwid);
    req.setRawHeader("cookie", cookie.toUtf8());

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, liveId, ttwid]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("获取房间信息失败: " + reply->errorString());
            return;
        }
        QString html = QString::fromUtf8(reply->readAll());
        RoomInfo info;
        info.liveId = liveId;
        info.isStreaming = true;

        // Extract roomId
        QRegularExpression reRoom("\"roomId\":\"(\\d+)\"");
        auto m1 = reRoom.match(html);
        if (m1.hasMatch()) {
            info.roomId = m1.captured(1);
        }

        // Extract user_unique_id
        QRegularExpression reUid("\"user_unique_id\":\"(\\d+)\"");
        auto m2 = reUid.match(html);
        if (m2.hasMatch()) {
            info.userUniqueId = m2.captured(1);
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
    QUrl url(SIGN_API);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["ApiKey"] = apiKey;
    body["RoomId"] = roomId;
    body["UserUniqueId"] = userUniqueId;
    body["BrowserName"] = "Chrome";
    body["BrowserVersion"] = "120.0.0.0";

    auto* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("签名API请求失败: " + reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto obj = doc.object();
        if (obj.contains("WssUrl")) {
            emit wssUrlReady(obj["WssUrl"].toString());
        } else if (obj.contains("wssUrl")) {
            emit wssUrlReady(obj["wssUrl"].toString());
        } else {
            emit error("签名API返回格式异常: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        }
    });
}
