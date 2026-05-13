#include "BarrageSender.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

BarrageSender::BarrageSender(QObject* parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

void BarrageSender::setCookies(const QMap<QString, QString>& cookies) {
    m_cookies = cookies;
}

void BarrageSender::setRoomInfo(const QString& liveId, const QString& roomId) {
    m_liveId = liveId;
    m_roomId = roomId;
}

bool BarrageSender::isLoggedIn() const {
    return m_cookies.contains("sessionid") && !m_cookies["sessionid"].isEmpty();
}

QString BarrageSender::buildCookieString() const {
    QStringList parts;
    for (auto it = m_cookies.constBegin(); it != m_cookies.constEnd(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }
    return parts.join("; ");
}

void BarrageSender::postToEndpoint(const QString& path, const QMap<QString, QString>& params,
                                    std::function<void(bool, const QString&)> callback) {
    if (!isLoggedIn()) {
        callback(false, "未登录");
        return;
    }
    QUrl url(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                  "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    req.setRawHeader("cookie", buildCookieString().toUtf8());
    req.setRawHeader("referer", QString("https://live.douyin.com/%1").arg(m_liveId).toUtf8());

    QUrlQuery query;
    query.addQueryItem("web_rid", m_liveId);
    query.addQueryItem("room_id", m_roomId);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    query.addQueryItem("identity", "0");

    auto* reply = m_nam->post(req, query.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto obj = doc.object();
        int statusCode = obj.value("status_code").toInt(-1);
        if (statusCode == 0) {
            callback(true, "");
        } else {
            QString msg = obj.value("data").toObject().value("prompts").toString();
            if (msg.isEmpty()) msg = QString("status_code=%1").arg(statusCode);
            callback(false, msg);
        }
    });
}

void BarrageSender::sendBarrage(const QString& text) {
    if (!isLoggedIn()) {
        emit barrageFailed(text, "请先登录抖音账号");
        emit logMessage("[弹幕发送失败] 未登录");
        return;
    }
    QMap<QString, QString> params;
    params["content"] = text;

    postToEndpoint("https://live.douyin.com/webcast/room/chat/", params,
        [this, text](bool ok, const QString& err) {
            if (ok) {
                emit barrageSent(text);
                emit logMessage(QString("[弹幕已发送] %1").arg(text));
            } else {
                emit barrageFailed(text, err);
                emit logMessage(QString("[弹幕发送失败] %1 - %2").arg(text, err));
            }
        });
}

void BarrageSender::sendLike() {
    if (!isLoggedIn()) {
        emit likeFailed("未登录");
        emit logMessage("[点赞失败] 未登录");
        return;
    }
    QMap<QString, QString> params;
    params["click_content"] = "";

    postToEndpoint("https://live.douyin.com/webcast/room/like/", params,
        [this](bool ok, const QString& err) {
            if (ok) {
                emit likeSent();
            } else {
                emit likeFailed(err);
                emit logMessage(QString("[点赞失败] %1").arg(err));
            }
        });
}
