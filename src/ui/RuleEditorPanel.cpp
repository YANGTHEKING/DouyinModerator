#include "RuleEditorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>

RuleEditorPanel::RuleEditorPanel(EmojiResolver* emoji, QWidget* parent)
    : QWidget(parent), m_emojiResolver(emoji)
{
    auto* mainLayout = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"名称", "触发条件", "匹配内容", "回复内容", "启用"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout;
    m_addBtn = new QPushButton("添加规则", this);
    m_editBtn = new QPushButton("编辑", this);
    m_removeBtn = new QPushButton("删除", this);
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &RuleEditorPanel::addRule);
    connect(m_editBtn, &QPushButton::clicked, this, &RuleEditorPanel::editRule);
    connect(m_removeBtn, &QPushButton::clicked, this, &RuleEditorPanel::removeRule);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &RuleEditorPanel::editRule);
    connect(m_table, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (m_refreshing) return;
        if (col == 4 && row >= 0 && row < m_rules.size()) {
            auto* item = m_table->item(row, col);
            if (item) {
                m_rules[row].enabled = (item->checkState() == Qt::Checked);
                emit rulesChanged();
            }
        }
    });
}

void RuleEditorPanel::setRules(const QList<AutoResponseRule>& rules) {
    m_rules = rules;
    refreshTable();
}

QList<AutoResponseRule> RuleEditorPanel::rules() const { return m_rules; }

void RuleEditorPanel::addRuleFromTable(const AutoResponseRule& rule) {
    m_rules.append(rule);
    refreshTable();
}

void RuleEditorPanel::refreshTable() {
    m_refreshing = true;
    m_table->setRowCount(0);
    for (int i = 0; i < m_rules.size(); ++i) {
        const auto& r = m_rules[i];
        m_table->insertRow(i);
        m_table->setItem(i, 0, new QTableWidgetItem(r.name));
        m_table->setItem(i, 1, new QTableWidgetItem(AutoResponseRule::triggerTypeName(r.trigger)));
        m_table->setItem(i, 2, new QTableWidgetItem(r.matchPattern.isEmpty() ? "*" : r.matchPattern));
        m_table->setItem(i, 3, new QTableWidgetItem(r.responseText));
        auto* checkItem = new QTableWidgetItem;
        checkItem->setCheckState(r.enabled ? Qt::Checked : Qt::Unchecked);
        checkItem->setText(r.enabled ? "是" : "否");
        m_table->setItem(i, 4, checkItem);
    }
    m_refreshing = false;
}

void RuleEditorPanel::addRule() {
    QDialog dlg(this);
    dlg.setWindowTitle("添加规则");
    auto* form = new QFormLayout(&dlg);

    auto* nameEdit = new QLineEdit(&dlg);
    auto* triggerCombo = new QComboBox(&dlg);
    triggerCombo->addItem("进入房间", static_cast<int>(TriggerType::OnMemberEnter));
    triggerCombo->addItem("关注", static_cast<int>(TriggerType::OnFollow));
    triggerCombo->addItem("收到礼物", static_cast<int>(TriggerType::OnGift));
    triggerCombo->addItem("特定礼物", static_cast<int>(TriggerType::OnSpecificGift));
    triggerCombo->addItem("加入粉丝团", static_cast<int>(TriggerType::OnFansclubJoin));
    triggerCombo->addItem("点赞", static_cast<int>(TriggerType::OnLike));
    triggerCombo->addItem("聊天关键词", static_cast<int>(TriggerType::OnChatKeyword));

    auto* patternEdit = new QLineEdit(&dlg);
    patternEdit->setPlaceholderText("* 表示匹配全部");
    auto* responseEdit = new QLineEdit(&dlg);
    responseEdit->setPlaceholderText("支持 {user} {gift} {count} {emoji:xxx}");
    auto* cooldownSpin = new QSpinBox(&dlg);
    cooldownSpin->setRange(0, 3600);
    cooldownSpin->setValue(5);
    cooldownSpin->setSuffix(" 秒");

    form->addRow("规则名称:", nameEdit);
    form->addRow("触发条件:", triggerCombo);
    form->addRow("匹配内容:", patternEdit);
    form->addRow("回复内容:", responseEdit);
    form->addRow("冷却时间:", cooldownSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted && !nameEdit->text().isEmpty()) {
        auto rule = AutoResponseRule::create(
            nameEdit->text(),
            static_cast<TriggerType>(triggerCombo->currentData().toInt()),
            patternEdit->text(),
            responseEdit->text(),
            cooldownSpin->value());
        m_rules.append(rule);
        refreshTable();
        emit rulesChanged();
    }
}

void RuleEditorPanel::editRule() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_rules.size()) return;

    auto& rule = m_rules[row];
    QDialog dlg(this);
    dlg.setWindowTitle("编辑规则");
    auto* form = new QFormLayout(&dlg);

    auto* nameEdit = new QLineEdit(rule.name, &dlg);
    auto* triggerCombo = new QComboBox(&dlg);
    triggerCombo->addItem("进入房间", static_cast<int>(TriggerType::OnMemberEnter));
    triggerCombo->addItem("关注", static_cast<int>(TriggerType::OnFollow));
    triggerCombo->addItem("收到礼物", static_cast<int>(TriggerType::OnGift));
    triggerCombo->addItem("特定礼物", static_cast<int>(TriggerType::OnSpecificGift));
    triggerCombo->addItem("加入粉丝团", static_cast<int>(TriggerType::OnFansclubJoin));
    triggerCombo->addItem("点赞", static_cast<int>(TriggerType::OnLike));
    triggerCombo->addItem("聊天关键词", static_cast<int>(TriggerType::OnChatKeyword));
    for (int i = 0; i < triggerCombo->count(); ++i) {
        if (triggerCombo->itemData(i).toInt() == static_cast<int>(rule.trigger)) {
            triggerCombo->setCurrentIndex(i);
            break;
        }
    }

    auto* patternEdit = new QLineEdit(rule.matchPattern, &dlg);
    auto* responseEdit = new QLineEdit(rule.responseText, &dlg);
    responseEdit->setPlaceholderText("支持 {user} {gift} {count} {emoji:xxx}");
    auto* cooldownSpin = new QSpinBox(&dlg);
    cooldownSpin->setRange(0, 3600);
    cooldownSpin->setValue(rule.cooldownSeconds);
    cooldownSpin->setSuffix(" 秒");

    form->addRow("规则名称:", nameEdit);
    form->addRow("触发条件:", triggerCombo);
    form->addRow("匹配内容:", patternEdit);
    form->addRow("回复内容:", responseEdit);
    form->addRow("冷却时间:", cooldownSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        rule.name = nameEdit->text();
        rule.trigger = static_cast<TriggerType>(triggerCombo->currentData().toInt());
        rule.matchPattern = patternEdit->text();
        rule.responseText = responseEdit->text();
        rule.cooldownSeconds = cooldownSpin->value();
        refreshTable();
        emit rulesChanged();
    }
}

void RuleEditorPanel::removeRule() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_rules.size()) return;
    if (QMessageBox::question(this, "确认", "确定删除此规则?") == QMessageBox::Yes) {
        m_rules.removeAt(row);
        refreshTable();
        emit rulesChanged();
    }
}

void RuleEditorPanel::toggleRule() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_rules.size()) return;
    m_rules[row].enabled = !m_rules[row].enabled;
    refreshTable();
    emit rulesChanged();
}
