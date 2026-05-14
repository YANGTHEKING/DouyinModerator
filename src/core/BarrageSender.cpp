#include "BarrageSender.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
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

    // 在 webview 中找到并点击点赞按钮（让抖音 JS 处理请求）
    QString js = R"JS(
        (function() {
            try {
                var selectors = [
                    '[data-e2e="like-icon"]',
                    '.webcast-chatroom___like-icon',
                    '[class*="like-icon"]',
                    '[class*="LikeIcon"]',
                    '[class*="like-count"]',
                    '[class*="player-container"] [class*="like"]'
                ];
                for (var i = 0; i < selectors.length; i++) {
                    var el = document.querySelector(selectors[i]);
                    if (el) {
                        el.click();
                        return JSON.stringify({ok: true, selector: selectors[i]});
                    }
                }
                // 找所有包含 "like" 的可点击元素
                var els = document.querySelectorAll('[class*="like"], [data-e2e*="like"]');
                if (els.length > 0) {
                    for (var j = 0; j < els.length; j++) {
                        if (els[j].offsetParent !== null) { // 可见
                            els[j].click();
                            return JSON.stringify({ok: true, selector: 'class-match-' + j});
                        }
                    }
                }
                return JSON.stringify({ok: false, error: '未找到点赞按钮'});
            } catch(e) {
                return JSON.stringify({ok: false, error: e.toString()});
            }
        })()
    )JS";

    qDebug() << "[Sender] 尝试点击点赞按钮";
    m_webView->page()->runJavaScript(js, [this](const QVariant& result) {
        qDebug() << "[Sender] 点赞结果:" << result.toString();
        auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
        auto obj = doc.object();
        if (obj["ok"].toBool()) {
            emit likeSent();
            emit logMessage("[点赞成功]");
        } else {
            emit likeFailed(obj["error"].toString("未知错误"));
            emit logMessage(QString("[点赞失败] %1").arg(obj["error"].toString()));
        }
    });
}

void BarrageSender::sendBarrage(const QString& text) {
    if (!m_webView) {
        emit barrageFailed(text, "WebView 未初始化");
        emit logMessage("[弹幕发送失败] 请重新登录");
        return;
    }

    // 在 webview 中找到弹幕输入框，输入并发送
    QString escapedText = text;
    escapedText.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n");

    QString js = QString(R"JS(
        (function() {
            try {
                var input = document.querySelector('textarea')
                         || document.querySelector('[contenteditable="true"][class*="chat"]')
                         || document.querySelector('[data-e2e="chat-input"]');

                if (!input) {
                    // 尝试点击聊天区域打开输入框
                    var areas = document.querySelectorAll('[class*="chat-input"], [class*="ChatInput"], [class*="chatroom"] input');
                    if (areas.length > 0) { areas[0].click(); }
                    setTimeout(function(){}, 300);
                    input = document.querySelector('textarea')
                         || document.querySelector('[contenteditable="true"]');
                }

                if (!input) {
                    return JSON.stringify({ok: false, error: '未找到弹幕输入框'});
                }

                // 设置输入值
                if (input.tagName === 'TEXTAREA' || input.tagName === 'INPUT') {
                    var setter = Object.getOwnPropertyDescriptor(HTMLTextAreaElement.prototype, 'value')?.set
                              || Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value')?.set;
                    if (setter) setter.call(input, '%1');
                    else input.value = '%1';
                    input.dispatchEvent(new Event('input', {bubbles: true}));
                    input.dispatchEvent(new Event('change', {bubbles: true}));
                } else {
                    input.focus();
                    document.execCommand('selectAll', false);
                    document.execCommand('insertText', false, '%1');
                }

                // 延迟发送
                setTimeout(function() {
                    var sendBtn = document.querySelector('button[class*="send"]')
                               || document.querySelector('[data-e2e="chat-send-btn"]');
                    if (sendBtn) {
                        sendBtn.click();
                    } else {
                        input.dispatchEvent(new KeyboardEvent('keydown', {key: 'Enter', keyCode: 13, bubbles: true}));
                    }
                }, 300);

                return JSON.stringify({ok: true});
            } catch(e) {
                return JSON.stringify({ok: false, error: e.toString()});
            }
        })()
    )JS").arg(escapedText);

    qDebug() << "[Sender] 尝试发送弹幕:" << text;
    m_webView->page()->runJavaScript(js, [this, text](const QVariant& result) {
        qDebug() << "[Sender] 弹幕结果:" << result.toString();
        auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
        auto obj = doc.object();
        if (obj["ok"].toBool()) {
            emit barrageSent(text);
            emit logMessage(QString("[弹幕已发送] %1").arg(text));
        } else {
            emit barrageFailed(text, obj["error"].toString("未知错误"));
            emit logMessage(QString("[弹幕发送失败] %1 - %2").arg(text, obj["error"].toString()));
        }
    });
}
