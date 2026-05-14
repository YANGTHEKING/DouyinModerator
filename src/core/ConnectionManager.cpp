#include "ConnectionManager.h"
#include <QUrl>
#include <QDebug>

ConnectionManager::ConnectionManager(QObject* parent)
    : QObject(parent),
      m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
      m_heartbeatTimer(new QTimer(this)),
      m_http(new HttpRequester(this))
{
    connect(m_socket, &QWebSocket::connected, this, &ConnectionManager::onWssConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &ConnectionManager::onWssDisconnected);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &ConnectionManager::onBinaryMessageReceived);

    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL_MS);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ConnectionManager::sendHeartbeat);
}

void ConnectionManager::connectToRoom(const QString& liveId, const QString& apiKey) {
    if (m_connected) {
        disconnect();
    }
    startConnection(liveId, apiKey);
}

void ConnectionManager::startConnection(const QString& liveId, const QString& apiKey) {
    qDebug() << "[Conn] startConnection, liveId:" << liveId;
    emit statusMessage("正在获取ttwid...");
    connect(m_http, &HttpRequester::ttwidReady, this, [this, liveId](const QString& ttwid) {
        qDebug() << "[Conn] ttwidReady:" << ttwid.left(20) + "...";
        m_ttwid = ttwid;
        emit statusMessage("正在获取房间信息...");
        m_http->fetchRoomInfo(liveId, ttwid);
    });

    connect(m_http, &HttpRequester::roomInfoReady, this, [this, apiKey](const RoomInfo& info) {
        qDebug() << "[Conn] roomInfoReady, roomId:" << info.roomId << "uid:" << info.userUniqueId << "title:" << info.title;
        m_roomInfo = info;
        emit statusMessage(QString("房间: %1, 正在获取签名...").arg(info.title.isEmpty() ? info.roomId : info.title));
        m_http->fetchSignedWssUrl(info.roomId, info.userUniqueId, apiKey);
    });

    connect(m_http, &HttpRequester::wssUrlReady, this, [this](const QString& wssUrl) {
        qDebug() << "[Conn] wssUrlReady:" << wssUrl.left(80) + "...";
        emit statusMessage("正在连接WebSocket...");
        QNetworkRequest req{QUrl(wssUrl)};
        req.setRawHeader("cookie", QString("ttwid=%1").arg(m_ttwid).toUtf8());
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
        m_socket->open(req);
    });

    connect(m_http, &HttpRequester::error, this, [this](const QString& err) {
        qWarning() << "[Conn] ERROR:" << err;
        emit connectionError(err);
        emit statusMessage("连接失败: " + err);
    });

    qDebug() << "[Conn] calling fetchTtwid...";
    m_http->fetchTtwid(liveId);
}

void ConnectionManager::disconnect() {
    m_heartbeatTimer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }
    m_connected = false;
    emit statusMessage("已断开连接");
    emit disconnected();
}

bool ConnectionManager::isConnected() const { return m_connected; }
RoomInfo ConnectionManager::roomInfo() const { return m_roomInfo; }
QString ConnectionManager::ttwid() const { return m_ttwid; }

void ConnectionManager::sendAck(const QByteArray& frame) {
    if (m_connected) {
        m_socket->sendBinaryMessage(frame);
    }
}

void ConnectionManager::onWssConnected() {
    qDebug() << "[Conn] WebSocket connected!";
    m_connected = true;
    m_heartbeatTimer->start();
    sendHeartbeat(); // Send initial heartbeat
    emit statusMessage("已连接到直播间");
    emit connected(m_roomInfo);
}

void ConnectionManager::onWssDisconnected() {
    qDebug() << "[Conn] WebSocket disconnected, error:" << m_socket->errorString();
    m_connected = false;
    m_heartbeatTimer->stop();
    emit statusMessage("WebSocket已断开");
    emit disconnected();
}

void ConnectionManager::onBinaryMessageReceived(const QByteArray& data) {
    emit rawFrameReceived(data);
}

void ConnectionManager::sendHeartbeat() {
    static const QByteArray heartbeat("\x3a\x02\x68\x62", 4);
    if (m_connected) {
        m_socket->sendBinaryMessage(heartbeat);
    }
}
