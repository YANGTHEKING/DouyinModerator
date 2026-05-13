#pragma once
#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include "HttpRequester.h"
#include "../models/RoomInfo.h"

class ConnectionManager : public QObject {
    Q_OBJECT
public:
    explicit ConnectionManager(QObject* parent = nullptr);

    void connectToRoom(const QString& liveId, const QString& apiKey);
    void disconnect();
    bool isConnected() const;
    RoomInfo roomInfo() const;
    QString ttwid() const;

    void sendAck(const QByteArray& frame);

signals:
    void connected(const RoomInfo& info);
    void disconnected();
    void connectionError(const QString& error);
    void rawFrameReceived(const QByteArray& data);
    void statusMessage(const QString& msg);

private slots:
    void onWssConnected();
    void onWssDisconnected();
    void onBinaryMessageReceived(const QByteArray& data);
    void sendHeartbeat();

private:
    void startConnection(const QString& liveId, const QString& apiKey);

    QWebSocket* m_socket;
    QTimer* m_heartbeatTimer;
    HttpRequester* m_http;
    RoomInfo m_roomInfo;
    QString m_ttwid;
    bool m_connected = false;

    static constexpr int HEARTBEAT_INTERVAL_MS = 10000;
};
