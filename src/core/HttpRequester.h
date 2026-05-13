#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../models/RoomInfo.h"

class HttpRequester : public QObject {
    Q_OBJECT
public:
    explicit HttpRequester(QObject* parent = nullptr);

    void fetchTtwid(const QString& liveId);
    void fetchRoomInfo(const QString& liveId, const QString& ttwid);
    void fetchSignedWssUrl(const QString& roomId, const QString& userUniqueId,
                           const QString& apiKey);

signals:
    void ttwidReady(const QString& ttwid);
    void roomInfoReady(const RoomInfo& info);
    void wssUrlReady(const QString& wssUrl);
    void error(const QString& message);

private:
    QNetworkAccessManager* m_nam;
    static constexpr const char* SIGN_API = "https://api.aiobs.cn/Douyin/Douyin/SignWss";
};
