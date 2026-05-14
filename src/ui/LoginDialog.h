#pragma once
#include <QDialog>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QMap>
#include <QTimer>
#include <QPushButton>
#include <QLabel>

// 自定义页面：禁止弹窗和外部协议跳转（防止 bitbrowser 等提示）
class NoPopupWebEnginePage : public QWebEnginePage {
    Q_OBJECT
public:
    using QWebEnginePage::QWebEnginePage;
protected:
    QWebEnginePage* createWindow(QWebEnginePage::WebWindowType) override {
        return nullptr; // 阻止所有弹窗
    }
};

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

    QMap<QString, QString> cookies() const;
    QWebEngineView* webView() const { return m_webView; }

signals:
    void loginSuccess(const QMap<QString, QString>& cookies);

private:
    void checkLoginStatus();
    void refreshAllCookies();

    QWebEngineView* m_webView;
    QPushButton* m_confirmBtn;
    QLabel* m_statusLabel;
    QTimer* m_checkTimer;
    QMap<QString, QString> m_cookies;
};
