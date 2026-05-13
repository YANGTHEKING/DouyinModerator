#include "EventLogPanel.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>

EventLogPanel::EventLogPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"时间", "类型", "用户名", "内容"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    layout->addWidget(m_table);
}

void EventLogPanel::setMaxEntries(int max) { m_maxEntries = max; }

void EventLogPanel::addEvent(const LiveEvent& event) {
    if (m_table->rowCount() >= m_maxEntries) {
        m_table->removeRow(0);
    }
    int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* timeItem = new QTableWidgetItem(event.timestamp.toString("HH:mm:ss"));
    auto* typeItem = new QTableWidgetItem(eventTypeText(event.type));
    auto* userItem = new QTableWidgetItem(event.userName);
    auto* contentItem = new QTableWidgetItem(event.content);

    typeItem->setForeground(eventTypeColor(event.type));

    m_table->setItem(row, 0, timeItem);
    m_table->setItem(row, 1, typeItem);
    m_table->setItem(row, 2, userItem);
    m_table->setItem(row, 3, contentItem);
    m_table->scrollToBottom();
}

void EventLogPanel::addLog(const QString& msg) {
    if (m_table->rowCount() >= m_maxEntries) {
        m_table->removeRow(0);
    }
    int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss"));
    auto* typeItem = new QTableWidgetItem("系统");
    auto* contentItem = new QTableWidgetItem(msg);
    typeItem->setForeground(Qt::gray);

    m_table->setItem(row, 0, timeItem);
    m_table->setItem(row, 1, typeItem);
    m_table->setItem(row, 2, new QTableWidgetItem("-"));
    m_table->setItem(row, 3, contentItem);
    m_table->scrollToBottom();
}

void EventLogPanel::clear() {
    m_table->setRowCount(0);
}

QString EventLogPanel::eventTypeText(LiveEventType type) {
    switch (type) {
        case LiveEventType::Member: return "进入";
        case LiveEventType::Chat: return "聊天";
        case LiveEventType::Gift: return "礼物";
        case LiveEventType::Like: return "点赞";
        case LiveEventType::Social: return "关注";
        case LiveEventType::Share: return "分享";
        case LiveEventType::Fansclub: return "入团";
        case LiveEventType::Control: return "控制";
        case LiveEventType::Stats: return "统计";
    }
    return "未知";
}

QColor EventLogPanel::eventTypeColor(LiveEventType type) {
    switch (type) {
        case LiveEventType::Member: return QColor(70, 130, 180);
        case LiveEventType::Chat: return QColor(0, 0, 0);
        case LiveEventType::Gift: return QColor(255, 140, 0);
        case LiveEventType::Like: return QColor(255, 69, 0);
        case LiveEventType::Social: return QColor(34, 139, 34);
        case LiveEventType::Share: return QColor(138, 43, 226);
        case LiveEventType::Fansclub: return QColor(255, 20, 147);
        case LiveEventType::Control: return QColor(128, 128, 128);
        case LiveEventType::Stats: return QColor(100, 100, 100);
    }
    return QColor(0, 0, 0);
}
