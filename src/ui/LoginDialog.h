#pragma once
#include <QDialog>
#include <QWebEngineView>
#include <QMap>
#include <QTimer>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

    QMap<QString, QString> cookies() const;

signals:
    void loginSuccess(const QMap<QString, QString>& cookies);

private:
    void checkLoginStatus();

    QWebEngineView* m_webView;
    QPushButton* m_confirmBtn;
    QLabel* m_statusLabel;
    QTimer* m_checkTimer;
    QMap<QString, QString> m_cookies;
};
