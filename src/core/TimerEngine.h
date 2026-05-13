#pragma once
#include <QObject>
#include <QTimer>
#include <QList>

class TimerEngine : public QObject {
    Q_OBJECT
public:
    explicit TimerEngine(QObject* parent = nullptr);

    void addTimedBarrage(const QString& text, int intervalSeconds);
    void removeTimedBarrage(int index);
    void clearTimedBarrages();
    QList<QPair<QString, int>> timedBarrages() const;

    void startAutoLike(int intervalSeconds);
    void stopAutoLike();
    bool isAutoLiking() const;
    int autoLikeInterval() const;

    void startAll();
    void stopAll();

signals:
    void sendBarrage(const QString& text);
    void sendLike();
    void logMessage(const QString& msg);

private:
    struct TimedBarrage {
        QString text;
        int intervalSeconds;
        QTimer* timer;
    };
    QList<TimedBarrage> m_barrages;
    QTimer* m_autoLikeTimer;
    int m_autoLikeInterval = 30;
};
