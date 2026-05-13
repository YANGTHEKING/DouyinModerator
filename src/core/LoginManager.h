#pragma once
#include <QObject>
#include <QMap>
#include <QNetworkCookie>
#include <QWebEngineView>
#include <QWebEngineProfile>

class LoginManager : public QObject {
    Q_OBJECT
public:
    explicit LoginManager(QObject* parent = nullptr);
    ~LoginManager();

    void showLogin(QWidget* parent = nullptr);
    void logout();
    bool isLoggedIn() const;
    QMap<QString, QString> cookies() const;

signals:
    void loginSuccess(const QMap<QString, QString>& cookies);
    void loginFailed(const QString& reason);
    void loginDialogClosed();

private slots:
    void onCookiesLoaded(const QList<QNetworkCookie>& cookies);

private:
    QWebEngineView* m_webView = nullptr;
    QDialog* m_dialog = nullptr;
    QMap<QString, QString> m_cookies;
    bool m_loggedIn = false;
};
