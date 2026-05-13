#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include "../models/AutoResponseRule.h"
#include "../core/EmojiResolver.h"

class RuleEditorPanel : public QWidget {
    Q_OBJECT
public:
    explicit RuleEditorPanel(EmojiResolver* emoji, QWidget* parent = nullptr);

    void setRules(const QList<AutoResponseRule>& rules);
    QList<AutoResponseRule> rules() const;
    void addRuleFromTable(const AutoResponseRule& rule);

signals:
    void rulesChanged();

private slots:
    void addRule();
    void editRule();
    void removeRule();
    void toggleRule();

private:
    void refreshTable();

    QTableWidget* m_table;
    QPushButton* m_addBtn;
    QPushButton* m_editBtn;
    QPushButton* m_removeBtn;
    QList<AutoResponseRule> m_rules;
    EmojiResolver* m_emojiResolver;
};
