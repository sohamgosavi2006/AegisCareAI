#include "MainWindow.h"
#include "DashboardView.h"
#include "PatientManagementView.h"
#include "DigitalTwinView.h"
#include "ScreeningView.h"
#include "TelemedicineView.h"
#include "SettingsView.h"
#include "ClinicalVoiceNotesPanel.h"
#include "UiTheme.h"
#include "../core/CommunicationManager.h"
#include "../core/CameraStreamManager.h"
#include "../core/CameraSourceManager.h"
#include "../core/DatabaseManager.h"
#include "../core/DemoReportGenerator.h"
#include "../core/VoiceAssistant.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDebug>
#include <QTimer>
#include <QScrollArea>
#include <QScopedValueRollback>
#include <QDialog>
#include <QDateTime>
#include <QJsonObject>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    m_sidebarAnim = new QPropertyAnimation(this, "sidebarWidth", this);
    m_sidebarAnim->setDuration(250);
    m_sidebarAnim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_sidebarAnim, &QPropertyAnimation::finished, this, [this]() {
        if (m_sidebarCollapsed) {
            m_sidebar->setVisible(false);
            m_expandRailButton->setVisible(true);
        }
    });

    m_databaseRefreshTimer = new QTimer(this);
    m_databaseRefreshTimer->setSingleShot(true);
    m_databaseRefreshTimer->setInterval(80);
    connect(m_databaseRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshVisibleView);

    createViews();

    // Start communication services and timers
    CommunicationManager::instance().init();
    CommunicationManager::instance().startAll();
    CameraSourceManager::instance().start();

    // Initialize Voice Assistant and speak Welcome message
    VoiceAssistant::instance().init();
    connect(&VoiceAssistant::instance(), &VoiceAssistant::spoken, this, &MainWindow::handleVoiceSpeech);
    QTimer::singleShot(1500, this, []() {
        VoiceAssistant::instance().speak("Welcome");
    });
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    setMinimumSize(1024, 768);
    setWindowTitle("AegisCare AI - Assisted Rural Healthcare Workstation");

    // Central workspace split layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ==========================================
    // SIDEBAR NAVIGATION WIDGET
    // ==========================================
    m_expandRailButton = new QPushButton("▶", this);
    m_expandRailButton->setFixedWidth(30);
    m_expandRailButton->setVisible(false);
    m_expandRailButton->setToolTip("Expand navigation");
    m_expandRailButton->setStyleSheet(
        "QPushButton { background:#0B111A; color:#00A8FF; border:none; border-right:1px solid rgba(255,255,255,0.08); font-weight:900; }"
        "QPushButton:hover { background:#102033; color:#FFFFFF; }"
    );
    connect(m_expandRailButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    rootLayout->addWidget(m_expandRailButton);

    m_sidebar = new QWidget(this);
    m_sidebar->setObjectName("Sidebar");
    m_sidebar->setFixedWidth(m_sidebarWidth);
    m_sidebar->setStyleSheet(
        "QWidget#Sidebar {"
        "  background-color: #0B111A;"
        "  border-right: 1px solid rgba(255, 255, 255, 0.08);"
        "}"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Sidebar logo heading
    QWidget* logoHeader = new QWidget(this);
    logoHeader->setFixedHeight(70);
    logoHeader->setStyleSheet("border-bottom: 1px solid rgba(255,255,255,0.05);");
    
    QHBoxLayout* logoLayout = new QHBoxLayout(logoHeader);
    logoLayout->setContentsMargins(15, 0, 15, 0);
    
    QLabel* logoIcon = new QLabel("🛡️", this);
    logoIcon->setStyleSheet("font-size: 24px;");
    logoLayout->addWidget(logoIcon);

    QLabel* logoText = new QLabel("AEGISCARE AI", this);
    logoText->setObjectName("logoText");
    logoText->setStyleSheet("font-size: 14px; font-weight: 800; color: #FFFFFF; letter-spacing: 1px;");
    logoLayout->addWidget(logoText);
    logoLayout->addStretch();
    
    sidebarLayout->addWidget(logoHeader);

    // Nav List Items
    m_navList = new QListWidget(this);
    m_navList->setStyleSheet(
        "QListWidget {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 10px 0px;"
        "}"
        "QListWidget::item {"
        "  color: #A1AAB3;"
        "  padding: 12px 18px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: rgba(255, 255, 255, 0.03);"
        "  color: #FFFFFF;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(0, 168, 255, 0.1);"
        "  color: #00A8FF;"
        "  border-left: 3px solid #00A8FF;"
        "}"
    );

    // Add navigation categories
    m_navList->addItem("📊   Dashboard");
    m_navList->addItem("👥   Patient Registry & Care Management");
    m_navList->addItem("🚶   Digital Health Twin");
    m_navList->addItem("🩺   Assisted Screening");
    m_navList->addItem("🏥   Telemedicine Link");
    m_navList->addItem("⚙️   Workstation Settings");
    
    m_navList->setCurrentRow(0);
    connect(m_navList, &QListWidget::currentRowChanged, this, &MainWindow::handleNavigation);
    sidebarLayout->addWidget(m_navList, 1);

    // Sidebar footer: collapse button
    m_collapseButton = new QPushButton("◀  COLLAPSE PANEL", this);
    m_collapseButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255,255,255,0.03); border: none; border-top: 1px solid rgba(255,255,255,0.05); color: #A1AAB3; font-weight: bold; font-size: 10px; padding: 12px;"
        "}"
        "QPushButton:hover { background-color: rgba(255,255,255,0.06); color: #FFFFFF; }"
    );
    connect(m_collapseButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    sidebarLayout->addWidget(m_collapseButton);

    rootLayout->addWidget(m_sidebar);

    // ==========================================
    // CENTRAL WORKSPACE CONTAINER
    // ==========================================
    QWidget* workspace = new QWidget(this);
    QVBoxLayout* workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    // 1. Top Workflow Timeline bar
    setupTimelineBar();
    workspaceLayout->addWidget(m_timelineWidget);

    // 2. View stack container
    m_viewStack = new QStackedWidget(workspace);
    m_viewStack->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_viewStack->setAutoFillBackground(true);
    m_viewStack->setStyleSheet("QStackedWidget { background-color: #0D1621; border:none; }");
    workspaceLayout->addWidget(m_viewStack, 1);

    // 3. Bottom status HUD bar
    setupStatusHUD();
    
    // Status Bar styling card
    QWidget* hudBarWidget = new QWidget(this);
    hudBarWidget->setMinimumHeight(42);
    hudBarWidget->setStyleSheet("background-color: #080D14; border-top: 1px solid rgba(255,255,255,0.05);");
    
    QHBoxLayout* hudLayout = new QHBoxLayout(hudBarWidget);
    hudLayout->setContentsMargins(15, 0, 15, 0);
    
    hudLayout->addWidget(m_operatorLabel);
    hudLayout->addWidget(new QLabel(" | ", this));
    hudLayout->addWidget(m_patientLabel);
    hudLayout->addWidget(new QLabel(" | ", this));
    hudLayout->addWidget(m_voiceAssistantFeedback, 1); // Expand feedback bubble
    hudLayout->addWidget(new QLabel(" | ", this));
    hudLayout->addWidget(m_espStatusDot);
    hudLayout->addWidget(m_arduinoStatusDot);
    hudLayout->addWidget(m_networkStatusLabel);

    workspaceLayout->addWidget(hudBarWidget);
    rootLayout->addWidget(workspace);

    // Style elements
    setStyleSheet("QMainWindow { background-color: #0D1621; }");
}

void MainWindow::createViews() {
    m_dashboardView = new DashboardView(this);
    m_patientView = new PatientManagementView(this);
    m_digitalTwinView = new DigitalTwinView(this);
    m_screeningView = new ScreeningView(this);
    m_telemedicineView = new TelemedicineView(this);
    m_settingsView = new SettingsView(this);
    m_voiceNotesPanel = new ClinicalVoiceNotesPanel(this);

    const auto addResponsiveView = [this](QWidget* view) {
        auto* scrollArea = new QScrollArea(m_viewStack);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setAttribute(Qt::WA_OpaquePaintEvent, true);
        scrollArea->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        scrollArea->viewport()->setAutoFillBackground(true);
        scrollArea->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background:#0D1621; border:none; }");
        view->setAttribute(Qt::WA_StyledBackground, true);
        view->setStyleSheet(QStringLiteral("%1 { background:#0D1621; }").arg(view->metaObject()->className()));
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        scrollArea->setWidget(view);
        m_viewStack->addWidget(scrollArea);
    };

    addResponsiveView(m_dashboardView);
    addResponsiveView(m_patientView);
    addResponsiveView(m_digitalTwinView);
    addResponsiveView(m_screeningView);
    addResponsiveView(m_telemedicineView);
    addResponsiveView(m_settingsView);
    m_dashboardView->setActive(true);
    m_screeningView->setActive(false);
    m_settingsView->setActive(false);

    connect(m_dashboardView, &DashboardView::emergencyRequested, this, [this]() {
        activateEmergencyMode();
    });

    // Connect patient selection triggers across widgets
    connect(m_patientView, &PatientManagementView::patientSelected, this, &MainWindow::selectActivePatient);
    
    // Connect digital twin scan triggers
    connect(m_digitalTwinView, &DigitalTwinView::screeningRequested, this, [this](const QString& region) {
        m_navList->setCurrentRow(3); // Go to screening view
        m_screeningView->selectOrganRegion(region);
    });

    CommunicationManager& communication = CommunicationManager::instance();
    connect(&communication, &CommunicationManager::buttonPressed,
            this, &MainWindow::handleControllerButton);
    connect(&communication, &CommunicationManager::joystickMoved,
            this, &MainWindow::handleControllerJoystick);
    connect(&communication, &CommunicationManager::scanControlReceived,
            this, &MainWindow::handleScanControl);
    connect(&communication, &CommunicationManager::developerAboutRequested,
            this, &MainWindow::showDeveloperAbout);
    connect(m_screeningView, &ScreeningView::scanCompleted,
            this, &MainWindow::handleScanCompleted);
    connect(m_voiceNotesPanel, &ClinicalVoiceNotesPanel::noteSaved,
            this, &MainWindow::handleVoiceNoteSaved);

    // Coalesce database bursts and refresh only the page the operator can see.
    connect(&DatabaseManager::instance(), &DatabaseManager::databaseUpdated, this, [this]() {
        m_databaseRefreshTimer->start();
    });

    // Settings actions routing
    m_settingsView->setCurrentUser(m_username, m_role, m_operatorFullName);
    connect(m_settingsView, &SettingsView::languageChanged, this, [this](VoiceAssistant::Language lang) {
        applyLanguage(lang);
    });
    connect(m_settingsView, &SettingsView::logoutRequested, this, [this]() {
        close();
    });
    connect(&CameraSourceManager::instance(), &CameraSourceManager::notificationRequested,
            this, &MainWindow::showCameraToast);
    // Workflow signals
    connect(&WorkflowEngine::instance(), &WorkflowEngine::stageChanged, this, &MainWindow::updateWorkflowProgress);
    updateWorkflowProgress(WorkflowEngine::instance().currentStage());
}

void MainWindow::setupStatusHUD() {
    m_operatorLabel = new QLabel("OPERATOR: Not Authenticated", this);
    m_operatorLabel->setStyleSheet("font-size: 10px; color: #A1AAB3; font-family: 'Monospace'; font-weight: bold;");

    m_patientLabel = new QLabel("PATIENT LOADED: NONE", this);
    m_patientLabel->setStyleSheet("font-size: 10px; color: #00A8FF; font-family: 'Monospace'; font-weight: bold;");

    m_voiceAssistantFeedback = new QLabel("🎙️ Voice Companion: Online", this);
    m_voiceAssistantFeedback->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.45); font-family: 'Monospace';");

    m_espStatusDot = new QLabel("ESP32-CAM: --", this);
    m_espStatusDot->setStyleSheet("font-size: 10px; color: #E74C3C; font-family: 'Monospace'; font-weight: bold;");

    m_arduinoStatusDot = new QLabel("ARDUINO: --", this);
    m_arduinoStatusDot->setStyleSheet("font-size: 10px; color: #E74C3C; font-family: 'Monospace'; font-weight: bold;");

    m_networkStatusLabel = new QLabel("📶 LOCAL_ONLY", this);
    m_networkStatusLabel->setStyleSheet("font-size: 10px; color: #E67E22; font-family: 'Monospace'; font-weight: bold;");

    // Connect status checkers
    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateHardwareStatus);
    statusTimer->start(1500);
}

void MainWindow::setupTimelineBar() {
    m_timelineWidget = new QWidget(this);
    m_timelineWidget->setMinimumHeight(56);
    m_timelineWidget->setStyleSheet("background-color: #0B111A; border-bottom: 1px solid rgba(255, 255, 255, 0.05);");

    QHBoxLayout* timelineLayout = new QHBoxLayout(m_timelineWidget);
    timelineLayout->setContentsMargins(15, 0, 15, 0);

    m_currentStepLabel = new QLabel("WORKFLOW PROGRESS: OPERATOR SETUP", this);
    m_currentStepLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF; letter-spacing: 0.5px;");
    timelineLayout->addWidget(m_currentStepLabel, 2);

    auto* demoBadge = new QLabel("DEMONSTRATION MODE • EDUCATIONAL PROTOTYPE • NOT A MEDICAL DEVICE", this);
    demoBadge->setAlignment(Qt::AlignCenter);
    demoBadge->setStyleSheet("font-size: 9px; font-weight: 800; color: #F5B041; padding: 4px 8px; background: rgba(245,176,65,0.08); border-radius: 5px;");
    timelineLayout->addWidget(demoBadge, 3);

    m_workflowProgress = new QProgressBar(this);
    m_workflowProgress->setRange(0, 100);
    m_workflowProgress->setValue(0);
    m_workflowProgress->setFixedHeight(12);
    m_workflowProgress->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 6px;"
        "  background-color: rgba(0, 0, 0, 100);"
        "  text-align: center;"
        "  font-size: 9px; color: transparent;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00A8FF, stop:1 #2ECC71);"
        "  border-radius: 5px;"
        "}"
    );
    timelineLayout->addWidget(m_workflowProgress, 3);
}

void MainWindow::setOperatorInfo(const QString& username, const QString& role, const QString& fullName) {
    m_username = username;
    m_role = role;
    m_operatorFullName = fullName;
    m_operatorLabel->setText(QString("OPERATOR: %1 (%2)").arg(fullName.toUpper()).arg(role.toUpper()));
    if (m_dashboardView) m_dashboardView->setOperatorName(fullName);
    if (m_settingsView) m_settingsView->setCurrentUser(username, role, fullName);
}

void MainWindow::selectActivePatient(const QString& patientId) {
    m_activePatientId = patientId;
    
    Patient p = DatabaseManager::instance().getPatient(patientId);
    m_patientLabel->setText(QString("PATIENT LOADED: %1 (%2)").arg(p.fullName.toUpper()).arg(p.patientId));
    
    // Sync subviews with selected patient
    m_digitalTwinView->setPatient(patientId);
    m_screeningView->setPatient(patientId);
    m_telemedicineView->setPatient(patientId);
    WorkflowEngine::instance().selectPatient();
    if (m_dashboardView) m_dashboardView->setActivePatient(p.fullName, p.patientId);
    CommunicationManager::instance().sendJSON(
        QJsonObject{{QStringLiteral("type"), QStringLiteral("patient_selected")},
                    {QStringLiteral("id"), p.patientId},
                    {QStringLiteral("name"), p.fullName}});
}

void MainWindow::handleNavigation(int index) {
    if (m_navigationInProgress || index < 0 || index >= m_viewStack->count()) return;
    QScopedValueRollback<bool> guard(m_navigationInProgress, true);
    if (m_navList->currentRow() != index) m_navList->setCurrentRow(index);
    if (m_viewStack->currentIndex() != index) m_viewStack->setCurrentIndex(index);
    m_dashboardView->setActive(index == 0);
    m_screeningView->setActive(index == 3);
    m_settingsView->setActive(index == 5);
    refreshVisibleView();

    static const QStringList pageGuidance = {
        "Dashboard opened. Review clinic activity and demonstration statistics.",
        "Patient registry and care management opened. Review profile, visits, notes, reports, and follow-up tasks.",
        "Digital twin opened. This educational visualization does not diagnose disease.",
        "Assisted screening opened. All results are simulated demonstrations.",
        "Telemedicine demonstration opened.",
        "Workstation settings opened."
    };
    if (index < pageGuidance.size()) VoiceAssistant::instance().speakTextDirectly(pageGuidance[index]);
}

void MainWindow::handleControllerButton(int buttonNum) {
    if (buttonNum == 1) {
        if (m_screeningView->isScanning()) {
            m_screeningView->stopScreeningScan();
            sendOledStatus(QStringLiteral("SCAN_STOPPED"));
            return;
        }
        if (m_activePatientId.isEmpty()) {
            QMessageBox::warning(this, "Select Patient",
                                 "Please select a patient before starting the demonstration scan.");
            sendOledStatus(QStringLiteral("SELECT_PATIENT_FIRST"));
            return;
        }
        m_navList->setCurrentRow(3);
        m_screeningView->startScreeningScan();
        CommunicationManager::instance().sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("START_SCAN")}});
        return;
    }

    if (buttonNum == 2) {
        if (m_activePatientId.isEmpty()) {
            QMessageBox::warning(this, "Select Patient",
                                 "Please select a patient before recording clinical voice notes.");
            sendOledStatus(QStringLiteral("SELECT_PATIENT_FIRST"));
            return;
        }
        const Patient patient = DatabaseManager::instance().getPatient(m_activePatientId);
        if (!m_voiceNotesPanel->isRecording()) {
            WorkflowEngine::instance().startVoiceRecording();
            m_voiceNotesPanel->startRecording(patient, QStringLiteral("Dr. Soham"));
            VoiceAssistant::instance().speakTextDirectly("Clinical Observation Mode activated. Recording started.");
            sendOledStatus(QStringLiteral("VOICE_RECORDING"));
        } else {
            VoiceAssistant::instance().speakTextDirectly("Recording stopped. Processing observation.");
            sendOledStatus(QStringLiteral("VOICE_PROCESSING"));
            m_voiceNotesPanel->stopRecordingAndSave();
            WorkflowEngine::instance().stopVoiceRecording();
        }
        return;
    }

    if (buttonNum == 3) {
        generatePatientReport();
        return;
    }

    if (buttonNum == 4) {
        activateEmergencyMode();
        return;
    }
}

void MainWindow::handleControllerJoystick(const QString& direction) {
    if (m_screeningView && m_screeningView->isScanning()) {
        m_screeningView->moveScanTarget(direction);
        return;
    }
    if (direction == "UP") {
        m_navList->setCurrentRow(qMax(0, m_navList->currentRow() - 1));
    } else if (direction == "DOWN") {
        m_navList->setCurrentRow(qMin(m_navList->count() - 1, m_navList->currentRow() + 1));
    } else if (direction == "LEFT") {
        m_navList->setCurrentRow(0);
    } else if (direction == "RIGHT") {
        m_navList->setCurrentRow(qMin(m_navList->count() - 1, m_navList->currentRow() + 1));
    } else if (direction == "CLICK") {
        handleNavigation(m_navList->currentRow());
    }
}

void MainWindow::handleScanControl(const QString& action) {
    if (m_screeningView) m_screeningView->moveScanTarget(action);
}

void MainWindow::showDeveloperAbout() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("About AegisCare AI");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->setFixedSize(340, 180);
    dialog->setStyleSheet("QDialog { background: #0B111A; border: 1px solid #00A8FF; } QLabel { color: #FFFFFF; }");
    auto* layout = new QVBoxLayout(dialog);
    auto* name = new QLabel("Soham Gosavi", dialog);
    name->setAlignment(Qt::AlignCenter);
    name->setStyleSheet("font-size: 24px; font-weight: 900; color: #FFFFFF;");
    auto* role = new QLabel("Developer\nAegisCare AI\nVersion 1.0.0", dialog);
    role->setAlignment(Qt::AlignCenter);
    role->setStyleSheet("font-size: 14px; color: #00A8FF; font-weight: 700;");
    layout->addStretch();
    layout->addWidget(name);
    layout->addWidget(role);
    layout->addStretch();
    dialog->show();
    QTimer::singleShot(3000, dialog, &QDialog::close);
}

void MainWindow::handleVoiceNoteSaved(const VoiceNote& note) {
    Q_UNUSED(note);
    VoiceAssistant::instance().speakTextDirectly("Clinical observation successfully saved.");
    if (!m_activePatientId.isEmpty()) {
        TwinRegionState state;
        state.patientId = m_activePatientId;
        state.regionName = QStringLiteral("Clinical Notes");
        state.status = QStringLiteral("Documented");
        state.notes = QStringLiteral("Voice observation added. Transcript and original audio available locally. No diagnosis was generated.");
        state.lastUpdated = QDateTime::currentDateTime().toString(Qt::ISODate);
        DatabaseManager::instance().updateTwinRegion(state);
    }
}

void MainWindow::handleScanCompleted() {
    sendOledStatus(QStringLiteral("SCAN_COMPLETED"));
    VoiceAssistant::instance().speakTextDirectly("Demonstration scan completed. You may generate the educational report.");
}

void MainWindow::generatePatientReport() {
    if (m_activePatientId.isEmpty()) {
        QMessageBox::warning(this, "Select Patient",
                             "Please select a patient before generating a report.");
        sendOledStatus(QStringLiteral("SELECT_PATIENT_FIRST"));
        return;
    }
    if (!WorkflowEngine::instance().canGenerateReport() && !m_screeningView->hasCompletedScan()) {
        QMessageBox::information(this, "Screening Required",
                                 "Complete screening before generating report.\n\nEducational Demonstration Prototype • Not Intended For Clinical Diagnosis.");
        sendOledStatus(QStringLiteral("FINISH_SCAN_FIRST"));
        return;
    }
    sendOledStatus(QStringLiteral("UPLOADING_REPORT"));
    const DemoReportGenerator::Result result =
        DemoReportGenerator::instance().generateForPatient(m_activePatientId, QStringLiteral("Dr. Soham"));
    if (!result.success) {
        QMessageBox::critical(this, "Report Generation Failed", result.error);
        sendOledStatus(QStringLiteral("REPORT_ERROR"));
        return;
    }
    WorkflowEngine::instance().markReportReady();
    CommunicationManager::instance().sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("REPORT_READY")},
                                                          {QStringLiteral("report_id"), result.report.reportId}});
    CommunicationManager::instance().sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("qr_ready")},
                                                          {QStringLiteral("url"), result.report.githubUrl}});
    QMessageBox::information(this, "Report Ready",
                             QString("Educational demonstration report generated.\n\n%1\n\nQR contains only this report URL.\nNot Intended For Clinical Diagnosis.")
                                 .arg(result.report.githubUrl));
}

void MainWindow::activateEmergencyMode() {
    WorkflowEngine::instance().activateEmergency();
    if (m_activePatientId.isEmpty()) {
        QMessageBox::warning(this, "Emergency Mode",
                             "Emergency mode activated, but no active patient is loaded.");
        sendOledStatus(QStringLiteral("EMERGENCY_MODE"));
        if (m_dashboardView) m_dashboardView->setEmergencyMode(true);
        m_navList->setCurrentRow(0);
        VoiceAssistant::instance().speakTextDirectly("Emergency Mode Activated. Please perform immediate evaluation.");
        return;
    }

    EmergencyEvent event;
    event.patientId = m_activePatientId;
    event.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    event.operatorName = QStringLiteral("Dr. Soham");
    event.priority = QStringLiteral("Critical");
    event.details = QStringLiteral("Emergency response mode activated from the hardware controller.");
    DatabaseManager::instance().addEmergencyEvent(event);

    TwinRegionState state;
    state.patientId = m_activePatientId;
    state.regionName = QStringLiteral("Emergency");
    state.status = QStringLiteral("Critical");
    state.notes = QStringLiteral("Emergency activated by Dr. Soham. Immediate evaluation requested. No automated diagnosis was performed.");
    state.lastUpdated = event.timestamp;
    DatabaseManager::instance().updateTwinRegion(state);

    sendOledStatus(QStringLiteral("EMERGENCY_MODE"));
    CommunicationManager::instance().sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("EMERGENCY_MODE")}});
    VoiceAssistant::instance().speakTextDirectly("Emergency Mode Activated. Patient marked as High Priority. Please perform immediate evaluation.");
    if (m_dashboardView) m_dashboardView->setEmergencyMode(true);
    m_navList->setCurrentRow(0);
    QMessageBox::critical(this, "EMERGENCY MODE",
                          "Patient marked HIGH PRIORITY.\n\nOperator: Dr. Soham\nPriority: Critical\n\nEducational Demonstration Prototype • Not Intended For Clinical Diagnosis.");
}

void MainWindow::sendOledStatus(const QString& status) {
    CommunicationManager::instance().sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("oled_status")},
                                                          {QStringLiteral("status"), status}});
}

void MainWindow::refreshVisibleView() {
    const int index = m_viewStack ? m_viewStack->currentIndex() : -1;
    if (index == 0 && m_dashboardView) m_dashboardView->refreshData();
    else if (index == 1 && m_patientView) m_patientView->refreshPatientList();
    else if (index == 2 && m_digitalTwinView) m_digitalTwinView->refreshTwinData();
}

void MainWindow::setSidebarWidth(int width) {
    m_sidebarWidth = width;
    m_sidebar->setFixedWidth(width);
}

void MainWindow::toggleSidebar() {
    m_sidebarAnim->stop();

    if (m_sidebarCollapsed) {
        m_sidebar->setVisible(true);
        m_expandRailButton->setVisible(false);
        m_sidebarAnim->setStartValue(0);
        m_sidebarAnim->setEndValue(240);
        m_collapseButton->setText("◀  COLLAPSE PANEL");
        if (auto* logoText = m_sidebar->findChild<QLabel*>("logoText")) logoText->setVisible(true);
        m_sidebarCollapsed = false;
    } else {
        m_sidebarAnim->setStartValue(240);
        m_sidebarAnim->setEndValue(0);
        m_collapseButton->setText("COLLAPSING...");
        m_sidebarCollapsed = true;
    }

    m_sidebarAnim->start();
}

void MainWindow::updateHardwareStatus() {
    CommunicationManager& comms = CommunicationManager::instance();
    CameraStreamManager& camera = CameraStreamManager::instance();
    CameraSourceManager& source = CameraSourceManager::instance();
    
    // Update ESP32
    const bool laptop = source.activeSource() == CameraProvider::Source::Laptop;
    const bool esp = source.activeSource() == CameraProvider::Source::Esp32;
    m_espStatusDot->setText(QString("CAMERA: %1 • %2 FPS%3")
                                .arg(source.activeSourceName())
                                .arg(source.fps(), 0, 'f', 1)
                                .arg(laptop ? " • FALLBACK" : (esp ? QString(" • %1 ms").arg(camera.latencyMs()) : QString())));
    m_espStatusDot->setStyleSheet(QString("font-size:10px; color:%1; font-family:'Monospace'; font-weight:bold;")
                                      .arg(esp ? "#2ECC71" : (laptop ? "#F5B041" : "#7F8C8D")));

    // Update Arduino
    if (comms.isArduinoConnected()) {
        const QString firmware = comms.firmwareVersion() == "Unknown"
                                     ? QString()
                                     : QString(" FW:%1").arg(comms.firmwareVersion());
        m_arduinoStatusDot->setText(QString("ARDUINO: OK [%1]%2")
                                        .arg(comms.currentSerialPort(), firmware));
        m_arduinoStatusDot->setStyleSheet("font-size: 10px; color: #2ECC71; font-family: 'Monospace'; font-weight: bold;");
    } else {
        const QString port = comms.currentSerialPort().isEmpty()
                                 ? QStringLiteral("AUTO")
                                 : comms.currentSerialPort();
        m_arduinoStatusDot->setText(QString("ARDUINO: NC [%1]").arg(port));
        m_arduinoStatusDot->setStyleSheet("font-size: 10px; color: #E74C3C; font-family: 'Monospace'; font-weight: bold;");
    }
    m_networkStatusLabel->setText(QString("OLED: %1 • JOYSTICK: %2")
                                      .arg(comms.isArduinoConnected() ? "CONNECTED" : "WAITING",
                                           comms.isArduinoConnected() ? "READY" : "WAITING"));
}

void MainWindow::showCameraToast(const QString& message, const QString& tone) {
    const QString accent = tone == "green" ? QStringLiteral("#2ECC71")
                         : tone == "orange" ? QStringLiteral("#E89B3C")
                                             : QStringLiteral("#4AA8FF");
    auto* toast = new QLabel(message, this);
    toast->setObjectName("CameraToast");
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->setWordWrap(true);
    toast->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    toast->setFixedSize(360, 58);
    toast->setStyleSheet(QString(
        "background:rgba(14,31,47,242); color:#F5FAFF; border:1px solid %1; border-radius:13px;"
        "padding:10px 16px; font-size:11px; font-weight:750;").arg(accent));
    const int toastCount = findChildren<QLabel*>("CameraToast", Qt::FindDirectChildrenOnly).size();
    const QPoint endPoint(qMax(12, width() - toast->width() - 24), 78 + qMax(0, toastCount - 1) * 66);
    toast->move(endPoint + QPoint(28, 0));
    toast->show();
    toast->raise();
    auto* animation = new QPropertyAnimation(toast, "pos", toast);
    animation->setDuration(220);
    animation->setStartValue(toast->pos());
    animation->setEndValue(endPoint);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(3400, toast, &QWidget::deleteLater);
}

void MainWindow::updateWorkflowProgress(WorkflowEngine::Stage stage) {
    WorkflowEngine& we = WorkflowEngine::instance();  
    m_currentStepLabel->setText("WORKFLOW STATUS: " + we.stageToString(stage).toUpper());
    m_workflowProgress->setValue(we.progressPercent());
}

void MainWindow::handleVoiceSpeech(const QString& text) {
    m_voiceAssistantFeedback->setText("🎙️ Voice Companion: \"" + text + "\"");
    
    // Subtle text flash effect
    m_voiceAssistantFeedback->setStyleSheet("font-size: 10px; color: #00A8FF; font-family: 'Monospace'; font-weight: bold;");
    QTimer::singleShot(2000, this, [this]() {
        m_voiceAssistantFeedback->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.45); font-family: 'Monospace';");
    });
}

void MainWindow::applyLanguage(VoiceAssistant::Language language) {
    if (!m_navList || m_navList->count() < 6) return;
    QStringList labels;
    if (language == VoiceAssistant::Hindi) {
        labels = {"📊   डैशबोर्ड", "👥   रोगी रजिस्ट्री और देखभाल", "🚶   डिजिटल हेल्थ ट्विन",
                  "🩺   सहायक स्क्रीनिंग", "🏥   टेलीमेडिसिन लिंक", "⚙️   वर्कस्टेशन सेटिंग्स"};
    } else if (language == VoiceAssistant::Marathi) {
        labels = {"📊   डॅशबोर्ड", "👥   रुग्ण नोंदणी आणि काळजी", "🚶   डिजिटल हेल्थ ट्विन",
                  "🩺   सहाय्यित स्क्रीनिंग", "🏥   टेलिमेडिसिन लिंक", "⚙️   वर्कस्टेशन सेटिंग्ज"};
    } else {
        labels = {"📊   Dashboard", "👥   Patient Registry & Care Management", "🚶   Digital Health Twin",
                  "🩺   Assisted Screening", "🏥   Telemedicine Link", "⚙️   Workstation Settings"};
    }
    for (int i = 0; i < labels.size(); ++i) m_navList->item(i)->setText(labels[i]);
}
