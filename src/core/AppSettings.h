#pragma once
#include <QObject>
#include <QSettings>
#include <QMap>
#include "../models/AutoResponseRule.h"

class AppSettings : public QObject {
    Q_OBJECT
public:
    explicit AppSettings(QObject* parent = nullptr);

    QString lastLiveId() const;
    void setLastLiveId(const QString& id);

    QString apiKey() const;
    void setApiKey(const QString& key);

    int maxLogEntries() const;
    void setMaxLogEntries(int count);

    QList<AutoResponseRule> loadRules() const;
    void saveRules(const QList<AutoResponseRule>& rules);

    QList<QPair<QString, int>> loadTimedBarrages() const;
    void saveTimedBarrages(const QList<QPair<QString, int>>& barrages);

    int autoLikeInterval() const;
    void setAutoLikeInterval(int seconds);
    bool autoLikeEnabled() const;
    void setAutoLikeEnabled(bool enabled);

    QMap<QString, QString> savedCookies() const;
    void saveCookies(const QMap<QString, QString>& cookies);

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

private:
    QSettings m_settings;
};
