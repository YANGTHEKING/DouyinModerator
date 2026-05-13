#pragma once
#include <QDialog>
#include <QWebEngineView>
#include <QMap>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

    QMap<QString, QString> cookies() const;

signals:
    void loginSuccess(const QMap<QString, QString>& cookies);

private:
    QWebEngineView* m_webView;
    QMap<QString, QString> m_cookies;
};
