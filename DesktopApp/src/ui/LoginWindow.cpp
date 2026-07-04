#include "LoginWindow.h"
#include "../core/DatabaseManager.h"
#include "../core/VoiceAssistant.h"
#include "../core/WorkflowEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>

LoginWindow::LoginWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    m_loadingTimer = new QTimer(this);
    m_loadingTimer->setInterval(35);
    connect(m_loadingTimer, &QTimer::timeout, this, &LoginWindow::advanceLoading);
    playIntroAnimation();
}

LoginWindow::~LoginWindow() {}

void LoginWindow::setupUi() {
    setMinimumSize(460, 650);
    resize(780, 780);
    setWindowTitle("AegisCare AI - Hospital Workstation Portal");
    
    // Core dark gradient background stylesheet
    setStyleSheet(
        "QMainWindow { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #080D14, stop:1 #111A24); }"
        "QLabel { color: #E1E6EB; font-family: 'Inter', sans-serif; }"
        "QLineEdit {"
        "  background-color: rgba(255, 255, 255, 0.04);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  color: #FFFFFF;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #00A8FF;"
        "  background-color: rgba(0, 168, 255, 0.05);"
        "}"
        "QComboBox {"
        "  background-color: rgba(255, 255, 255, 0.04);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  color: #E1E6EB;"
        "  font-size: 13px;"
        "}"
        "QComboBox:focus {"
        "  border: 1px solid #00A8FF;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "}"
        "QCheckBox { color: #A1AAB3; font-size: 12px; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 3px;"
        "  background-color: rgba(255, 255, 255, 0.04);"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #00A8FF;"
        "  border: 1px solid #00A8FF;"
        "}"
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00A8FF, stop:1 #007ACC);"
        "  color: #FFFFFF;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 10px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1AB2FF, stop:1 #0088E6);"
        "}"
        "QPushButton:pressed {"
        "  background: #0066AA;"
        "}"
        "QProgressBar {"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 4px;"
        "  background-color: rgba(0, 0, 0, 80);"
        "  text-align: center;"
        "  color: #FFFFFF;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #00A8FF;"
        "  border-radius: 3px;"
        "}"
    );

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Glassmorphism Card Frame
    QWidget* loginCard = new QWidget(this);
    loginCard->setObjectName("LoginCard");
    loginCard->setStyleSheet(
        "QWidget#LoginCard {"
        "  background-color: rgba(255, 255, 255, 0.04);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 16px;"
        "}"
    );
    loginCard->setMinimumWidth(320);
    loginCard->setMaximumWidth(430);
    loginCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    QGraphicsDropShadowEffect* cardShadow = new QGraphicsDropShadowEffect(this);
    cardShadow->setBlurRadius(30);
    cardShadow->setColor(QColor(0, 0, 0, 150));
    cardShadow->setOffset(0, 8);
    loginCard->setGraphicsEffect(cardShadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(loginCard);
    cardLayout->setContentsMargins(30, 40, 30, 40);
    cardLayout->setSpacing(16);

    // Title / Brand headers
    QVBoxLayout* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);
    headerLayout->setAlignment(Qt::AlignCenter);

    QLabel* iconLabel = new QLabel(this);
    iconLabel->setText("🛡️"); // High-tech shield shield-health icon placeholder
    iconLabel->setStyleSheet("font-size: 38px; margin-bottom: 5px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("AEGISCARE AI", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 800; letter-spacing: 2px; color: #FFFFFF;");
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel);

    QLabel* subLabel = new QLabel("PORTABLE CLINICAL WORKSTATION", this);
    subLabel->setStyleSheet("font-size: 9px; font-weight: 700; color: #00A8FF; letter-spacing: 1px;");
    subLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(subLabel);
    
    cardLayout->addLayout(headerLayout);
    cardLayout->addSpacing(10);

    // Inputs
    QVBoxLayout* inputsLayout = new QVBoxLayout();
    inputsLayout->setSpacing(12);

    // Role Selection
    QLabel* roleTitle = new QLabel("Select Workstation Role", this);
    roleTitle->setStyleSheet("font-size: 11px; color: #A1AAB3; font-weight: bold;");
    m_roleCombo = new QComboBox(this);
    m_roleCombo->addItems({"Doctor", "Administrator"});
    inputsLayout->addWidget(roleTitle);
    inputsLayout->addWidget(m_roleCombo);

    // Username Input
    QLabel* userTitle = new QLabel("Username / Operator ID", this);
    userTitle->setStyleSheet("font-size: 11px; color: #A1AAB3; font-weight: bold;");
    m_usernameInput = new QLineEdit(this);
    m_usernameInput->setPlaceholderText("Enter operator username");
    m_usernameInput->setText("soham");
    inputsLayout->addWidget(userTitle);
    inputsLayout->addWidget(m_usernameInput);

    // Password Input
    QLabel* passTitle = new QLabel("Access Password", this);
    passTitle->setStyleSheet("font-size: 11px; color: #A1AAB3; font-weight: bold;");
    m_passwordInput = new QLineEdit(this);
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setPlaceholderText("••••••••");
    m_passwordInput->setText("soham");
    inputsLayout->addWidget(passTitle);
    inputsLayout->addWidget(m_passwordInput);

    cardLayout->addLayout(inputsLayout);

    // Toggles layout
    QHBoxLayout* togglesLayout = new QHBoxLayout();
    m_rememberCheck = new QCheckBox("Remember", this);
    m_showPasswordCheck = new QCheckBox("Show", this);
    togglesLayout->addWidget(m_rememberCheck);
    togglesLayout->addWidget(m_showPasswordCheck);
    cardLayout->addLayout(togglesLayout);

    // Buttons
    QVBoxLayout* buttonsLayout = new QVBoxLayout();
    buttonsLayout->setSpacing(8);

    m_loginButton = new QPushButton("INITIALIZE SESSION", this);
    buttonsLayout->addWidget(m_loginButton);

    m_settingsButton = new QPushButton("WORKSTATION INFORMATION", this);
    m_settingsButton->setObjectName("SecondaryButton");
    m_settingsButton->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid rgba(255,255,255,0.10); color: #A1AAB3; }"
        "QPushButton:hover { border-color: #00A8FF; color: #FFFFFF; }"
    );
    buttonsLayout->addWidget(m_settingsButton);
    cardLayout->addLayout(buttonsLayout);

    // Info link forgot password
    m_forgotLabel = new QLabel("Forgot Password — Disabled in Demonstration Mode", this);
    m_forgotLabel->setAlignment(Qt::AlignCenter);
    m_forgotLabel->setStyleSheet("font-size: 11px; color: #6F7C89; margin-top: 5px;");
    m_forgotLabel->setToolTip("Password reset is disabled in this educational demonstration.");
    cardLayout->addWidget(m_forgotLabel);

    m_connectionLabel = new QLabel("● DATABASE READY  •  HARDWARE STARTS AFTER SIGN-IN", this);
    m_connectionLabel->setAlignment(Qt::AlignCenter);
    m_connectionLabel->setWordWrap(true);
    m_connectionLabel->setStyleSheet("font-size: 10px; color: #2ECC71; font-weight: 700;");
    cardLayout->addWidget(m_connectionLabel);

    // Error Label & Loading Indicator
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setStyleSheet("color: #E74C3C; font-size: 12px; font-weight: bold;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_errorLabel);

    m_loadingBar = new QProgressBar(this);
    m_loadingBar->setRange(0, 100);
    m_loadingBar->setValue(0);
    m_loadingBar->setFormat("Mounting File Systems... %p%");
    cardLayout->addWidget(m_loadingBar);
    showLoading(false);

    mainLayout->addWidget(loginCard);

    // Signal mappings
    connect(m_loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(m_settingsButton, &QPushButton::clicked, this, &LoginWindow::handleSettings);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &LoginWindow::togglePasswordVisibility);
    connect(m_usernameInput, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);
    connect(m_passwordInput, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);

    setTabOrder(m_roleCombo, m_usernameInput);
    setTabOrder(m_usernameInput, m_passwordInput);
    setTabOrder(m_passwordInput, m_rememberCheck);
    setTabOrder(m_rememberCheck, m_showPasswordCheck);
    setTabOrder(m_showPasswordCheck, m_loginButton);

    QSettings settings;
    const QString rememberedUser = settings.value("login/username").toString();
    if (!rememberedUser.isEmpty()) {
        m_usernameInput->setText(rememberedUser);
        m_roleCombo->setCurrentText(settings.value("login/role", "Doctor").toString());
        m_rememberCheck->setChecked(true);
    }
}

void LoginWindow::playIntroAnimation() {
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    centralWidget()->setGraphicsEffect(m_opacityEffect);

    QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    opacityAnim->setDuration(800);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutQuad);
    opacityAnim->start();
}

void LoginWindow::showLoading(bool loading) {
    m_loadingBar->setVisible(loading);
    m_loginButton->setEnabled(!loading);
    if (m_guestButton) m_guestButton->setEnabled(!loading);
    m_usernameInput->setEnabled(!loading);
    m_passwordInput->setEnabled(!loading);
    m_roleCombo->setEnabled(!loading);
    m_rememberCheck->setEnabled(!loading);
    m_showPasswordCheck->setEnabled(!loading);
    m_settingsButton->setEnabled(!loading);
    if (loading) m_loadingBar->setValue(0);
}

void LoginWindow::togglePasswordVisibility(bool checked) {
    m_passwordInput->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void LoginWindow::handleForgotPassword() {
    QMessageBox::information(this, "AegisCare Workstation Assistance", 
                             "For hospital workstation security compliance, password resets must be authorized by your regional IT administrator. Please contact IT support with your Operator ID.");
}

void LoginWindow::handleGuestMode() {
    beginSession("guest", "Guest Demo Mode", "Guest Clinician",
                 "Initializing Educational Demo... %p%");
}

void LoginWindow::handleLogin() {
    if (m_authenticationInProgress) return;
    m_errorLabel->setText("");
    QString username = m_usernameInput->text().trimmed();
    QString password = m_passwordInput->text();
    QString selectedRole = m_roleCombo->currentText();

    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("Please enter username and password.");
        return;
    }

    User user = DatabaseManager::instance().authenticateUser(username, password);
    if (user.username.isEmpty()) {
        m_errorLabel->setText("Invalid database credentials.");
        return;
    }

    if (user.role != selectedRole) {
        m_errorLabel->setText(QString("Assigned role is %1, not %2.").arg(user.role).arg(selectedRole));
        return;
    }

    QSettings settings;
    if (m_rememberCheck->isChecked()) {
        settings.setValue("login/username", user.username);
        settings.setValue("login/role", user.role);
    } else {
        settings.remove("login/username");
        settings.remove("login/role");
    }
    beginSession(user.username, user.role, user.fullName, "Verifying Session... %p%");
}

void LoginWindow::handleSettings() {
    QMessageBox::information(
        this, "Workstation Information",
        "AegisCare AI\n\nDemonstration Mode • Educational Prototype\n"
        "Local SQLite registry: Ready\nArduino and ESP32 links initialize after sign-in.\n\n"
        "This software does not diagnose disease and is not a medical device.");
}

void LoginWindow::beginSession(const QString& username, const QString& role,
                               const QString& fullName, const QString& loadingText) {
    if (m_authenticationInProgress) return;
    m_authenticationInProgress = true;
    m_pendingUsername = username;
    m_pendingRole = role;
    m_pendingFullName = fullName;
    showLoading(true);
    m_loadingBar->setFormat(loadingText);
    m_loadingTimer->start();
}

void LoginWindow::advanceLoading() {
    const int value = qMin(100, m_loadingBar->value() + 8);
    m_loadingBar->setValue(value);
    if (value == 40) m_loadingBar->setFormat("Loading Local Registry... %p%");
    else if (value == 72) m_loadingBar->setFormat("Preparing Workstation UI... %p%");
    if (value < 100) return;

    m_loadingTimer->stop();
    WorkflowEngine::instance().nextStage();
    emit loginSuccessful(m_pendingUsername, m_pendingRole, m_pendingFullName);
    close();
}
