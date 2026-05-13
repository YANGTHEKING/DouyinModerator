#pragma once
#include <QString>

struct RoomInfo {
    QString liveId;
    QString roomId;
    QString userUniqueId;
    QString title;
    QString streamerName;
    int64_t viewerCount = 0;
    bool isStreaming = false;
};
