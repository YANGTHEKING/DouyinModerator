#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QNetworkCookie>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("登录抖音");
    resize(500, 600);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    auto* cookieStore = m_webView->page()->profile()->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this,
        [this](const QNetworkCookie& cookie) {
            QString name = QString::fromUtf8(cookie.name());
            QString value = QString::fromUtf8(cookie.value());
            m_cookies[name] = value;
        });

    connect(m_webView, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
        if (url.toString().contains("live.douyin.com") &&
            !url.toString().contains("login")) {
            if (m_cookies.contains("sessionid") || m_cookies.contains("sessionid_ss")) {
                emit loginSuccess(m_cookies);
                accept();
            }
        }
    });

    m_webView->load(QUrl("https://live.douyin.com/"));
}

QMap<QString, QString> LoginDialog::cookies() const { return m_cookies; }
