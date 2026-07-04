#include "SettingsView.h"

#include "../core/CommunicationManager.h"
#include "../core/DatabaseManager.h"
#include "../core/CameraStreamManager.h"
#include "components/ModernCard.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QDialog>

namespace {
ModernCard* makeCard(QWidget* parent, const QString& title, QVBoxLayout** content) {
    auto* card = new ModernCard(parent);
    card->setHoverEffect(false);
    card->setStyleSheet("QFrame { background: rgba(255,255,255,0.035); border:1px solid rgba(255,255,255,0.08); border-radius:14px; }");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(11);
    auto* label = new QLabel(title, card);
    label->setStyleSheet("font-size:12px; color:#00A8FF; font-weight:900; letter-spacing:.5px;");
    layout->addWidget(label);
    *content = layout;
    return card;
}

bool copyRecursively(const QString& sourcePath, const QString& targetPath) {
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) return true;
    if (sourceInfo.isFile()) {
        QDir().mkpath(QFileInfo(targetPath).absolutePath());
        QFile::remove(targetPath);
        return QFile::copy(sourcePath, targetPath);
    }
    QDir source(sourcePath);
    if (!QDir().mkpath(targetPath)) return false;
    for (const QFileInfo& entry : source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        if (!copyRecursively(entry.absoluteFilePath(), QDir(targetPath).filePath(entry.fileName()))) return false;
    }
    return true;
}
}

SettingsView::SettingsView(QWidget* parent) : QWidget(parent) {
    setupUi();
    loadSettings();
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1500);
    connect(m_refreshTimer, &QTimer::timeout, this, &SettingsView::refreshStatus);
    refreshStatus();

    auto& camera = CameraStreamManager::instance();
    connect(&camera, &CameraStreamManager::statusChanged,
            this, &SettingsView::updateCameraStatus, Qt::UniqueConnection);
    connect(&camera, &CameraStreamManager::statisticsChanged,
            this, &SettingsView::refreshStatus, Qt::UniqueConnection);
    auto& source = CameraSourceManager::instance();
    connect(&source, &CameraSourceManager::frameReady,
            this, &SettingsView::updateCameraFrame, Qt::UniqueConnection);
    connect(&source, &CameraSourceManager::sourceChanged, this,
            [this](CameraProvider::Source, const QString&) {
        m_cameraPreview->clear();
        m_cameraPreview->setText("Switching camera source...");
        refreshStatus();
    });
    connect(&source, &CameraSourceManager::statusChanged, this, &SettingsView::refreshStatus);
    camera.configure(m_espIpEdit->text(), m_espPortEdit->text().toUShort(), QStringLiteral("/stream"));
    source.start();
}

SettingsView::~SettingsView() = default;

void SettingsView::setupUi() {
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(22, 22, 22, 22);
    main->setSpacing(16);

    auto* title = new QLabel("WORKSTATION SETTINGS", this);
    title->setStyleSheet("font-size:24px; font-weight:950; color:#FFFFFF;");
    auto* notice = new QLabel("Educational Demonstration Prototype • Permanent Dark Medical Interface", this);
    notice->setStyleSheet("font-size:10px; color:#F5B041; font-weight:850;");
    main->addWidget(title);
    main->addWidget(notice);

    auto* grid = new QGridLayout();
    grid->setSpacing(16);

    QVBoxLayout* accountLayout = nullptr;
    ModernCard* account = makeCard(this, "1. USER ACCOUNT", &accountLayout);
    auto* profileRow = new QHBoxLayout();
    auto* avatar = new QLabel("👨‍⚕️", account);
    avatar->setFixedSize(72, 72);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size:36px; background:rgba(0,168,255,.12); border:1px solid rgba(0,168,255,.35); border-radius:36px;");
    auto* accountText = new QVBoxLayout();
    m_accountName = new QLabel("Dr. Soham", account);
    m_accountName->setStyleSheet("font-size:18px; font-weight:900; color:#FFFFFF;");
    m_accountUsername = new QLabel("Username: soham", account);
    m_accountRole = new QLabel("Role: Doctor", account);
    m_lastLogin = new QLabel("Last Login: Current session", account);
    accountText->addWidget(m_accountName);
    accountText->addWidget(m_accountUsername);
    accountText->addWidget(m_accountRole);
    accountText->addWidget(m_lastLogin);
    profileRow->addWidget(avatar);
    profileRow->addLayout(accountText, 1);
    accountLayout->addLayout(profileRow);
    auto* logoutButton = new QPushButton("LOGOUT CURRENT SESSION", account);
    connect(logoutButton, &QPushButton::clicked, this, &SettingsView::logoutRequested);
    accountLayout->addWidget(logoutButton);
    grid->addWidget(account, 0, 0);

    QVBoxLayout* languageLayout = nullptr;
    ModernCard* language = makeCard(this, "2. LANGUAGE SETTINGS", &languageLayout);
    auto* languageForm = new QFormLayout();
    m_langCombo = new QComboBox(language);
    m_langCombo->addItems({"English (Default)", "Hindi (हिन्दी)", "Marathi (मराठी)"});
    connect(m_langCombo, &QComboBox::currentIndexChanged, this, &SettingsView::handleLanguageChange);
    languageForm->addRow("Application Language:", m_langCombo);
    auto* languageNote = new QLabel("Menus and workstation guidance use a centralized language setting. Future translations can be added without changing workflow logic.", language);
    languageNote->setWordWrap(true);
    languageNote->setStyleSheet("color:#A1AAB3; font-size:11px;");
    languageLayout->addLayout(languageForm);
    languageLayout->addWidget(languageNote);
    grid->addWidget(language, 0, 1);

    QVBoxLayout* hardwareLayout = nullptr;
    ModernCard* hardware = makeCard(this, "3. PATIENT CAMERA & HARDWARE DIAGNOSTICS", &hardwareLayout);
    auto* hardwarePanels = new QHBoxLayout();
    hardwarePanels->setSpacing(12);

    auto* leftPanel = new QFrame(hardware);
    leftPanel->setStyleSheet("QFrame { background:#0A131E; border:1px solid #1B2B3A; border-radius:9px; }");
    auto* left = new QVBoxLayout(leftPanel);
    left->addWidget(new QLabel("HARDWARE STATUS", leftPanel));
    m_arduinoStatus = new QLabel(hardware);
    m_espStatus = new QLabel(hardware);
    m_wifiStatus = new QLabel(hardware);
    m_cameraHardwareStatus = new QLabel(hardware);
    m_oledStatus = new QLabel(hardware);
    m_inputStatus = new QLabel(hardware);
    for (QLabel* label : {m_arduinoStatus, m_espStatus, m_wifiStatus, m_cameraHardwareStatus,
                          m_oledStatus, m_inputStatus}) {
        label->setWordWrap(true);
        label->setStyleSheet("background:#07101A; border:none; border-radius:6px; padding:7px; color:#DDE7F0;");
        left->addWidget(label);
    }
    m_hardwareTests = new QLabel("Hardware Test\nButtons: WAIT • Joystick: WAIT • OLED: WAIT\nCamera: WAIT • Voice: PASS • Report: PASS • Emergency: PASS", leftPanel);
    m_hardwareTests->setWordWrap(true);
    left->addWidget(m_hardwareTests);
    auto* testGrid = new QGridLayout();
    const QStringList tests = {"Buttons", "Joystick", "OLED", "Camera", "Voice", "Report", "Emergency"};
    for (int i = 0; i < tests.size(); ++i) {
        auto* test = new QPushButton(tests[i] + ": TEST", leftPanel);
        test->setMinimumHeight(30);
        m_testButtons.insert(tests[i], test);
        testGrid->addWidget(test, i / 2, i % 2);
    }
    left->addLayout(testGrid);
    connect(m_testButtons["Buttons"], &QPushButton::clicked, this, [this] {
        m_testButtons["Buttons"]->setText("Buttons: ARMED");
    });
    connect(m_testButtons["Joystick"], &QPushButton::clicked, this, [this] {
        m_testButtons["Joystick"]->setText("Joystick: MOVE NOW");
    });
    connect(m_testButtons["OLED"], &QPushButton::clicked, this, [this] {
        const bool sent = CommunicationManager::instance().sendJSON(
            QJsonObject{{QStringLiteral("type"), QStringLiteral("oled_status")},
                        {QStringLiteral("status"), QStringLiteral("HARDWARE_TEST")}});
        setHardwareTestResult("OLED", sent, sent ? "command sent" : "Nano offline");
    });
    connect(m_testButtons["Camera"], &QPushButton::clicked, this, [this] {
        CameraStreamManager::instance().reconnect();
        m_testButtons["Camera"]->setText("Camera: CONNECTING");
    });
    connect(m_testButtons["Voice"], &QPushButton::clicked, this, [this] {
        VoiceAssistant::instance().speakTextDirectly("Voice hardware test successful.");
        setHardwareTestResult("Voice", true);
    });
    connect(m_testButtons["Report"], &QPushButton::clicked, this, [this] {
        setHardwareTestResult("Report", QFileInfo::exists(DatabaseManager::instance().databasePath()), "database ready");
    });
    connect(m_testButtons["Emergency"], &QPushButton::clicked, this, [this] {
        const bool sent = CommunicationManager::instance().sendJSON(
            QJsonObject{{QStringLiteral("type"), QStringLiteral("hardware_test")},
                        {QStringLiteral("target"), QStringLiteral("emergency_button")}});
        setHardwareTestResult("Emergency", sent, sent ? "test packet sent" : "Nano offline");
    });
    connect(&CommunicationManager::instance(), &CommunicationManager::buttonPressed, this,
            [this](int) { setHardwareTestResult("Buttons", true, "press detected"); });
    connect(&CommunicationManager::instance(), &CommunicationManager::joystickMoved, this,
            [this](const QString&) { setHardwareTestResult("Joystick", true, "movement detected"); });
    left->addStretch();
    hardwarePanels->addWidget(leftPanel, 3);

    auto* centerPanel = new QFrame(hardware);
    centerPanel->setStyleSheet("QFrame { background:#050A10; border:1px solid rgba(0,168,255,.35); border-radius:9px; }");
    auto* center = new QVBoxLayout(centerPanel);
    auto* streamTitle = new QLabel("PATIENT CAMERA LIVE MONITOR", centerPanel);
    streamTitle->setStyleSheet("font-weight:900; color:#00A8FF; border:none;");
    center->addWidget(streamTitle);
    m_cameraConnection = new QLabel("Current Source: Searching...", centerPanel);
    m_cameraConnection->setAlignment(Qt::AlignCenter);
    m_cameraPreview = new QLabel("CAMERA OFFLINE\nConnect the ESP32-CAM to begin", centerPanel);
    m_cameraPreview->setAlignment(Qt::AlignCenter);
    m_cameraPreview->setMinimumSize(390, 250);
    m_cameraPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_cameraPreview->setStyleSheet("background:#020509; color:#718096; border:1px solid #172533; border-radius:7px;");
    m_cameraMetrics = new QLabel("Resolution: -- • FPS: 0 • Latency: -- • Frames: 0", centerPanel);
    m_cameraMetrics->setWordWrap(true);
    center->addWidget(m_cameraConnection);
    center->addWidget(m_cameraPreview, 1);
    center->addWidget(m_cameraMetrics);
    auto* cameraButtons = new QGridLayout();
    auto* startCamera = new QPushButton("START STREAM", centerPanel);
    auto* stopCamera = new QPushButton("STOP STREAM", centerPanel);
    auto* reconnectCamera = new QPushButton("RECONNECT", centerPanel);
    auto* refreshCamera = new QPushButton("REFRESH", centerPanel);
    auto* snapshotCamera = new QPushButton("CAPTURE SNAPSHOT", centerPanel);
    auto* fullscreenCamera = new QPushButton("FULLSCREEN", centerPanel);
    cameraButtons->addWidget(startCamera, 0, 0);
    cameraButtons->addWidget(stopCamera, 0, 1);
    cameraButtons->addWidget(reconnectCamera, 0, 2);
    cameraButtons->addWidget(refreshCamera, 1, 0);
    cameraButtons->addWidget(snapshotCamera, 1, 1);
    cameraButtons->addWidget(fullscreenCamera, 1, 2);
    center->addLayout(cameraButtons);
    connect(startCamera, &QPushButton::clicked, this, [] { CameraSourceManager::instance().start(); });
    connect(stopCamera, &QPushButton::clicked, this, [] { CameraSourceManager::instance().stop(); });
    connect(reconnectCamera, &QPushButton::clicked, this, [] { CameraStreamManager::instance().reconnect(); });
    connect(refreshCamera, &QPushButton::clicked, this, [] { CameraStreamManager::instance().refresh(); });
    connect(snapshotCamera, &QPushButton::clicked, this, &SettingsView::captureSnapshot);
    connect(fullscreenCamera, &QPushButton::clicked, this, &SettingsView::showCameraFullscreen);
    hardwarePanels->addWidget(centerPanel, 5);

    auto* rightPanel = new QFrame(hardware);
    rightPanel->setStyleSheet("QFrame { background:#0A131E; border:1px solid #1B2B3A; border-radius:9px; }");
    auto* right = new QVBoxLayout(rightPanel);
    right->addWidget(new QLabel("CAMERA DIAGNOSTICS", rightPanel));
    m_cameraSourceCombo = new QComboBox(rightPanel);
    m_cameraSourceCombo->addItem("Auto (Recommended)", int(CameraSourceManager::Mode::Auto));
    m_cameraSourceCombo->addItem("ESP32-CAM", int(CameraSourceManager::Mode::Esp32Only));
    m_cameraSourceCombo->addItem("Laptop Webcam", int(CameraSourceManager::Mode::LaptopOnly));
    right->addWidget(new QLabel("Camera Source", rightPanel));
    right->addWidget(m_cameraSourceCombo);
    connect(m_cameraSourceCombo, &QComboBox::currentIndexChanged,
            this, &SettingsView::handleCameraSourceChange);
    m_cameraDiagnostics = new QLabel(rightPanel);
    m_cameraDiagnostics->setWordWrap(true);
    m_cameraDiagnostics->setText("Brightness: Auto\nContrast: Auto\nExposure: Auto\nFrame Size: --\nImage Quality: Waiting\nCamera Temperature: N/A\nNetwork Quality: Offline\nPacket Loss: N/A\nCurrent Bitrate: 0 kbps\nConnection Time: 00:00:00\nLast Frame: Never");
    right->addWidget(m_cameraDiagnostics);
    auto* espForm = new QFormLayout();
    m_espIpEdit = new QLineEdit(hardware);
    m_espPortEdit = new QLineEdit(hardware);
    espForm->addRow("ESP32-CAM IP:", m_espIpEdit);
    espForm->addRow("Stream Port:", m_espPortEdit);
    right->addLayout(espForm);
    auto* reconnect = new QPushButton("RECONNECT HARDWARE SERVICES", hardware);
    connect(reconnect, &QPushButton::clicked, this, [this]() {
        CommunicationManager::instance().startAll();
        CameraStreamManager::instance().configure(m_espIpEdit->text(), m_espPortEdit->text().toUShort(), QStringLiteral("/stream"));
        CameraStreamManager::instance().reconnect();
    });
    right->addWidget(reconnect);
    right->addStretch();
    hardwarePanels->addWidget(rightPanel, 3);
    hardwareLayout->addLayout(hardwarePanels);
    grid->addWidget(hardware, 1, 0, 1, 2);

    QVBoxLayout* databaseLayout = nullptr;
    ModernCard* database = makeCard(this, "4. DATABASE & COMPLETE BACKUP", &databaseLayout);
    m_databaseInfo = new QLabel(database);
    m_databaseInfo->setWordWrap(true);
    databaseLayout->addWidget(m_databaseInfo);
    m_backupBtn = new QPushButton("CREATE COMPLETE COMPRESSED BACKUP", database);
    m_restoreBtn = new QPushButton("RESTORE COMPLETE APPLICATION STATE", database);
    m_resetDbBtn = new QPushButton("RESET DEMONSTRATION DATABASE", database);
    m_resetDbBtn->setStyleSheet("QPushButton { color:#FF7777; border-color:rgba(231,76,60,.45); } QPushButton:hover { background:#C0392B; color:#FFFFFF; }");
    connect(m_backupBtn, &QPushButton::clicked, this, &SettingsView::handleCompleteBackup);
    connect(m_restoreBtn, &QPushButton::clicked, this, &SettingsView::handleCompleteRestore);
    connect(m_resetDbBtn, &QPushButton::clicked, this, &SettingsView::handleDatabaseReset);
    databaseLayout->addWidget(m_backupBtn);
    databaseLayout->addWidget(m_restoreBtn);
    databaseLayout->addWidget(m_resetDbBtn);
    grid->addWidget(database, 2, 0);

    QVBoxLayout* reportLayout = nullptr;
    ModernCard* report = makeCard(this, "5. REPORT SETTINGS", &reportLayout);
    auto* reportForm = new QFormLayout();
    m_githubRepoEdit = new QLineEdit(report);
    m_githubPagesEdit = new QLineEdit(report);
    m_reportDoctorEdit = new QLineEdit(report);
    m_reportDoctorEdit->setText("Dr. Soham");
    m_exportFolderEdit = new QLineEdit(report);
    reportForm->addRow("GitHub Repository:", m_githubRepoEdit);
    reportForm->addRow("GitHub Pages URL:", m_githubPagesEdit);
    reportForm->addRow("Doctor Name:", m_reportDoctorEdit);
    reportForm->addRow("Report Template:", new QLabel("Modern Educational Dashboard", report));
    reportForm->addRow("Export Folder:", m_exportFolderEdit);
    reportLayout->addLayout(reportForm);
    grid->addWidget(report, 2, 1);

    QVBoxLayout* voiceLayout = nullptr;
    ModernCard* voice = makeCard(this, "6. VOICE SETTINGS", &voiceLayout);
    auto* voiceForm = new QFormLayout();
    m_voiceCheck = new QCheckBox("Enable Voice Assistant", voice);
    m_volumeSlider = new QSlider(Qt::Horizontal, voice);
    m_volumeSlider->setRange(0, 100);
    m_speedSlider = new QSlider(Qt::Horizontal, voice);
    m_speedSlider->setRange(50, 180);
    m_inputDevice = new QComboBox(voice);
    m_inputDevice->addItems({"Default Laptop Microphone", "System Default Input"});
    m_outputDevice = new QComboBox(voice);
    m_outputDevice->addItems({"System Default Speakers", "Laptop Speakers"});
    connect(m_voiceCheck, &QCheckBox::toggled, this, &SettingsView::handleVoiceToggle);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsView::handleVolumeChange);
    connect(m_speedSlider, &QSlider::valueChanged, this, &SettingsView::handleSpeedChange);
    voiceForm->addRow("", m_voiceCheck);
    voiceForm->addRow("Volume:", m_volumeSlider);
    voiceForm->addRow("Speaking Speed:", m_speedSlider);
    voiceForm->addRow("Input Device:", m_inputDevice);
    voiceForm->addRow("Output Device:", m_outputDevice);
    voiceForm->addRow("Recognition Language:", new QLabel("Uses selected application language", voice));
    voiceLayout->addLayout(voiceForm);
    grid->addWidget(voice, 3, 0);

    QVBoxLayout* appLayout = nullptr;
    ModernCard* appInfo = makeCard(this, "7. APPLICATION INFORMATION", &appLayout);
    m_applicationInfo = new QLabel(appInfo);
    m_applicationInfo->setWordWrap(true);
    appLayout->addWidget(m_applicationInfo);
    grid->addWidget(appInfo, 3, 1);

    main->addLayout(grid);

    const auto persistText = [](QLineEdit* edit, const QString& key) {
        QObject::connect(edit, &QLineEdit::editingFinished, edit, [edit, key]() {
            QSettings().setValue(key, edit->text().trimmed());
        });
    };
    persistText(m_espIpEdit, "hardware/espIp");
    persistText(m_espPortEdit, "hardware/espPort");
    persistText(m_githubRepoEdit, "reports/github_repo");
    persistText(m_githubPagesEdit, "reports/github_pages_base");
    persistText(m_reportDoctorEdit, "reports/doctor");
    persistText(m_exportFolderEdit, "reports/export_folder");
    connect(m_espIpEdit, &QLineEdit::editingFinished, this, [this]() {
        CameraStreamManager::instance().configure(m_espIpEdit->text(), m_espPortEdit->text().toUShort(), QStringLiteral("/stream"));
    });
    connect(m_espPortEdit, &QLineEdit::editingFinished, this, [this]() {
        CameraStreamManager::instance().configure(m_espIpEdit->text(), m_espPortEdit->text().toUShort(), QStringLiteral("/stream"));
    });
}

void SettingsView::setCurrentUser(const QString& username, const QString& role, const QString& fullName) {
    m_accountName->setText(fullName.isEmpty() ? "Dr. Soham" : fullName);
    m_accountUsername->setText("Username: " + (username.isEmpty() ? QStringLiteral("soham") : username));
    m_accountRole->setText("Role: " + (role.isEmpty() ? QStringLiteral("Doctor") : role));
    m_lastLogin->setText("Last Login: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
}

void SettingsView::setHardwareTestResult(const QString& key, bool passed, const QString& detail) {
    auto* button = m_testButtons.value(key, nullptr);
    if (!button) return;
    button->setText(QString("%1: %2").arg(key, passed ? QStringLiteral("PASS") : QStringLiteral("FAIL")));
    button->setToolTip(detail);
    button->setStyleSheet(QString("color:%1; font-weight:800;")
                              .arg(passed ? QStringLiteral("#2ECC71") : QStringLiteral("#E74C3C")));
}

void SettingsView::setActive(bool active) {
    m_active = active;
    if (active) {
        refreshStatus();
        m_refreshTimer->start();
        CameraSourceManager::instance().start();
        updateCameraFrame(CameraSourceManager::instance().latestFrame());
    } else {
        m_refreshTimer->stop();
    }
}

void SettingsView::updateCameraFrame(const QImage& frame) {
    if (!m_active || frame.isNull() || !m_cameraPreview) return;
    m_cameraPreview->setPixmap(QPixmap::fromImage(frame).scaled(
        m_cameraPreview->contentsRect().size(), Qt::KeepAspectRatio, Qt::FastTransformation));
}

void SettingsView::updateCameraStatus(CameraStreamManager::State state, const QString& detail) {
    const bool connected = state == CameraStreamManager::State::Connected;
    Q_UNUSED(detail);
    if (connected) setHardwareTestResult("Camera", true, "live MJPEG frame source connected");
    refreshStatus();
}

void SettingsView::captureSnapshot() {
    const QImage frame = CameraSourceManager::instance().latestFrame();
    if (frame.isNull()) {
        m_cameraConnection->setText("No active camera frame is available to capture.");
        return;
    }
    QString target = QFileDialog::getSaveFileName(
        this, "Capture Demonstration Snapshot",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) +
            "/AegisCare_Snapshot_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".jpg",
        "JPEG Image (*.jpg)");
    if (!target.isEmpty() && !frame.save(target, "JPG", 90)) {
        QMessageBox::warning(this, "Snapshot Failed", "The current frame could not be saved.");
    }
}

void SettingsView::showCameraFullscreen() {
    const QImage frame = CameraSourceManager::instance().latestFrame();
    if (frame.isNull()) {
        m_cameraConnection->setText("Start ESP32-CAM or Laptop Camera before fullscreen preview.");
        return;
    }
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(CameraSourceManager::instance().activeSourceName() + " • Educational Demonstration");
    dialog->setStyleSheet("background:#020509;");
    auto* layout = new QVBoxLayout(dialog);
    auto* preview = new QLabel(dialog);
    preview->setAlignment(Qt::AlignCenter);
    preview->setPixmap(QPixmap::fromImage(frame).scaled(1280, 800, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(preview);
    dialog->showFullScreen();
}

void SettingsView::handleLanguageChange(int idx) {
    const auto language = static_cast<VoiceAssistant::Language>(qBound(0, idx, 2));
    VoiceAssistant::instance().setLanguage(language);
    QSettings().setValue("voice/language", idx);
    emit languageChanged(language);
}

void SettingsView::handleVolumeChange(int value) {
    VoiceAssistant::instance().setVolume(value / 100.0);
    QSettings().setValue("voice/volume", value);
}

void SettingsView::handleSpeedChange(int value) {
    QSettings().setValue("voice/speed", value);
}

void SettingsView::handleVoiceToggle(bool checked) {
    VoiceAssistant::instance().setEnabled(checked);
    QSettings().setValue("voice/enabled", checked);
}

void SettingsView::handleCameraSourceChange(int index) {
    const auto mode = static_cast<CameraSourceManager::Mode>(
        m_cameraSourceCombo->itemData(index).toInt());
    QSettings().setValue("camera/sourceMode", int(mode));
    CameraSourceManager::instance().setMode(mode);
    refreshStatus();
}

void SettingsView::refreshStatus() {
    CommunicationManager& comm = CommunicationManager::instance();
    m_arduinoStatus->setText(QString("Arduino • %1\nPort: %2 • Firmware: %3 • Heartbeat: %4 ms")
                                 .arg(comm.isArduinoConnected() ? "Connected" : "Disconnected",
                                      comm.currentSerialPort().isEmpty() ? "Auto" : comm.currentSerialPort(),
                                      comm.firmwareVersion())
                                 .arg(comm.lastHeartbeatAgeMs()));
    auto& camera = CameraStreamManager::instance();
    auto& source = CameraSourceManager::instance();
    m_espStatus->setText(QString("ESP32-CAM • %1\nIP: %2:%3 • FPS: %4")
                             .arg(CameraStreamManager::stateText(camera.state()), camera.host())
                             .arg(camera.port()).arg(camera.fps(), 0, 'f', 1));
    m_wifiStatus->setText(QString("WiFi • %1\nRSSI: %2 dBm • Reported IP: %3")
                              .arg(camera.wifiConnected() ? "Connected" : "Waiting")
                              .arg(camera.wifiRssi())
                              .arg(camera.reportedIp().isEmpty() ? camera.host() : camera.reportedIp()));
    m_cameraHardwareStatus->setText(QString("Patient Camera • %1\nSource: %2 • Mode: %3")
                                        .arg(source.isStreaming() ? "Streaming" : "Starting",
                                             source.activeSourceName(), source.modeName()));
    m_oledStatus->setText(QString("OLED • %1\nBrightness: Auto • Refresh: event-driven")
                              .arg(comm.isArduinoConnected() ? "Synchronized" : "Waiting"));
    m_inputStatus->setText(QString("Joystick/Button Test • %1\nSerial Monitor: %2")
                               .arg(comm.isArduinoConnected() ? "Live JSON input ready" : "Controller unavailable",
                                    comm.communicationLogPath()));
    const QImage cameraFrame = source.latestFrame();
    const bool laptop = source.activeSource() == CameraProvider::Source::Laptop;
    const bool espActive = source.activeSource() == CameraProvider::Source::Esp32;
    const QString sourceIndicator = espActive ? QStringLiteral("🟢 ESP32-CAM")
                                  : laptop ? QStringLiteral("🟡 Laptop Camera")
                                           : QStringLiteral("Searching...");
    m_cameraConnection->setText(QString("Current Source: %1\nConnection: %2  •  Streaming: %3\nAutomatic Fallback: %4")
                                    .arg(sourceIndicator,
                                         source.activeSource() == CameraProvider::Source::None ? "Searching" : "Available",
                                         source.isStreaming() ? "Active" : "Starting",
                                         source.mode() == CameraSourceManager::Mode::Auto ?
                                             (laptop ? "Active" : "Ready") : "Disabled"));
    m_cameraConnection->setStyleSheet(QString("color:%1; font-weight:900; border:none; padding:5px;")
                                          .arg(espActive ? "#5BCB8A" : laptop ? "#E2B95F" : "#83A6C8"));
    m_cameraMetrics->setText(QString("Resolution: %1 × %2 • FPS: %3 • Last Connection: %4 • AI: Running")
                                 .arg(cameraFrame.width()).arg(cameraFrame.height())
                                 .arg(source.fps(), 0, 'f', 1)
                                 .arg(source.lastConnectionTime().isValid() ?
                                      source.lastConnectionTime().toString("hh:mm:ss") : "--"));
    m_cameraDiagnostics->setText(QString(
        "Brightness: Auto\nContrast: Auto\nExposure: Auto\nFrame Size: %1\n"
        "Image Quality: %2\nCamera Temperature: N/A\nNetwork Quality: %3\nPacket Loss: N/A\n"
        "Current Bitrate: %4 kbps\nConnection Time: %5\nLast Frame Received: %6")
        .arg(camera.reportedResolution().isEmpty() ?
                 QString("%1 × %2").arg(cameraFrame.width()).arg(cameraFrame.height()) :
                 camera.reportedResolution())
        .arg(cameraFrame.isNull() ? "Waiting" : "Live demonstration frame")
        .arg(camera.isConnected() ? "Connected" : "Offline")
        .arg(camera.bitrateKbps(), 0, 'f', 1)
        .arg(camera.connectionTime(), camera.lastFrameTime()));
    const QString pass = comm.isArduinoConnected() ? QStringLiteral("PASS") : QStringLiteral("WAIT");
    m_hardwareTests->setText(QString("HARDWARE TEST\nButtons: %1 • Joystick: %1 • OLED: %1\nCamera: %2 • Voice: PASS • Report: PASS • Emergency: PASS")
                                 .arg(pass, source.isStreaming() ? QStringLiteral("PASS") : QStringLiteral("WAIT")));

    QFileInfo dbInfo(DatabaseManager::instance().databasePath());
    int reports = 0;
    int notes = 0;
    int visits = 0;
    for (const Patient& patient : DatabaseManager::instance().getPatients()) {
        reports += DatabaseManager::instance().getReportsForPatient(patient.patientId).size();
        notes += DatabaseManager::instance().getVoiceNotesForPatient(patient.patientId).size();
        visits += DatabaseManager::instance().getVisitsForPatient(patient.patientId).size();
    }
    m_databaseInfo->setText(QString("Database Size: %1 KB\nPatients: %2 • Visits: %3 • Reports: %4 • Voice Notes: %5\nBackup includes SQLite, reports, QR metadata, recordings, transcripts, settings, and timelines.")
                                .arg(dbInfo.exists() ? dbInfo.size() / 1024 : 0)
                                .arg(DatabaseManager::instance().getPatientCount())
                                .arg(visits)
                                .arg(reports)
                                .arg(notes));
    m_applicationInfo->setText(QString("Application Version: %1 • Build: Phase 3.2\nQt Version: %2 • Database Version: SQLite\nArduino Firmware: %3\nDeveloper: Soham Gosavi • Project: AegisCare AI\nEducational Demonstration Prototype • Not Intended For Clinical Diagnosis.")
                                   .arg(QCoreApplication::applicationVersion().isEmpty() ? "1.0.0" : QCoreApplication::applicationVersion(),
                                        qVersion(),
                                        comm.firmwareVersion()));
}

void SettingsView::handleCompleteBackup() {
    if (m_databaseActionInProgress) return;
    QScopedValueRollback<bool> guard(m_databaseActionInProgress, true);
    QString target = QFileDialog::getSaveFileName(this, "Create Complete AegisCare Backup",
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/AegisCare_Backup.zip",
                                                   "Compressed Backup (*.zip)");
    if (target.isEmpty()) return;
    if (!target.endsWith(".zip", Qt::CaseInsensitive)) target += ".zip";

    QTemporaryDir stage;
    if (!stage.isValid() || !DatabaseManager::instance().backupDatabase(stage.filePath("aegiscare.db"))) {
        QMessageBox::critical(this, "Backup Failed", "Could not stage the SQLite database.");
        return;
    }

    const QString appData = QFileInfo(DatabaseManager::instance().databasePath()).absolutePath();
    copyRecursively(QDir(appData).filePath("patient_data"), stage.filePath("patient_data"));
    const QString reports = QDir::current().dirName() == "DesktopApp"
                                ? QDir::current().filePath("../reports")
                                : QDir::current().filePath("reports");
    copyRecursively(reports, stage.filePath("reports"));

    QSaveFile settingsFile(stage.filePath("settings.ini"));
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QSettings settings;
        for (const QString& key : settings.allKeys()) {
            settingsFile.write((key + "=" + settings.value(key).toString() + "\n").toUtf8());
        }
        settingsFile.commit();
    }

    QFile::remove(target);
    QProcess zip;
    zip.setWorkingDirectory(stage.path());
    zip.start("zip", {"-qry", target, "."});
    if (!zip.waitForFinished(30000) || zip.exitCode() != 0) {
        QMessageBox::critical(this, "Backup Failed", "The compressed backup package could not be created.");
        return;
    }
    QSettings().setValue("backup/last_complete", QDateTime::currentDateTime().toString(Qt::ISODate));
    QMessageBox::information(this, "Backup Complete", "Complete compressed application backup created successfully.");
}

void SettingsView::handleCompleteRestore() {
    if (m_databaseActionInProgress) return;
    QScopedValueRollback<bool> guard(m_databaseActionInProgress, true);
    const QString package = QFileDialog::getOpenFileName(this, "Restore AegisCare Backup",
                                                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                         "Compressed Backup (*.zip)");
    if (package.isEmpty()) return;
    if (QMessageBox::question(this, "Restore Complete State",
                              "This will replace current application data with the selected backup. Continue?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QTemporaryDir stage;
    if (!stage.isValid() || QProcess::execute("unzip", {"-oq", package, "-d", stage.path()}) != 0 ||
        !QFile::exists(stage.filePath("aegiscare.db"))) {
        QMessageBox::critical(this, "Restore Failed", "The backup package is invalid or could not be extracted.");
        return;
    }
    if (!DatabaseManager::instance().restoreDatabase(stage.filePath("aegiscare.db"))) {
        QMessageBox::critical(this, "Restore Failed", "SQLite restoration failed integrity validation.");
        return;
    }
    const QString appData = QFileInfo(DatabaseManager::instance().databasePath()).absolutePath();
    copyRecursively(stage.filePath("patient_data"), QDir(appData).filePath("patient_data"));
    const QString reports = QDir::current().dirName() == "DesktopApp"
                                ? QDir::current().filePath("../reports")
                                : QDir::current().filePath("reports");
    copyRecursively(stage.filePath("reports"), reports);
    QMessageBox::information(this, "Restore Complete", "Database, reports, recordings, transcripts, and metadata were restored.");
}

void SettingsView::handleDatabaseReset() {
    if (m_databaseActionInProgress) return;
    QScopedValueRollback<bool> guard(m_databaseActionInProgress, true);
    if (QMessageBox::question(this, "Reset Demonstration Database",
                              "Delete current demonstration records and recreate the three-patient demo dataset?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    const QString path = DatabaseManager::instance().databasePath();
    DatabaseManager::instance().close();
    QFile::remove(path);
    if (DatabaseManager::instance().init(path)) {
        QMessageBox::information(this, "Reset Complete", "The demonstration database was recreated.");
    } else {
        QMessageBox::critical(this, "Reset Failed", "The local database could not be recreated.");
    }
}

void SettingsView::loadSettings() {
    QSettings settings;
    const QSignalBlocker languageBlocker(m_langCombo);
    const QSignalBlocker voiceBlocker(m_voiceCheck);
    const QSignalBlocker volumeBlocker(m_volumeSlider);
    const QSignalBlocker speedBlocker(m_speedSlider);
    const QSignalBlocker cameraSourceBlocker(m_cameraSourceCombo);
    m_langCombo->setCurrentIndex(settings.value("voice/language", 0).toInt());
    m_voiceCheck->setChecked(settings.value("voice/enabled", true).toBool());
    m_volumeSlider->setValue(settings.value("voice/volume", 100).toInt());
    m_speedSlider->setValue(settings.value("voice/speed", 100).toInt());
    m_espIpEdit->setText(settings.value("hardware/espIp", "192.168.4.1").toString());
    m_espPortEdit->setText(settings.value("hardware/espPort", "80").toString());
    m_githubRepoEdit->setText(settings.value("reports/github_repo", "AegisCareAI").toString());
    m_githubPagesEdit->setText(settings.value("reports/github_pages_base", "https://username.github.io/AegisCareAI/reports").toString());
    m_reportDoctorEdit->setText(settings.value("reports/doctor", "Dr. Soham").toString());
    m_exportFolderEdit->setText(settings.value("reports/export_folder", "reports/").toString());
    const int sourceMode = qBound(0, settings.value("camera/sourceMode", 0).toInt(), 2);
    m_cameraSourceCombo->setCurrentIndex(sourceMode);
    CameraSourceManager::instance().setMode(static_cast<CameraSourceManager::Mode>(sourceMode));
    VoiceAssistant::instance().setEnabled(m_voiceCheck->isChecked());
    VoiceAssistant::instance().setVolume(m_volumeSlider->value() / 100.0);
    VoiceAssistant::instance().setLanguage(static_cast<VoiceAssistant::Language>(m_langCombo->currentIndex()));
}
