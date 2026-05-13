#include "AutoResponseEngine.h"
#include <QRegularExpression>

AutoResponseEngine::AutoResponseEngine(QObject* parent) : QObject(parent) {}

void AutoResponseEngine::addRule(const AutoResponseRule& rule) { m_rules.append(rule); }

void AutoResponseEngine::removeRule(const QString& ruleId) {
    m_rules.erase(std::remove_if(m_rules.begin(), m_rules.end(),
        [&](const AutoResponseRule& r) { return r.id == ruleId; }), m_rules.end());
}

void AutoResponseEngine::updateRule(const AutoResponseRule& rule) {
    for (int i = 0; i < m_rules.size(); ++i) {
        if (m_rules[i].id == rule.id) {
            m_rules[i] = rule;
            return;
        }
    }
}

QList<AutoResponseRule> AutoResponseEngine::rules() const { return m_rules; }

void AutoResponseEngine::setRules(const QList<AutoResponseRule>& rules) { m_rules = rules; }

bool AutoResponseEngine::checkCooldown(const QString& ruleId) {
    if (m_lastFired.contains(ruleId)) {
        auto it = m_rules.constBegin();
        for (; it != m_rules.constEnd(); ++it) {
            if (it->id == ruleId) break;
        }
        if (it != m_rules.constEnd()) {
            int secs = m_lastFired[ruleId].secsTo(QDateTime::currentDateTime());
            if (secs < it->cooldownSeconds) return false;
        }
    }
    return true;
}

QString AutoResponseEngine::resolveTemplate(const QString& tmpl, const QString& userName,
                                             const QString& extraField, const QString& extraValue) {
    QString result = tmpl;
    result.replace("{user}", userName);
    if (!extraField.isEmpty()) {
        result.replace("{" + extraField + "}", extraValue);
    }
    // Resolve emoji shortcodes: {emoji:xxx} → [xxx]
    QRegularExpression re("\\{emoji:([^}]+)\\}");
    auto it = re.globalMatch(result);
    while (it.hasNext()) {
        auto m = it.next();
        result.replace(m.captured(0), "[" + m.captured(1) + "]");
    }
    return result;
}

void AutoResponseEngine::fireRule(const AutoResponseRule& rule, const QString& userName,
                                   const QString& extraField, const QString& extraValue) {
    if (!rule.enabled || !checkCooldown(rule.id)) return;
    QString text = resolveTemplate(rule.responseText, userName, extraField, extraValue);
    m_lastFired[rule.id] = QDateTime::currentDateTime();
    emit sendBarrage(text);
    emit logMessage(QString("[自动回复] 规则'%1'触发，发送: %2").arg(rule.name, text));
}

void AutoResponseEngine::onChat(const QString& userId, const QString& userName, const QString& content) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnChatKeyword) {
            if (rule.matchPattern == "*" || content.contains(rule.matchPattern)) {
                fireRule(rule, userName, "content", content);
            }
        }
    }
}

void AutoResponseEngine::onMember(const QString& userId, const QString& userName) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnMemberEnter) {
            fireRule(rule, userName);
        }
    }
}

void AutoResponseEngine::onGift(const QString& userId, const QString& userName,
                                 const QString& giftName, int64_t diamondCount, int64_t count) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnGift) {
            fireRule(rule, userName, "gift", giftName);
        } else if (rule.trigger == TriggerType::OnSpecificGift) {
            if (rule.matchPattern == giftName) {
                fireRule(rule, userName, "gift", giftName);
            }
        }
    }
}

void AutoResponseEngine::onFollow(const QString& userId, const QString& userName) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnFollow) {
            fireRule(rule, userName);
        }
    }
}

void AutoResponseEngine::onLike(const QString& userId, const QString& userName, int64_t count) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnLike) {
            fireRule(rule, userName, "count", QString::number(count));
        }
    }
}

void AutoResponseEngine::onFansclub(const QString& userId, const QString& userName, int32_t level) {
    for (const auto& rule : m_rules) {
        if (rule.trigger == TriggerType::OnFansclubJoin) {
            fireRule(rule, userName);
        }
    }
}
