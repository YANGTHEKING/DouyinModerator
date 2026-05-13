#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>

class BarrageSender : public QObject {
    Q_OBJECT
public:
    explicit BarrageSender(QObject* parent = nullptr);

    void setCookies(const QMap<QString, QString>& cookies);
    void setRoomInfo(const QString& liveId, const QString& roomId);

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
    QNetworkAccessManager* m_nam;
    QMap<QString, QString> m_cookies;
    QString m_liveId;
    QString m_roomId;

    QString buildCookieString() const;
    void postToEndpoint(const QString& path, const QMap<QString, QString>& params,
                        std::function<void(bool, const QString&)> callback);
};
