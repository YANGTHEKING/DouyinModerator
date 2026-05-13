#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include "../models/LiveEvent.h"

class EventLogPanel : public QWidget {
    Q_OBJECT
public:
    explicit EventLogPanel(QWidget* parent = nullptr);

    void setMaxEntries(int max);
    void addEvent(const LiveEvent& event);
    void addLog(const QString& msg);
    void clear();

private:
    QTableWidget* m_table;
    int m_maxEntries = 1000;

    QString eventTypeText(LiveEventType type);
    QColor eventTypeColor(LiveEventType type);
};
