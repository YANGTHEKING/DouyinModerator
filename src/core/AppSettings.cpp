#include "AppSettings.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

AppSettings::AppSettings(QObject* parent)
    : QObject(parent), m_settings("DouyinModerator", "DouyinModerator") {}

QString AppSettings::lastLiveId() const { return m_settings.value("liveId").toString(); }
void AppSettings::setLastLiveId(const QString& id) { m_settings.setValue("liveId", id); }

QString AppSettings::apiKey() const { return m_settings.value("apiKey").toString(); }
void AppSettings::setApiKey(const QString& key) { m_settings.setValue("apiKey", key); }

int AppSettings::maxLogEntries() const { return m_settings.value("maxLogEntries", 1000).toInt(); }
void AppSettings::setMaxLogEntries(int count) { m_settings.setValue("maxLogEntries", count); }

QList<AutoResponseRule> AppSettings::loadRules() const {
    QList<AutoResponseRule> rules;
    QJsonDocument doc = QJsonDocument::fromJson(m_settings.value("rules").toByteArray());
    for (const auto& val : doc.array()) {
        auto obj = val.toObject();
        AutoResponseRule r;
        r.id = obj["id"].toString();
        r.name = obj["name"].toString();
        r.trigger = static_cast<TriggerType>(obj["trigger"].toInt());
        r.matchPattern = obj["matchPattern"].toString();
        r.responseText = obj["responseText"].toString();
        r.cooldownSeconds = obj["cooldown"].toInt();
        r.enabled = obj["enabled"].toBool();
        rules.append(r);
    }
    return rules;
}

void AppSettings::saveRules(const QList<AutoResponseRule>& rules) {
    QJsonArray arr;
    for (const auto& r : rules) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["name"] = r.name;
        obj["trigger"] = static_cast<int>(r.trigger);
        obj["matchPattern"] = r.matchPattern;
        obj["responseText"] = r.responseText;
        obj["cooldown"] = r.cooldownSeconds;
        obj["enabled"] = r.enabled;
        arr.append(obj);
    }
    m_settings.setValue("rules", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QList<QPair<QString, int>> AppSettings::loadTimedBarrages() const {
    QList<QPair<QString, int>> barrages;
    QJsonDocument doc = QJsonDocument::fromJson(m_settings.value("timedBarrages").toByteArray());
    for (const auto& val : doc.array()) {
        auto obj = val.toObject();
        barrages.append({obj["text"].toString(), obj["interval"].toInt()});
    }
    return barrages;
}

void AppSettings::saveTimedBarrages(const QList<QPair<QString, int>>& barrages) {
    QJsonArray arr;
    for (const auto& b : barrages) {
        QJsonObject obj;
        obj["text"] = b.first;
        obj["interval"] = b.second;
        arr.append(obj);
    }
    m_settings.setValue("timedBarrages", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

int AppSettings::autoLikeInterval() const { return m_settings.value("autoLikeInterval", 30).toInt(); }
void AppSettings::setAutoLikeInterval(int seconds) { m_settings.setValue("autoLikeInterval", seconds); }
bool AppSettings::autoLikeEnabled() const { return m_settings.value("autoLikeEnabled", false).toBool(); }
void AppSettings::setAutoLikeEnabled(bool enabled) { m_settings.setValue("autoLikeEnabled", enabled); }

QMap<QString, QString> AppSettings::savedCookies() const {
    QMap<QString, QString> cookies;
    QJsonDocument doc = QJsonDocument::fromJson(m_settings.value("cookies").toByteArray());
    auto obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        cookies[it.key()] = it.value().toString();
    }
    return cookies;
}

void AppSettings::saveCookies(const QMap<QString, QString>& cookies) {
    QJsonObject obj;
    for (auto it = cookies.begin(); it != cookies.end(); ++it) {
        obj[it.key()] = it.value();
    }
    m_settings.setValue("cookies", QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray AppSettings::windowGeometry() const { return m_settings.value("windowGeometry").toByteArray(); }
void AppSettings::setWindowGeometry(const QByteArray& geometry) { m_settings.setValue("windowGeometry", geometry); }
