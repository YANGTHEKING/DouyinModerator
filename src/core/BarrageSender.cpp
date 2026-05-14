#include "BarrageSender.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
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

void BarrageSender::sendLike() {
    if (!m_webView) {
        emit likeFailed("WebView 未初始化");
        emit logMessage("[点赞失败] 请重新登录");
        return;
    }

    // 拦截并重放 Douyin 的点赞 API 请求
    // 先注入拦截器捕获一次真实请求，然后复用
    QString js = R"JS(
        (function() {
            return new Promise(function(resolve) {
                var origSend = XMLHttpRequest.prototype.send;
                var origOpen = XMLHttpRequest.prototype.open;
                var captured = null;

                XMLHttpRequest.prototype.open = function(method, url) {
                    this._url = url;
                    this._method = method;
                    return origOpen.apply(this, arguments);
                };

                XMLHttpRequest.prototype.send = function(body) {
                    if (this._url && this._url.indexOf('/webcast/room/like') !== -1) {
                        captured = {
                            url: this._url,
                            method: this._method,
                            body: body,
                            headers: {}
                        };
                        var xhr = this;
                        var origSetHeader = this.setRequestHeader;
                        this.setRequestHeader = function(name, value) {
                            if (captured) captured.headers[name] = value;
                            return origSetHeader.apply(this, arguments);
                        };
                    }
                    return origSend.apply(this, arguments);
                };

                // 找点赞区域触发点击
                var player = document.querySelector('video')
                          || document.querySelector('[class*="player"]')
                          || document.querySelector('[class*="Player"]');
                if (player) {
                    var rect = player.getBoundingClientRect();
                    var x = rect.left + rect.width / 2;
                    var y = rect.top + rect.height / 2;
                    player.dispatchEvent(new PointerEvent('pointerdown', {bubbles: true, clientX: x, clientY: y, pointerId: 1, pointerType: 'touch'}));
                    player.dispatchEvent(new PointerEvent('pointerup', {bubbles: true, clientX: x, clientY: y, pointerId: 1, pointerType: 'touch'}));
                    setTimeout(function() {
                        player.dispatchEvent(new PointerEvent('pointerdown', {bubbles: true, clientX: x, clientY: y, pointerId: 1, pointerType: 'touch'}));
                        player.dispatchEvent(new PointerEvent('pointerup', {bubbles: true, clientX: x, clientY: y, pointerId: 1, pointerType: 'touch'}));
                    }, 80);
                }

                setTimeout(function() {
                    XMLHttpRequest.prototype.send = origSend;
                    XMLHttpRequest.prototype.open = origOpen;
                    resolve(JSON.stringify({captured: captured, hasPlayer: !!player}));
                }, 2000);
            });
        })()
    )JS";

    qDebug() << "[Sender] 注入 XHR 拦截器 + 模拟双击";
    m_webView->page()->runJavaScript(js, [this](const QVariant& result) {
        qDebug() << "[Sender] 拦截结果:" << result.toString().left(500);
        auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
        auto obj = doc.object();
        auto captured = obj["captured"].toObject();

        if (!captured.isEmpty()) {
            // 捕获到了真实请求，直接用同样的参数重放
            QString url = captured["url"].toString();
            QString body = captured["body"].toString();
            qDebug() << "[Sender] 捕获到真实请求:" << url << "body:" << body;

            // 通过 webview XHR 重放
            QString replayJs = QString(R"JS(
                (function() {
                    return new Promise(function(resolve) {
                        var xhr = new XMLHttpRequest();
                        xhr.open('POST', '%1', true);
                        xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
                        xhr.withCredentials = true;
                        xhr.timeout = 10000;
                        xhr.onload = function() { resolve(xhr.responseText); };
                        xhr.onerror = function() { resolve('{"error":"replay failed"}'); };
                        xhr.ontimeout = function() { resolve('{"error":"replay timeout"}'); };
                        xhr.send('%2');
                    });
                })()
            )JS").arg(url, body);

            m_webView->page()->runJavaScript(replayJs, [this](const QVariant& replayResult) {
                qDebug() << "[Sender] 重放结果:" << replayResult.toString().left(500);
                auto rdoc = QJsonDocument::fromJson(replayResult.toString().toUtf8());
                int sc = rdoc.object().value("status_code").toInt(-1);
                if (sc == 0) {
                    emit likeSent();
                    emit logMessage("[点赞成功]");
                } else {
                    emit likeFailed(replayResult.toString().left(100));
                    emit logMessage(QString("[点赞失败] %1").arg(replayResult.toString().left(100)));
                }
            });
        } else {
            emit likeFailed("未能捕获点赞请求，抖音可能更新了前端实现");
            emit logMessage("[点赞失败] 未捕获到点赞 API 调用");
        }
    });
}

void BarrageSender::sendBarrage(const QString& text) {
    if (!m_webView) {
        emit barrageFailed(text, "WebView 未初始化");
        emit logMessage("[弹幕发送失败] 请重新登录");
        return;
    }

    // 同样用拦截器方式：捕获弹幕 API 请求格式，然后重放
    QString escapedText = text;
    escapedText.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n");

    QString js = QString(R"JS(
        (function() {
            return new Promise(function(resolve) {
                var origSend = XMLHttpRequest.prototype.send;
                var origOpen = XMLHttpRequest.prototype.open;
                var captured = null;

                XMLHttpRequest.prototype.open = function(method, url) {
                    this._url = url;
                    this._method = method;
                    return origOpen.apply(this, arguments);
                };

                XMLHttpRequest.prototype.send = function(body) {
                    if (this._url && this._url.indexOf('/webcast/room/chat') !== -1) {
                        captured = {url: this._url, method: this._method, body: body};
                    }
                    return origSend.apply(this, arguments);
                };

                // 找到输入框并输入文字
                var input = document.querySelector('textarea')
                         || document.querySelector('[contenteditable="true"]');
                if (input) {
                    input.focus();
                    if (input.tagName === 'TEXTAREA') {
                        var setter = Object.getOwnPropertyDescriptor(HTMLTextAreaElement.prototype, 'value')?.set;
                        if (setter) setter.call(input, '%1');
                        else input.value = '%1';
                        input.dispatchEvent(new Event('input', {bubbles: true}));
                    } else {
                        document.execCommand('selectAll', false);
                        document.execCommand('insertText', false, '%1');
                    }
                    // 点击发送按钮
                    setTimeout(function() {
                        var sendBtn = document.querySelector('button[class*="send"]')
                                   || document.querySelector('[data-e2e="chat-send-btn"]');
                        if (sendBtn) sendBtn.click();
                        else input.dispatchEvent(new KeyboardEvent('keydown', {key: 'Enter', keyCode: 13, bubbles: true}));
                    }, 200);
                }

                setTimeout(function() {
                    XMLHttpRequest.prototype.send = origSend;
                    XMLHttpRequest.prototype.open = origOpen;
                    resolve(JSON.stringify({captured: captured, hasInput: !!input}));
                }, 3000);
            });
        })()
    )JS").arg(escapedText);

    qDebug() << "[Sender] 尝试发送弹幕:" << text;
    m_webView->page()->runJavaScript(js, [this, text](const QVariant& result) {
        qDebug() << "[Sender] 弹幕拦截结果:" << result.toString().left(500);
        auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
        auto obj = doc.object();
        auto captured = obj["captured"].toObject();

        if (!captured.isEmpty()) {
            QString url = captured["url"].toString();
            QString body = captured["body"].toString();

            QString replayJs = QString(R"JS(
                (function() {
                    return new Promise(function(resolve) {
                        var xhr = new XMLHttpRequest();
                        xhr.open('POST', '%1', true);
                        xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
                        xhr.withCredentials = true;
                        xhr.timeout = 10000;
                        xhr.onload = function() { resolve(xhr.responseText); };
                        xhr.onerror = function() { resolve('{"error":"replay failed"}'); };
                        xhr.ontimeout = function() { resolve('{"error":"replay timeout"}'); };
                        xhr.send('%2');
                    });
                })()
            )JS").arg(url, body);

            m_webView->page()->runJavaScript(replayJs, [this, text](const QVariant& replayResult) {
                auto rdoc = QJsonDocument::fromJson(replayResult.toString().toUtf8());
                int sc = rdoc.object().value("status_code").toInt(-1);
                if (sc == 0) {
                    emit barrageSent(text);
                    emit logMessage(QString("[弹幕已发送] %1").arg(text));
                } else {
                    emit barrageFailed(text, replayResult.toString().left(100));
                    emit logMessage(QString("[弹幕发送失败] %1 - %2").arg(text, replayResult.toString().left(100)));
                }
            });
        } else {
            emit barrageFailed(text, "未能捕获弹幕请求");
            emit logMessage("[弹幕发送失败] 未捕获到弹幕 API 调用");
        }
    });
}
