#include "BarrageSender.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QDebug>

BarrageSender::BarrageSender(QObject* parent) : QObject(parent) {}

void BarrageSender::setCookies(const QMap<QString, QString>& cookies) {
    m_cookies = cookies;
}

void BarrageSender::setTtwid(const QString& ttwid) {
    m_ttwid = ttwid;
}

void BarrageSender::setRoomInfo(const QString& liveId, const QString& roomId) {
    m_liveId = liveId;
    m_roomId = roomId;
}

void BarrageSender::setWebView(QWebEngineView* webView) {
    m_webView = webView;
}

bool BarrageSender::isLoggedIn() const {
    return m_cookies.contains("sessionid") && !m_cookies["sessionid"].isEmpty();
}

void BarrageSender::postViaWebView(const QString& path, const QMap<QString, QString>& params,
                                    std::function<void(bool, const QString&)> callback) {
    if (!m_webView) {
        callback(false, "WebView 未初始化，请先登录");
        return;
    }

    // 构建 POST body
    QUrlQuery query;
    query.addQueryItem("web_rid", m_liveId);
    query.addQueryItem("room_id", m_roomId);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    query.addQueryItem("identity", "0");

    QString body = query.query(QUrl::FullyEncoded);
    QString jsCode = QString(R"(
        (async function() {
            try {
                const resp = await fetch('%1', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                        'Referer': 'https://live.douyin.com/%2'
                    },
                    body: '%3',
                    credentials: 'include'
                });
                const data = await resp.json();
                return JSON.stringify(data);
            } catch(e) {
                return JSON.stringify({error: e.toString()});
            }
        })()
    )").arg(path, m_liveId, body);

    qDebug() << "[Sender] POST via JS:" << path << "body:" << body.left(200);

    m_webView->page()->runJavaScript(jsCode, [callback, path](const QVariant& result) {
        QString respStr = result.toString();
        qDebug() << "[Sender] JS Response:" << respStr.left(500);

        auto doc = QJsonDocument::fromJson(respStr.toUtf8());
        auto obj = doc.object();

        if (obj.contains("error")) {
            callback(false, obj["error"].toString());
            return;
        }

        int statusCode = obj.value("status_code").toInt(-1);
        if (statusCode == 0) {
            callback(true, "");
        } else {
            QString msg = obj.value("data").toObject().value("prompts").toString();
            if (msg.isEmpty()) msg = obj.value("status_message").toString();
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
    if (!m_webView) {
        emit barrageFailed(text, "WebView 未初始化");
        emit logMessage("[弹幕发送失败] WebView 未初始化");
        return;
    }
    QMap<QString, QString> params;
    params["content"] = text;

    postViaWebView("https://live.douyin.com/webcast/room/chat/", params,
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
    if (!m_webView) {
        emit likeFailed("WebView 未初始化");
        emit logMessage("[点赞失败] WebView 未初始化");
        return;
    }
    QMap<QString, QString> params;
    params["click_content"] = "";

    postViaWebView("https://live.douyin.com/webcast/room/like/", params,
        [this](bool ok, const QString& err) {
            if (ok) {
                emit likeSent();
                emit logMessage("[点赞成功]");
            } else {
                emit likeFailed(err);
                emit logMessage(QString("[点赞失败] %1").arg(err));
            }
        });
}
