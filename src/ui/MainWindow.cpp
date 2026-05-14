#include "MainWindow.h"
#include "EventLogPanel.h"
#include "RuleEditorPanel.h"
#include "ConfigPanel.h"
#include "LoginDialog.h"

#include "../core/ConnectionManager.h"
#include "../core/ProtobufDecoder.h"
#include "../core/MessageRouter.h"
#include "../core/AutoResponseEngine.h"
#include "../core/TimerEngine.h"
#include "../core/BarrageSender.h"
#include "../core/LoginManager.h"
#include "../core/EmojiResolver.h"
#include "../core/AppSettings.h"
#include "../models/LiveEvent.h"

#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QDateTime>
#include <QWebEngineProfile>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_settings = new AppSettings(this);
    m_emoji = new EmojiResolver(this);
    m_conn = new ConnectionManager(this);
    m_decoder = new ProtobufDecoder(this);
    m_router = new MessageRouter(this);
    m_autoResp = new AutoResponseEngine(this);
    m_timer = new TimerEngine(this);
    m_sender = new BarrageSender(this);
    m_login = new LoginManager(this);

    setupUi();
    setupConnections();
    loadSettings();

    setWindowTitle("抖音直播房管工具");
    resize(900, 650);
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUi() {
    // Toolbar
    auto* toolbar = addToolBar("主工具栏");
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel(" 房间号: ", this));
    m_roomIdEdit = new QLineEdit(this);
    m_roomIdEdit->setFixedWidth(150);
    m_roomIdEdit->setPlaceholderText("直播间号");
    toolbar->addWidget(m_roomIdEdit);

    m_connectBtn = new QPushButton("连接", this);
    m_connectBtn->setFixedWidth(70);
    toolbar->addWidget(m_connectBtn);

    toolbar->addSeparator();

    m_loginBtn = new QPushButton("登录", this);
    m_loginBtn->setFixedWidth(70);
    toolbar->addWidget(m_loginBtn);

    toolbar->addSeparator();

    m_statusLabel = new QLabel(" 未连接", this);
    toolbar->addWidget(m_statusLabel);

    toolbar->addSeparator();

    m_viewerLabel = new QLabel("", this);
    toolbar->addWidget(m_viewerLabel);

    // Central tabs
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_eventLog = new EventLogPanel(this);
    m_ruleEditor = new RuleEditorPanel(m_emoji, this);
    m_configPanel = new ConfigPanel(this);

    m_tabs->addTab(m_eventLog, "事件日志");
    m_tabs->addTab(m_ruleEditor, "自动回复规则");
    m_tabs->addTab(m_configPanel, "设置");

    // Status bar
    statusBar()->showMessage("就绪");
}

void MainWindow::setupConnections() {
    // Toolbar buttons
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginClicked);

    // Connection → Decoder
    connect(m_conn, &ConnectionManager::rawFrameReceived,
            m_decoder, &ProtobufDecoder::processFrame);

    // Decoder → Router
    connect(m_decoder, &ProtobufDecoder::messageDecoded,
            m_router, &MessageRouter::route);

    // Decoder → Connection (ack)
    connect(m_decoder, &ProtobufDecoder::ackFrameReady,
            m_conn, &ConnectionManager::sendAck);

    // Router → EventLog
    connect(m_router, &MessageRouter::memberMessage, this,
        [this](const QString& uid, const QString& name) {
            LiveEvent e{LiveEventType::Member, uid, name, "进入了直播间", "", 0, 0, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onMember(uid, name);
        });

    connect(m_router, &MessageRouter::chatMessage, this,
        [this](const QString& uid, const QString& name, const QString& content) {
            LiveEvent e{LiveEventType::Chat, uid, name, content, "", 0, 0, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onChat(uid, name, content);
        });

    connect(m_router, &MessageRouter::giftMessage, this,
        [this](const QString& uid, const QString& name, const QString& giftName,
               int64_t diamonds, int64_t count) {
            QString desc = QString("送出 %1 x%2").arg(giftName).arg(count);
            if (diamonds > 0) desc += QString(" (%1钻石)").arg(diamonds);
            LiveEvent e{LiveEventType::Gift, uid, name, desc, giftName, diamonds, count, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onGift(uid, name, giftName, diamonds, count);
        });

    connect(m_router, &MessageRouter::likeMessage, this,
        [this](const QString& uid, const QString& name, int64_t count) {
            LiveEvent e{LiveEventType::Like, uid, name, QString("点赞 x%1").arg(count), "", 0, count, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onLike(uid, name, count);
        });

    connect(m_router, &MessageRouter::socialMessage, this,
        [this](const QString& uid, const QString& name) {
            LiveEvent e{LiveEventType::Social, uid, name, "关注了主播", "", 0, 0, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onFollow(uid, name);
        });

    connect(m_router, &MessageRouter::fansclubMessage, this,
        [this](const QString& uid, const QString& name, int32_t level) {
            LiveEvent e{LiveEventType::Fansclub, uid, name, "加入了粉丝团", "", 0, 0, QDateTime::currentDateTime()};
            m_eventLog->addEvent(e);
            m_autoResp->onFansclub(uid, name, level);
        });

    connect(m_router, &MessageRouter::controlMessage, this,
        [this](int32_t action) {
            if (action == 3) {
                m_eventLog->addLog("直播间已结束");
                m_conn->disconnect();
            }
        });

    connect(m_router, &MessageRouter::unknownMessage, this,
        [this](const QString& method) {
            m_eventLog->addLog(QString("未知消息类型: %1").arg(method));
        });

    // AutoResponseEngine → BarrageSender
    connect(m_autoResp, &AutoResponseEngine::sendBarrage,
            m_sender, &BarrageSender::sendBarrage);
    connect(m_autoResp, &AutoResponseEngine::logMessage,
            m_eventLog, &EventLogPanel::addLog);

    // TimerEngine → BarrageSender
    connect(m_timer, &TimerEngine::sendBarrage,
            m_sender, &BarrageSender::sendBarrage);
    connect(m_timer, &TimerEngine::sendLike,
            m_sender, &BarrageSender::sendLike);
    connect(m_timer, &TimerEngine::logMessage,
            m_eventLog, &EventLogPanel::addLog);

    // BarrageSender logs
    connect(m_sender, &BarrageSender::logMessage,
            m_eventLog, &EventLogPanel::addLog);

    // ConnectionManager logs
    connect(m_conn, &ConnectionManager::statusMessage,
            [this](const QString& msg) {
                m_statusLabel->setText(" " + msg);
                statusBar()->showMessage(msg);
            });

    connect(m_conn, &ConnectionManager::connected, this,
        [this](const RoomInfo& info) {
            m_connectBtn->setText("断开");
            m_statusLabel->setText(" 已连接");
            m_statusLabel->setStyleSheet("color: green;");
            m_sender->setRoomInfo(info.liveId, info.roomId);
            m_sender->setTtwid(m_conn->ttwid());
            // Start timers if configured
            if (m_configPanel->autoLikeEnabled()) {
                m_timer->startAutoLike(m_configPanel->autoLikeInterval());
            }
            m_timer->startAll();
        });

    connect(m_conn, &ConnectionManager::disconnected, this,
        [this]() {
            m_connectBtn->setText("连接");
            m_statusLabel->setText(" 已断开");
            m_statusLabel->setStyleSheet("color: red;");
            m_timer->stopAll();
        });

    connect(m_conn, &ConnectionManager::connectionError, this,
        [this](const QString& err) {
            QMessageBox::warning(this, "连接错误", err);
        });

    // Login
    connect(m_configPanel, &ConfigPanel::loginRequested, this, &MainWindow::onLoginClicked);
    connect(m_configPanel, &ConfigPanel::logoutRequested, this, &MainWindow::onLogoutClicked);
    connect(m_configPanel, &ConfigPanel::settingsChanged, this, &MainWindow::onSaveSettings);

    // Rule editor
    connect(m_ruleEditor, &RuleEditorPanel::rulesChanged, this, &MainWindow::onRulesChanged);
}

void MainWindow::onConnectClicked() {
    if (m_conn->isConnected()) {
        m_conn->disconnect();
        return;
    }

    QString liveId = m_roomIdEdit->text().trimmed();
    if (liveId.isEmpty()) {
        liveId = m_configPanel->liveId();
    }
    if (liveId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入直播间号");
        return;
    }

    QString apiKey = m_configPanel->apiKey();

    m_eventLog->clear();
    m_eventLog->addLog(QString("正在连接直播间: %1").arg(liveId));
    m_conn->connectToRoom(liveId, apiKey);
}

void MainWindow::onLoginClicked() {
    auto* dlg = new LoginDialog(this);
    connect(dlg, &LoginDialog::loginSuccess, this,
        [this, dlg](const QMap<QString, QString>& cookies) {
            m_sender->setCookies(cookies);
            m_configPanel->setLoginStatus(true);
            m_loginBtn->setEnabled(false);
            m_eventLog->addLog("登录成功");
            m_settings->saveCookies(cookies);

            // 创建一个隐藏 webview 用于发请求（共享登录 cookie 的同一 profile）
            if (!m_requestWebView) {
                m_requestWebView = new QWebEngineView(this);
                m_requestWebView->setVisible(false);
                // 使用默认 profile，与 LoginDialog 的 webview 共享 cookies
                auto* page = new QWebEnginePage(QWebEngineProfile::defaultProfile(), m_requestWebView);
                m_requestWebView->setPage(page);
                // 先加载 live.douyin.com 确保 cookie 生效
                m_requestWebView->load(QUrl("https://live.douyin.com/"));
            }
            m_sender->setWebView(m_requestWebView);
        });
    connect(dlg, &QDialog::finished, dlg, &QObject::deleteLater);
    dlg->show();
}

void MainWindow::onLogoutClicked() {
    m_sender->setCookies({});
    m_sender->setWebView(nullptr);
    m_configPanel->setLoginStatus(false);
    m_loginBtn->setEnabled(true);
    m_eventLog->addLog("已退出登录");
}

void MainWindow::onSaveSettings() {
    saveSettings();
    m_eventLog->setMaxEntries(m_configPanel->maxLogEntries());
    statusBar()->showMessage("设置已保存", 3000);
}

void MainWindow::onRulesChanged() {
    m_autoResp->setRules(m_ruleEditor->rules());
    m_settings->saveRules(m_ruleEditor->rules());
}

void MainWindow::updateViewerCount(int64_t count) {
    m_viewerLabel->setText(QString("观众: %1").arg(count));
}

void MainWindow::loadSettings() {
    m_configPanel->setLiveId(m_settings->lastLiveId());
    m_configPanel->setApiKey(m_settings->apiKey());
    m_configPanel->setMaxLogEntries(m_settings->maxLogEntries());
    m_configPanel->setAutoLikeInterval(m_settings->autoLikeInterval());
    m_configPanel->setAutoLikeEnabled(m_settings->autoLikeEnabled());
    m_roomIdEdit->setText(m_settings->lastLiveId());

    auto rules = m_settings->loadRules();
    m_ruleEditor->setRules(rules);
    m_autoResp->setRules(rules);

    auto timed = m_settings->loadTimedBarrages();
    for (const auto& b : timed) {
        m_timer->addTimedBarrage(b.first, b.second);
    }

    // Restore cookies
    auto cookies = m_settings->savedCookies();
    if (cookies.contains("sessionid") || cookies.contains("sessionid_ss")) {
        m_sender->setCookies(cookies);
        m_configPanel->setLoginStatus(true);
        m_loginBtn->setEnabled(false);

        // 创建隐藏 webview 用于发请求
        if (!m_requestWebView) {
            m_requestWebView = new QWebEngineView(this);
            m_requestWebView->setVisible(false);
            auto* page = new QWebEnginePage(QWebEngineProfile::defaultProfile(), m_requestWebView);
            m_requestWebView->setPage(page);
            m_requestWebView->load(QUrl("https://live.douyin.com/"));
        }
        m_sender->setWebView(m_requestWebView);
    }

    auto geometry = m_settings->windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    m_eventLog->setMaxEntries(m_configPanel->maxLogEntries());

    // Add default rules if none exist
    if (rules.isEmpty()) {
        m_ruleEditor->addRuleFromTable(AutoResponseRule::create(
            "欢迎进入", TriggerType::OnMemberEnter, "*",
            "欢迎 {user} 来到直播间！[欢迎]"));
        m_ruleEditor->addRuleFromTable(AutoResponseRule::create(
            "感谢关注", TriggerType::OnFollow, "*",
            "感谢 {user} 的关注！[比心]"));
        m_ruleEditor->addRuleFromTable(AutoResponseRule::create(
            "感谢礼物", TriggerType::OnGift, "*",
            "感谢 {user} 送的{gift}！[玫瑰]"));
        m_ruleEditor->addRuleFromTable(AutoResponseRule::create(
            "感谢入团", TriggerType::OnFansclubJoin, "*",
            "欢迎 {user} 加入粉丝团！[666]"));
        onRulesChanged();
    }
}

void MainWindow::saveSettings() {
    m_settings->setLastLiveId(m_configPanel->liveId());
    m_settings->setApiKey(m_configPanel->apiKey());
    m_settings->setMaxLogEntries(m_configPanel->maxLogEntries());
    m_settings->setAutoLikeInterval(m_configPanel->autoLikeInterval());
    m_settings->setAutoLikeEnabled(m_configPanel->autoLikeEnabled());
    m_settings->saveRules(m_ruleEditor->rules());
    m_settings->setWindowGeometry(saveGeometry());
}
