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
};
