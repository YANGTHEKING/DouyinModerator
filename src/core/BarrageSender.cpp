#include "BarrageSender.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QTimer>
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

    // 确保 webview 的 URL 在 live.douyin.com 域下（否则跨域请求会被拒）
    QString currentUrl = m_webView->url().toString();
    if (!currentUrl.contains("live.douyin.com")) {
        qDebug() << "[Sender] WebView URL 不在 live.douyin.com 域下:" << currentUrl;
        qDebug() << "[Sender] 正在重新加载 live.douyin.com ...";
        connect(m_webView, &QWebEngineView::loadFinished, this,
            [this, path, params, callback](bool ok) {
                if (ok) {
                    // 页面加载完后延迟执行
                    QTimer::singleShot(1000, this, [this, path, params, callback]() {
                        doActualPost(path, params, callback);
                    });
                } else {
                    callback(false, "页面加载失败");
                }
            }, Qt::SingleShotConnection);
        m_webView->load(QUrl("https://live.douyin.com/"));
        return;
    }

    doActualPost(path, params, callback);
}

void BarrageSender::doActualPost(const QString& path, const QMap<QString, QString>& params,
                                  std::function<void(bool, const QString&)> callback) {
    // 构建 POST body
    QUrlQuery query;
    query.addQueryItem("web_rid", m_liveId);
    query.addQueryItem("room_id", m_roomId);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    query.addQueryItem("identity", "0");

    // URL 编码 body 避免 JS 注入问题
    QString encodedBody = QString::fromUtf8(QUrl::toPercentEncoding(query.query(QUrl::FullyEncoded)));

    // 先简单检查当前页面是否加载完成
    QString checkJs = QString("document.readyState");

    m_webView->page()->runJavaScript(checkJs, [this, path, encodedBody, callback](const QVariant& readyState) {
        qDebug() << "[Sender] document.readyState:" << readyState.toString();

        QString jsCode = QString(R"JS(
            (function() {
                try {
                    if (typeof fetch === 'undefined') {
                        return '{"error":"fetch not available"}';
                    }
                    var body = decodeURIComponent('%1');
                    var xhr = new XMLHttpRequest();
                    xhr.open('POST', '%2', false);  // synchronous
                    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
                    xhr.withCredentials = true;
                    xhr.send(body);
                    return xhr.responseText;
                } catch(e) {
                    return '{"error":"' + e.toString() + '"}';
                }
            })()
        )JS").arg(encodedBody, path);

        qDebug() << "[Sender] POST via JS:" << path;

        m_webView->page()->runJavaScript(jsCode, [callback, path](const QVariant& result) {
            if (!result.isValid() || result.toString().isEmpty()) {
                qWarning() << "[Sender] JS 返回空值，可能页面未就绪或跨域被拒";
                callback(false, "JavaScript 执行返回空值");
                return;
            }

            QString respStr = result.toString();
            qDebug() << "[Sender] JS Response:" << respStr.left(500);

            auto doc = QJsonDocument::fromJson(respStr.toUtf8());
            if (doc.isNull()) {
                callback(false, "响应不是有效 JSON: " + respStr.left(100));
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
