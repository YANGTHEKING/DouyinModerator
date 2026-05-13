#pragma once
#include <QString>
#include <QDateTime>

enum class LiveEventType {
    Member,      // 进入直播间
    Chat,        // 聊天消息
    Gift,        // 送礼
    Like,        // 点赞
    Social,      // 关注
    Share,       // 分享
    Fansclub,    // 加入粉丝团
    Control,     // 控制消息(如直播结束)
    Stats        // 统计信息
};

struct LiveEvent {
    LiveEventType type;
    QString userId;
    QString userName;
    QString content;
    QString giftName;
    int64_t giftDiamonds = 0;
    int64_t count = 0;
    QDateTime timestamp;
};
