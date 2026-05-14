#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QNetworkCookie>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineSettings>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("登录抖音 — 扫码或输入账号密码登录后点击「确认登录」");
    resize(520, 680);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_webView = new QWebEngineView(this);
    // 使用自定义页面禁止弹窗
    auto* page = new NoPopupWebEnginePage(QWebEngineProfile::defaultProfile(), m_webView);
    m_webView->setPage(page);
    // 禁止 JavaScript 打开新窗口
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    layout->addWidget(m_webView);

    // 底部状态栏
    auto* bottomWidget = new QWidget(this);
    bottomWidget->setFixedHeight(50);
    auto* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(10, 5, 10, 5);

    m_statusLabel = new QLabel("请在上方页面登录抖音账号", bottomWidget);
    m_confirmBtn = new QPushButton("确认登录", bottomWidget);
    m_confirmBtn->setFixedWidth(100);
    m_confirmBtn->setEnabled(false);

    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_confirmBtn);
    layout->addWidget(bottomWidget);

    // Cookie 检查定时器（每 2 秒检查一次）
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(2000);
    connect(m_checkTimer, &QTimer::timeout, this, &LoginDialog::checkLoginStatus);
    m_checkTimer->start();

    // cookieAdded 信号也连接（作为补充）
    auto* cookieStore = m_webView->page()->profile()->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this,
        [this](const QNetworkCookie& cookie) {
            QString name = QString::fromUtf8(cookie.name());
            QString value = QString::fromUtf8(cookie.value());
            m_cookies[name] = value;
        });

    // 确认登录按钮
    connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
        checkLoginStatus();
        if (m_cookies.contains("sessionid") || m_cookies.contains("sessionid_ss")) {
            emit loginSuccess(m_cookies);
            accept();
        } else {
            m_statusLabel->setText("未检测到登录状态，请先在页面中完成登录");
            m_statusLabel->setStyleSheet("color: red;");
        }
    });

    m_webView->load(QUrl("https://live.douyin.com/"));
}

void LoginDialog::checkLoginStatus() {
    auto* cookieStore = m_webView->page()->profile()->cookieStore();
    cookieStore->loadAllCookies();

    // 延迟检查（loadAllCookies 是异步的）
    QTimer::singleShot(500, this, [this]() {
        // 通过 cookieAdded 信号收集到的 cookie 已经存在 m_cookies 中
        bool hasSession = m_cookies.contains("sessionid") || m_cookies.contains("sessionid_ss");
        if (hasSession) {
            m_statusLabel->setText("已检测到登录！点击「确认登录」完成");
            m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
            m_confirmBtn->setEnabled(true);
            m_confirmBtn->setStyleSheet("background-color: #2ecc71; color: white; font-weight: bold;");
        } else {
            // 只有在按钮不可用时才更新状态，避免覆盖用户看到的提示
            if (!m_confirmBtn->isEnabled()) {
                m_statusLabel->setText("等待登录中...");
            }
        }
    });
}

QMap<QString, QString> LoginDialog::cookies() const { return m_cookies; }
