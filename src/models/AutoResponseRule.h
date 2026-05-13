#pragma once
#include <QString>
#include <QUuid>

enum class TriggerType {
    OnMemberEnter,
    OnFollow,
    OnGift,
    OnSpecificGift,
    OnFansclubJoin,
    OnLike,
    OnChatKeyword,
    Always
};

struct AutoResponseRule {
    QString id;
    QString name;
    TriggerType trigger = TriggerType::OnMemberEnter;
    QString matchPattern;     // 礼物名或聊天关键词，"*"表示匹配全部
    QString responseText;     // 回复模板，支持 {user} {gift} {count} {emoji:xxx}
    int cooldownSeconds = 5;
    bool enabled = true;

    static AutoResponseRule create(const QString& name, TriggerType trigger,
                                   const QString& pattern, const QString& response, int cooldown = 5) {
        return {QUuid::createUuid().toString(QUuid::WithoutBraces), name,
                trigger, pattern, response, cooldown, true};
    }

    static QString triggerTypeName(TriggerType t) {
        switch (t) {
            case TriggerType::OnMemberEnter: return "进入房间";
            case TriggerType::OnFollow: return "关注";
            case TriggerType::OnGift: return "收到礼物";
            case TriggerType::OnSpecificGift: return "特定礼物";
            case TriggerType::OnFansclubJoin: return "加入粉丝团";
            case TriggerType::OnLike: return "点赞";
            case TriggerType::OnChatKeyword: return "聊天关键词";
            case TriggerType::Always: return "始终";
        }
        return "未知";
    }
};
