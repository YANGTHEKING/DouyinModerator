#include "TimerEngine.h"

TimerEngine::TimerEngine(QObject* parent)
    : QObject(parent), m_autoLikeTimer(new QTimer(this))
{
    connect(m_autoLikeTimer, &QTimer::timeout, this, [this]() {
        emit sendLike();
        emit logMessage("[自动点赞] 已发送点赞");
    });
}

void TimerEngine::addTimedBarrage(const QString& text, int intervalSeconds) {
    auto* timer = new QTimer(this);
    timer->setInterval(intervalSeconds * 1000);
    TimedBarrage b{text, intervalSeconds, timer};
    connect(timer, &QTimer::timeout, this, [this, text]() {
        emit sendBarrage(text);
        emit logMessage(QString("[定时弹幕] 发送: %1").arg(text));
    });
    m_barrages.append(b);
}

void TimerEngine::removeTimedBarrage(int index) {
    if (index >= 0 && index < m_barrages.size()) {
        m_barrages[index].timer->stop();
        delete m_barrages[index].timer;
        m_barrages.removeAt(index);
    }
}

void TimerEngine::clearTimedBarrages() {
    for (auto& b : m_barrages) {
        b.timer->stop();
        delete b.timer;
    }
    m_barrages.clear();
}

QList<QPair<QString, int>> TimerEngine::timedBarrages() const {
    QList<QPair<QString, int>> result;
    for (const auto& b : m_barrages) {
        result.append({b.text, b.intervalSeconds});
    }
    return result;
}

void TimerEngine::startAutoLike(int intervalSeconds) {
    m_autoLikeInterval = intervalSeconds;
    m_autoLikeTimer->setInterval(intervalSeconds * 1000);
    m_autoLikeTimer->start();
}

void TimerEngine::stopAutoLike() {
    m_autoLikeTimer->stop();
}

bool TimerEngine::isAutoLiking() const {
    return m_autoLikeTimer->isActive();
}

int TimerEngine::autoLikeInterval() const { return m_autoLikeInterval; }

void TimerEngine::startAll() {
    for (auto& b : m_barrages) {
        b.timer->start();
    }
}

void TimerEngine::stopAll() {
    for (auto& b : m_barrages) {
        b.timer->stop();
    }
    m_autoLikeTimer->stop();
}
