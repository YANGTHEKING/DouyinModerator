#include "ConfigPanel.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

ConfigPanel::ConfigPanel(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);

    // Login group
    auto* loginGroup = new QGroupBox("登录设置", this);
    auto* loginLayout = new QFormLayout(loginGroup);
    m_loginStatusLabel = new QLabel("未登录", loginGroup);
    m_loginBtn = new QPushButton("登录抖音", loginGroup);
    m_logoutBtn = new QPushButton("退出登录", loginGroup);
    m_logoutBtn->setEnabled(false);

    auto* loginBtnLayout = new QHBoxLayout;
    loginBtnLayout->addWidget(m_loginBtn);
    loginBtnLayout->addWidget(m_logoutBtn);
    loginLayout->addRow("登录状态:", m_loginStatusLabel);
    loginLayout->addRow("", loginBtnLayout);
    mainLayout->addWidget(loginGroup);

    // Room settings
    auto* roomGroup = new QGroupBox("房间设置", this);
    auto* roomLayout = new QFormLayout(roomGroup);
    m_liveIdEdit = new QLineEdit(roomGroup);
    m_liveIdEdit->setPlaceholderText("输入直播间号 (如 123456789)");
    roomLayout->addRow("直播间号:", m_liveIdEdit);
    mainLayout->addWidget(roomGroup);

    // API settings
    auto* apiGroup = new QGroupBox("API设置", this);
    auto* apiLayout = new QFormLayout(apiGroup);
    m_apiKeyEdit = new QLineEdit(apiGroup);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("签名API的ApiKey");
    apiLayout->addRow("API Key:", m_apiKeyEdit);
    mainLayout->addWidget(apiGroup);

    // Auto like
    auto* likeGroup = new QGroupBox("自动点赞", this);
    auto* likeLayout = new QFormLayout(likeGroup);
    m_autoLikeSpin = new QSpinBox(likeGroup);
    m_autoLikeSpin->setRange(5, 300);
    m_autoLikeSpin->setValue(30);
    m_autoLikeSpin->setSuffix(" 秒");
    m_autoLikeCheck = new QCheckBox("启用自动点赞", likeGroup);
    likeLayout->addRow("点赞间隔:", m_autoLikeSpin);
    likeLayout->addRow("", m_autoLikeCheck);
    mainLayout->addWidget(likeGroup);

    // General
    auto* generalGroup = new QGroupBox("通用设置", this);
    auto* generalLayout = new QFormLayout(generalGroup);
    m_maxLogSpin = new QSpinBox(generalGroup);
    m_maxLogSpin->setRange(100, 10000);
    m_maxLogSpin->setValue(1000);
    generalLayout->addRow("最大日志条数:", m_maxLogSpin);
    mainLayout->addWidget(generalGroup);

    m_saveBtn = new QPushButton("保存设置", this);
    mainLayout->addWidget(m_saveBtn);
    mainLayout->addStretch();

    connect(m_loginBtn, &QPushButton::clicked, this, &ConfigPanel::loginRequested);
    connect(m_logoutBtn, &QPushButton::clicked, this, &ConfigPanel::logoutRequested);
    connect(m_saveBtn, &QPushButton::clicked, this, &ConfigPanel::settingsChanged);
}

void ConfigPanel::setLiveId(const QString& id) { m_liveIdEdit->setText(id); }
QString ConfigPanel::liveId() const { return m_liveIdEdit->text().trimmed(); }
void ConfigPanel::setApiKey(const QString& key) { m_apiKeyEdit->setText(key); }
QString ConfigPanel::apiKey() const { return m_apiKeyEdit->text().trimmed(); }
void ConfigPanel::setMaxLogEntries(int count) { m_maxLogSpin->setValue(count); }
int ConfigPanel::maxLogEntries() const { return m_maxLogSpin->value(); }

void ConfigPanel::setLoginStatus(bool loggedIn, const QString& username) {
    if (loggedIn) {
        m_loginStatusLabel->setText(QString("已登录 (%1)").arg(username.isEmpty() ? "已登录" : username));
        m_loginStatusLabel->setStyleSheet("color: green;");
        m_loginBtn->setEnabled(false);
        m_logoutBtn->setEnabled(true);
    } else {
        m_loginStatusLabel->setText("未登录");
        m_loginStatusLabel->setStyleSheet("color: red;");
        m_loginBtn->setEnabled(true);
        m_logoutBtn->setEnabled(false);
    }
}

void ConfigPanel::setAutoLikeInterval(int seconds) { m_autoLikeSpin->setValue(seconds); }
int ConfigPanel::autoLikeInterval() const { return m_autoLikeSpin->value(); }
void ConfigPanel::setAutoLikeEnabled(bool enabled) { m_autoLikeCheck->setChecked(enabled); }
bool ConfigPanel::autoLikeEnabled() const { return m_autoLikeCheck->isChecked(); }
