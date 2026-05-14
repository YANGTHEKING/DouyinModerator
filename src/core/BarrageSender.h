#pragma once
#include <QObject>
#include <QMap>
#include <QWebEngineView>

class BarrageSender : public QObject {
    Q_OBJECT
public:
    explicit BarrageSender(QObject* parent = nullptr);

    void setCookies(const QMap<QString, QString>& cookies);
    void setTtwid(const QString& ttwid);
    void setRoomInfo(const QString& liveId, const QString& roomId);
    // 设置登录后的 webview，通过它执行 JS 发请求（避免 cookie/会话不一致）
    void setWebView(QWebEngineView* webView);

    void sendBarrage(const QString& text);
    void sendLike();
    bool isLoggedIn() const;

signals:
    void barrageSent(const QString& text);
    void barrageFailed(const QString& text, const QString& error);
    void likeSent();
    void likeFailed(const QString& error);
    void logMessage(const QString& msg);

private:
    QWebEngineView* m_webView = nullptr;
    QMap<QString, QString> m_cookies;
    QString m_ttwid;
    QString m_liveId;
    QString m_roomId;

    // 通过 webview JS 发送 POST 请求
    void postViaWebView(const QString& path, const QMap<QString, QString>& params,
                        std::function<void(bool, const QString&)> callback);
    void doActualPost(const QString& path, const QMap<QString, QString>& params,
                      std::function<void(bool, const QString&)> callback);
};
