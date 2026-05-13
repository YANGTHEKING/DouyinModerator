#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

class ConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit ConfigPanel(QWidget* parent = nullptr);

    void setLiveId(const QString& id);
    QString liveId() const;
    void setApiKey(const QString& key);
    QString apiKey() const;
    void setMaxLogEntries(int count);
    int maxLogEntries() const;
    void setLoginStatus(bool loggedIn, const QString& username = "");
    void setAutoLikeInterval(int seconds);
    int autoLikeInterval() const;
    void setAutoLikeEnabled(bool enabled);
    bool autoLikeEnabled() const;

signals:
    void loginRequested();
    void logoutRequested();
    void settingsChanged();

private:
    QLineEdit* m_liveIdEdit;
    QLineEdit* m_apiKeyEdit;
    QSpinBox* m_maxLogSpin;
    QLabel* m_loginStatusLabel;
    QPushButton* m_loginBtn;
    QPushButton* m_logoutBtn;
    QSpinBox* m_autoLikeSpin;
    QCheckBox* m_autoLikeCheck;
    QPushButton* m_saveBtn;
};
