#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QProgressBar>
#include <QGraphicsOpacityEffect>
#include <QTimer>

class LoginWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    virtual ~LoginWindow();

signals:
    void loginSuccessful(const QString& username, const QString& role, const QString& fullName);

private slots:
    void handleLogin();
    void togglePasswordVisibility(bool checked);
    void handleForgotPassword();
    void handleGuestMode();
    void handleSettings();
    void advanceLoading();

private:
    QLineEdit* m_usernameInput = nullptr;
    QLineEdit* m_passwordInput = nullptr;
    QComboBox* m_roleCombo = nullptr;
    QCheckBox* m_rememberCheck = nullptr;
    QCheckBox* m_showPasswordCheck = nullptr;
    QPushButton* m_loginButton = nullptr;
    QPushButton* m_guestButton = nullptr;
    QLabel* m_errorLabel = nullptr;
    QProgressBar* m_loadingBar = nullptr;
    QLabel* m_forgotLabel = nullptr;
    QLabel* m_connectionLabel = nullptr;
    QPushButton* m_settingsButton = nullptr;

    QGraphicsOpacityEffect* m_opacityEffect = nullptr;
    QTimer* m_loadingTimer = nullptr;
    bool m_authenticationInProgress = false;
    QString m_pendingUsername;
    QString m_pendingRole;
    QString m_pendingFullName;

    void setupUi();
    void playIntroAnimation();
    void showLoading(bool loading);
    void beginSession(const QString& username, const QString& role, const QString& fullName,
                      const QString& loadingText);
};

#endif // LOGINWINDOW_H
