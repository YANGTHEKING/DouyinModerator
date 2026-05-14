#include "BarrageSender.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUrl>
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

    // 先检查 session 是否还有效（GET 请求）
    QString checkJs = QString(R"JS(
        (function() {
            return new Promise(function(resolve) {
                var xhr = new XMLHttpRequest();
                xhr.open('GET', 'https://live.douyin.com/webcast/room/web/enter/?web_rid=%1', true);
                xhr.withCredentials = true;
                xhr.timeout = 5000;
                xhr.onload = function() { resolve(xhr.responseText); };
                xhr.onerror = function() { resolve('{"error":"check failed"}'); };
                xhr.ontimeout = function() { resolve('{"error":"check timeout"}'); };
                xhr.send();
            });
        })()
    )JS").arg(m_liveId);

    m_webView->page()->runJavaScript(checkJs, [this, path, params, callback](const QVariant& checkResult) {
        // 检查 session 是否有效
        if (checkResult.isValid()) {
            auto checkDoc = QJsonDocument::fromJson(checkResult.toString().toUtf8());
            int checkStatus = checkDoc.object().value("status_code").toInt(-1);
            qDebug() << "[Sender] Session check status_code:" << checkStatus;
            if (checkStatus != 0) {
                callback(false, QString("Session 无效 (status=%1)，请重新登录").arg(checkStatus));
                emit logMessage("[Sender] Session 已过期，请重新登录抖音账号");
                return;
            }
        }

        // Session 有效，发送实际请求
        doActualPost(path, params, callback);
    });
}

void BarrageSender::doActualPost(const QString& path, const QMap<QString, QString>& params,
                                  std::function<void(bool, const QString&)> callback) {
    QUrlQuery query;
    query.addQueryItem("web_rid", m_liveId);
    query.addQueryItem("room_id", m_roomId);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    query.addQueryItem("identity", "0");
    QString body = query.query(QUrl::FullyEncoded);

    qDebug() << "[Sender] POST via JS:" << path << "body:" << body;

    QString jsCode = QString(R"JS(
        (function() {
            return new Promise(function(resolve) {
                try {
                    var xhr = new XMLHttpRequest();
                    xhr.open('POST', '%1', true);
                    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
                    xhr.withCredentials = true;
                    xhr.timeout = 10000;
                    xhr.onload = function() { resolve(xhr.responseText); };
                    xhr.onerror = function() { resolve('{"error":"xhr network error"}'); };
                    xhr.ontimeout = function() { resolve('{"error":"xhr timeout"}'); };
                    xhr.send('%2');
                } catch(e) {
                    resolve('{"error":"' + e.message + '"}');
                }
            });
        })()
    )JS").arg(path, body);

    m_webView->page()->runJavaScript(jsCode, [callback](const QVariant& result) {
        if (!result.isValid() || result.toString().isEmpty()) {
            qWarning() << "[Sender] JS 返回空值";
            callback(false, "JavaScript 执行返回空值");
            return;
        }

        QString respStr = result.toString();
        qDebug() << "[Sender] JS Response:" << respStr.left(500);

        auto doc = QJsonDocument::fromJson(respStr.toUtf8());
        if (doc.isNull()) {
            callback(false, "响应不是有效 JSON: " + respStr.left(200));
            return;
        }
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
        emit barrageFailed(text, "WebView 未初始化，请重新登录");
        emit logMessage("[弹幕发送失败] WebView 未初始化，请重新登录");
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
        emit likeFailed("WebView 未初始化，请重新登录");
        emit logMessage("[点赞失败] WebView 未初始化，请重新登录");
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
