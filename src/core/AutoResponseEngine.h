#pragma once
#include <QObject>
#include <QMap>
#include <QDateTime>
#include "../models/AutoResponseRule.h"

class AutoResponseEngine : public QObject {
    Q_OBJECT
public:
    explicit AutoResponseEngine(QObject* parent = nullptr);

    void addRule(const AutoResponseRule& rule);
    void removeRule(const QString& ruleId);
    void updateRule(const AutoResponseRule& rule);
    QList<AutoResponseRule> rules() const;
    void setRules(const QList<AutoResponseRule>& rules);

    void onChat(const QString& userId, const QString& userName, const QString& content);
    void onMember(const QString& userId, const QString& userName);
    void onGift(const QString& userId, const QString& userName,
                const QString& giftName, int64_t diamondCount, int64_t count);
    void onFollow(const QString& userId, const QString& userName);
    void onLike(const QString& userId, const QString& userName, int64_t count);
    void onFansclub(const QString& userId, const QString& userName, int32_t level);

signals:
    void sendBarrage(const QString& text);
    void logMessage(const QString& msg);

private:
    bool checkCooldown(const QString& ruleId);
    void fireRule(const AutoResponseRule& rule, const QString& userName,
                  const QString& extraField = "", const QString& extraValue = "");
    QString resolveTemplate(const QString& tmpl, const QString& userName,
                            const QString& extraField = "", const QString& extraValue = "");

    QList<AutoResponseRule> m_rules;
    QMap<QString, QDateTime> m_lastFired;
};
