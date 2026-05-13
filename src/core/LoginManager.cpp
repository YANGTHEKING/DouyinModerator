#include "LoginManager.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>

LoginManager::LoginManager(QObject* parent) : QObject(parent) {}

LoginManager::~LoginManager() {
    if (m_dialog) m_dialog->deleteLater();
    if (m_webView) m_webView->deleteLater();
}

void LoginManager::showLogin(QWidget* parent) {
    if (m_dialog) {
        m_dialog->raise();
        m_dialog->activateWindow();
        return;
    }

    m_dialog = new QDialog(parent);
    m_dialog->setWindowTitle("登录抖音");
    m_dialog->resize(500, 600);

    auto* layout = new QVBoxLayout(m_dialog);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(m_dialog);
    layout->addWidget(m_webView);

    // Monitor cookie changes for sessionid
    auto* cookieStore = m_webView->page()->profile()->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this,
        [this](const QNetworkCookie& cookie) {
            if (cookie.name() == "sessionid" || cookie.name() == "sessionid_ss") {
                m_cookies[QString::fromUtf8(cookie.name())] = QString::fromUtf8(cookie.value());
            }
            if (cookie.name() == "ttwid") {
                m_cookies["ttwid"] = QString::fromUtf8(cookie.value());
            }
        });

    // Also check URL changes for login detection
    connect(m_webView, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
        QString urlStr = url.toString();
        if (urlStr.contains("live.douyin.com") && !urlStr.contains("login")) {
            // Likely logged in, collect cookies
            auto* cookieStore = m_webView->page()->profile()->cookieStore();
            cookieStore->loadAllCookies();
        }
    });

    connect(m_dialog, &QDialog::finished, this, [this](int result) {
        if (m_cookies.contains("sessionid") || m_cookies.contains("sessionid_ss")) {
            m_loggedIn = true;
            emit loginSuccess(m_cookies);
        }
        m_dialog->deleteLater();
        m_dialog = nullptr;
        m_webView->deleteLater();
        m_webView = nullptr;
        emit loginDialogClosed();
    });

    m_webView->load(QUrl("https://live.douyin.com/"));
    m_dialog->show();
}

void LoginManager::logout() {
    m_cookies.clear();
    m_loggedIn = false;
}

bool LoginManager::isLoggedIn() const { return m_loggedIn; }

QMap<QString, QString> LoginManager::cookies() const { return m_cookies; }

void LoginManager::onCookiesLoaded(const QList<QNetworkCookie>& cookies) {
    for (const auto& c : cookies) {
        QString name = QString::fromUtf8(c.name());
        QString value = QString::fromUtf8(c.value());
        if (name == "sessionid" || name == "sessionid_ss" || name == "ttwid") {
            m_cookies[name] = value;
        }
    }
}
