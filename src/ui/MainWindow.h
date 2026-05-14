#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QToolBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QWebEngineView>

class ConnectionManager;
class ProtobufDecoder;
class MessageRouter;
class AutoResponseEngine;
class TimerEngine;
class BarrageSender;
class LoginManager;
class EmojiResolver;
class AppSettings;
class EventLogPanel;
class RuleEditorPanel;
class ConfigPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onLoginClicked();
    void onLogoutClicked();
    void onSaveSettings();
    void onRulesChanged();
    void updateViewerCount(int64_t count);

private:
    void setupUi();
    void setupConnections();
    void loadSettings();
    void saveSettings();

    // Core components
    ConnectionManager* m_conn;
    ProtobufDecoder* m_decoder;
    MessageRouter* m_router;
    AutoResponseEngine* m_autoResp;
    TimerEngine* m_timer;
    BarrageSender* m_sender;
    LoginManager* m_login;
    EmojiResolver* m_emoji;
    AppSettings* m_settings;

    // UI components
    QTabWidget* m_tabs;
    EventLogPanel* m_eventLog;
    RuleEditorPanel* m_ruleEditor;
    ConfigPanel* m_configPanel;

    // Toolbar
    QPushButton* m_connectBtn;
    QLineEdit* m_roomIdEdit;
    QPushButton* m_loginBtn;
    QLabel* m_statusLabel;
    QLabel* m_viewerLabel;

    // 用于发请求的隐藏 webview（共享 login cookies）
    QWebEngineView* m_requestWebView = nullptr;
};
