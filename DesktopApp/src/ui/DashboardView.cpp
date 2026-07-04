#include "DashboardView.h"

#include "../core/CommunicationManager.h"
#include "../core/DatabaseManager.h"
#include "../core/CameraSourceManager.h"

#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QScopedValueRollback>
#include <QVBoxLayout>

DashboardView::DashboardView(QWidget* parent) : QWidget(parent) {
    setupUi();
    connect(&WorkflowEngine::instance(), &WorkflowEngine::deviceStateChanged,
            this, &DashboardView::handleWorkflowStateChanged);
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(1200);
    connect(m_statusTimer, &QTimer::timeout, this, &DashboardView::refreshSystemStatus);
    m_statusTimer->start();

    m_emergencyFlashTimer = new QTimer(this);
    m_emergencyFlashTimer->setInterval(550);
    connect(m_emergencyFlashTimer, &QTimer::timeout, this, &DashboardView::toggleEmergencyBanner);
    refreshData();
}

DashboardView::~DashboardView() = default;

void DashboardView::setActive(bool active) {
    if (active) {
        refreshSystemStatus();
        m_statusTimer->start();
    } else {
        m_statusTimer->stop();
    }
}

void DashboardView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 22, 22, 22);
    mainLayout->setSpacing(18);
    setStyleSheet(
        "QWidget { color: #E1E6EB; }"
        "QListWidget { background: rgba(0,0,0,85); border: 1px solid rgba(255,255,255,0.08); border-radius: 10px; padding: 8px; }"
        "QListWidget::item { padding: 7px; color: #DDE7F0; }"
        "QProgressBar { border: 1px solid rgba(255,255,255,0.12); border-radius: 7px; background: rgba(0,0,0,90); height: 14px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #00A8FF,stop:1 #2ECC71); border-radius: 6px; }"
    );

    auto* header = new QHBoxLayout();
    auto* titleBox = new QVBoxLayout();
    auto* title = new QLabel("HOSPITAL COMMAND CENTER", this);
    title->setStyleSheet("font-size: 24px; font-weight: 950; color: #FFFFFF; letter-spacing: 1.4px;");
    auto* subtitle = new QLabel("Educational Demonstration Prototype • Not Intended For Clinical Diagnosis", this);
    subtitle->setStyleSheet("font-size: 11px; font-weight: 800; color: #F5B041;");
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    header->addLayout(titleBox, 1);
    m_headerBadge = new QLabel("SYSTEM NOMINAL", this);
    m_headerBadge->setAlignment(Qt::AlignCenter);
    m_headerBadge->setMinimumWidth(180);
    m_headerBadge->setStyleSheet("background: rgba(46,204,113,0.14); color:#2ECC71; border:1px solid rgba(46,204,113,0.45); border-radius:12px; padding:10px; font-weight:900;");
    header->addWidget(m_headerBadge);
    mainLayout->addLayout(header);

    auto* topGrid = new QGridLayout();
    topGrid->setSpacing(14);
    topGrid->addWidget(createStatusCard("ARDUINO NANO", &m_arduinoLabel, "#00A8FF"), 0, 0);
    topGrid->addWidget(createStatusCard("PATIENT SCANNER", &m_espLabel, "#56A9E8"), 0, 1);
    topGrid->addWidget(createStatusCard("OLED SYNCHRONIZATION", &m_oledLabel, "#9B59B6"), 0, 2);
    topGrid->addWidget(createStatusCard("VOICE ASSISTANT", &m_voiceLabel, "#F1C40F"), 1, 0);
    topGrid->addWidget(createStatusCard("SQLITE DATABASE", &m_databaseLabel, "#00D1B2"), 1, 1);
    topGrid->addWidget(createStatusCard("GITHUB PAGES REPORTS", &m_githubLabel, "#E67E22"), 1, 2);
    mainLayout->addLayout(topGrid);

    auto* midRow = new QHBoxLayout();
    midRow->setSpacing(14);

    auto* workflowCard = new ModernCard(this);
    workflowCard->setHoverEffect(false);
    workflowCard->setStyleSheet("QFrame { background: rgba(255,255,255,0.035); border: 1px solid rgba(255,255,255,0.08); border-radius: 14px; }");
    auto* workflowLayout = new QVBoxLayout(workflowCard);
    workflowLayout->setContentsMargins(18, 16, 18, 16);
    auto* workflowTitle = new QLabel("WORKFLOW STATUS", workflowCard);
    workflowTitle->setStyleSheet("font-size: 13px; color:#00A8FF; font-weight:900;");
    workflowLayout->addWidget(workflowTitle);
    m_operatorLabel = new QLabel("Current Operator: Dr. Soham", workflowCard);
    m_patientLabel = new QLabel("Current Patient: No patient selected", workflowCard);
    m_workflowStageLabel = new QLabel("Current Stage: IDLE", workflowCard);
    workflowLayout->addWidget(m_operatorLabel);
    workflowLayout->addWidget(m_patientLabel);
    workflowLayout->addWidget(m_workflowStageLabel);
    m_workflowProgress = new QProgressBar(workflowCard);
    m_workflowProgress->setRange(0, 100);
    workflowLayout->addWidget(m_workflowProgress);
    m_overallHealthLabel = new QLabel("Overall System Health: GREEN", workflowCard);
    m_overallHealthLabel->setStyleSheet("font-weight:900; color:#2ECC71;");
    workflowLayout->addWidget(m_overallHealthLabel);
    midRow->addWidget(workflowCard, 2);

    auto* logCard = new ModernCard(this);
    logCard->setHoverEffect(false);
    logCard->setStyleSheet("QFrame { background: rgba(255,255,255,0.035); border: 1px solid rgba(255,255,255,0.08); border-radius: 14px; }");
    auto* logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(18, 16, 18, 16);
    auto* logTitle = new QLabel("LIVE DEMONSTRATION LOG", logCard);
    logTitle->setStyleSheet("font-size: 13px; color:#00A8FF; font-weight:900;");
    m_activityLog = new QListWidget(logCard);
    logLayout->addWidget(logTitle);
    logLayout->addWidget(m_activityLog, 1);
    midRow->addWidget(logCard, 3);
    mainLayout->addLayout(midRow, 1);

    auto* metricGrid = new QGridLayout();
    metricGrid->setSpacing(14);
    metricGrid->addWidget(createMetricCard("TOTAL PATIENTS", &m_totalPatientsLabel, "👥", "#00A8FF"), 0, 0);
    metricGrid->addWidget(createMetricCard("TODAY'S VISITS", &m_todayVisitsLabel, "🩺", "#2ECC71"), 0, 1);
    metricGrid->addWidget(createMetricCard("DEMO SCANS", &m_scansLabel, "📡", "#9B59B6"), 0, 2);
    metricGrid->addWidget(createMetricCard("REPORTS GENERATED", &m_reportsLabel, "📄", "#F1C40F"), 0, 3);
    metricGrid->addWidget(createMetricCard("VOICE NOTES", &m_voiceNotesLabel, "🎤", "#00D1B2"), 1, 0);
    metricGrid->addWidget(createMetricCard("EMERGENCY CASES", &m_emergencyCasesLabel, "🚨", "#E74C3C"), 1, 1);
    metricGrid->addWidget(createMetricCard("FOLLOW-UPS PENDING", &m_followupsLabel, "🗓️", "#E67E22"), 1, 2);

    auto* emergencyCard = new ModernCard(this);
    emergencyCard->setHoverEffect(false);
    emergencyCard->setStyleSheet("QFrame { background: rgba(231,76,60,0.08); border: 1px solid rgba(231,76,60,0.35); border-radius: 14px; }");
    auto* emergencyLayout = new QVBoxLayout(emergencyCard);
    emergencyLayout->setContentsMargins(16, 14, 16, 14);
    auto* emergencyTitle = new QLabel("EMERGENCY MODE", emergencyCard);
    emergencyTitle->setStyleSheet("font-size: 13px; color:#FF6B6B; font-weight:950;");
    m_emergencyStatusLabel = new QLabel("Status: Standby", emergencyCard);
    m_lastEmergencyLabel = new QLabel("Last Emergency: --", emergencyCard);
    auto* emergencyButton = new QPushButton("🚨 ACTIVATE HIGH PRIORITY", emergencyCard);
    emergencyButton->setStyleSheet("QPushButton { background:#C0392B; color:#FFFFFF; border:none; border-radius:9px; padding:13px; font-weight:950; } QPushButton:hover { background:#E74C3C; }");
    connect(emergencyButton, &QPushButton::clicked, this, &DashboardView::emergencyRequested);
    emergencyLayout->addWidget(emergencyTitle);
    emergencyLayout->addWidget(m_emergencyStatusLabel);
    emergencyLayout->addWidget(m_lastEmergencyLabel);
    emergencyLayout->addWidget(emergencyButton);
    metricGrid->addWidget(emergencyCard, 1, 3);
    mainLayout->addLayout(metricGrid);
}

ModernCard* DashboardView::createStatusCard(const QString& title, QLabel** targetLabel, const QString& color) {
    auto* card = new ModernCard(this);
    card->setMinimumHeight(118);
    card->setHoverEffect(false);
    card->setStyleSheet("QFrame { background: rgba(255,255,255,0.035); border: 1px solid rgba(255,255,255,0.08); border-radius: 14px; }");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(QString("font-size: 11px; color:%1; font-weight:900; letter-spacing:.5px;").arg(color));
    *targetLabel = new QLabel("--", card);
    (*targetLabel)->setWordWrap(true);
    (*targetLabel)->setStyleSheet("font-size: 11px; color:#DDE7F0; line-height: 140%;");
    layout->addWidget(titleLabel);
    layout->addWidget(*targetLabel, 1);
    return card;
}

ModernCard* DashboardView::createMetricCard(const QString& title, QLabel** targetLabel, const QString& icon, const QString& color) {
    auto* card = new ModernCard(this);
    card->setMinimumHeight(102);
    card->setStyleSheet("QFrame { background: rgba(255,255,255,0.035); border: 1px solid rgba(255,255,255,0.08); border-radius: 14px; }");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(15, 12, 15, 12);
    auto* top = new QHBoxLayout();
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("font-size: 9px; color:#A1AAB3; font-weight:900;");
    auto* iconLabel = new QLabel(icon, card);
    iconLabel->setStyleSheet(QString("font-size: 18px; color:%1;").arg(color));
    top->addWidget(titleLabel, 1);
    top->addWidget(iconLabel);
    *targetLabel = new QLabel("0", card);
    (*targetLabel)->setStyleSheet(QString("font-size: 25px; color:%1; font-weight:950;").arg(color));
    layout->addLayout(top);
    layout->addWidget(*targetLabel);
    return card;
}

void DashboardView::refreshData() {
    DatabaseManager& db = DatabaseManager::instance();
    const QList<Patient> patients = db.getPatients();
    int reportCount = 0;
    int voiceCount = 0;
    int emergencyCount = 0;
    int scanCount = 0;
    QString lastReport = "--";
    QString lastEmergency = "--";
    for (const Patient& patient : patients) {
        scanCount += db.getVisitsForPatient(patient.patientId).size();
        const auto reports = db.getReportsForPatient(patient.patientId);
        reportCount += reports.size();
        if (!reports.isEmpty() && (lastReport == "--" || reports.first().generatedAt > lastReport)) {
            lastReport = reports.first().generatedAt;
        }
        voiceCount += db.getVoiceNotesForPatient(patient.patientId).size();
        const auto emergencies = db.getEmergencyEventsForPatient(patient.patientId);
        emergencyCount += emergencies.size();
        if (!emergencies.isEmpty() && (lastEmergency == "--" || emergencies.first().timestamp > lastEmergency)) {
            lastEmergency = emergencies.first().timestamp;
        }
    }

    m_totalPatientsLabel->setText(QString::number(patients.size()));
    m_todayVisitsLabel->setText(QString::number(db.getVisitCountToday()));
    m_scansLabel->setText(QString::number(scanCount));
    m_reportsLabel->setText(QString::number(reportCount));
    m_voiceNotesLabel->setText(QString::number(voiceCount));
    m_emergencyCasesLabel->setText(QString::number(emergencyCount));
    m_followupsLabel->setText(QString::number(db.getPendingFollowUps().size()));
    m_githubLabel->setText(QString("Online: configured\nLast Report: %1\nStatus: GitHub Pages URL mode").arg(lastReport));
    m_lastEmergencyLabel->setText("Last Emergency: " + lastEmergency);

    m_activityLog->clear();
    const auto logs = db.getRecentLogs(8);
    for (const auto& entry : logs) {
        const QString time = QDateTime::fromString(entry.value("timestamp"), Qt::ISODate).toString("hh:mm");
        m_activityLog->addItem(QString("%1  %2").arg(time.isEmpty() ? "--:--" : time, entry.value("action")));
    }
    refreshSystemStatus();
}

void DashboardView::setOperatorName(const QString& name) {
    if (!name.trimmed().isEmpty()) m_operatorName = name;
    if (m_operatorLabel) m_operatorLabel->setText("Current Operator: " + m_operatorName);
}

void DashboardView::setActivePatient(const QString& patientName, const QString& patientId) {
    m_activePatientDisplay = patientId.isEmpty()
                                 ? QStringLiteral("No patient selected")
                                 : QString("%1 (%2)").arg(patientName, patientId);
    if (m_patientLabel) m_patientLabel->setText("Current Patient: " + m_activePatientDisplay);
    appendActivity("Patient Loaded: " + m_activePatientDisplay);
}

void DashboardView::setEmergencyMode(bool enabled) {
    m_emergencyMode = enabled;
    if (enabled) {
        m_emergencyFlashTimer->start();
        m_emergencyStatusLabel->setText("Status: HIGH PRIORITY ACTIVE");
        appendActivity("Emergency Mode Activated");
    } else {
        m_emergencyFlashTimer->stop();
        m_headerBadge->setText("SYSTEM NOMINAL");
        m_headerBadge->setStyleSheet("background: rgba(46,204,113,0.14); color:#2ECC71; border:1px solid rgba(46,204,113,0.45); border-radius:12px; padding:10px; font-weight:900;");
        m_emergencyStatusLabel->setText("Status: Standby");
    }
}

void DashboardView::refreshSystemStatus() {
    CommunicationManager& comm = CommunicationManager::instance();
    CameraSourceManager& camera = CameraSourceManager::instance();
    const qint64 heartbeatAge = comm.lastHeartbeatAgeMs();
    m_arduinoLabel->setText(QString("State: %1\nPort: %2\nFirmware: %3\nLast Heartbeat: %4 ms\nButton Activity: JSON events\nJoystick Status: synchronized")
                                .arg(comm.isArduinoConnected() ? "Connected" : "Disconnected",
                                     comm.currentSerialPort().isEmpty() ? "AUTO" : comm.currentSerialPort(),
                                     comm.firmwareVersion(),
                                     heartbeatAge < 0 ? "--" : QString::number(heartbeatAge)));
    const bool fallback = camera.activeSource() == CameraProvider::Source::Laptop;
    m_espLabel->setText(QString("Source: %1\nStatus: %2\nAI Engine: %3\nESP32: %4\nMode: %5")
                            .arg(camera.activeSourceName(),
                                 camera.isStreaming() ? "Scanning" : "Starting",
                                 camera.isStreaming() ? "Running" : "Ready",
                                 camera.esp32Available() ? "Connected" : "Disconnected",
                                 fallback ? "Fallback Active" : camera.modeName()));
    m_oledLabel->setText(QString("Connected: %1\nCurrent Screen: workflow mirror\nBrightness: Auto\nSync: %2")
                             .arg(comm.isArduinoConnected() ? "Yes" : "No",
                                  comm.isArduinoConnected() ? "Synchronized" : "Waiting for Nano"));
    m_voiceLabel->setText("Ready: Yes\nMode: Ready / Speaking\nClinical Notes: Button 2 controlled");
    m_databaseLabel->setText(QString("Connected: Yes\nTotal Patients: %1\nLast Backup: see audit logs")
                                 .arg(DatabaseManager::instance().getPatientCount()));

    const bool healthy = camera.isStreaming() && comm.isArduinoConnected();
    if (m_emergencyMode) {
        m_overallHealthLabel->setText("Overall System Health: RED - EMERGENCY");
        m_overallHealthLabel->setStyleSheet("font-weight:900; color:#E74C3C;");
    } else if (healthy) {
        m_overallHealthLabel->setText("Overall System Health: GREEN");
        m_overallHealthLabel->setStyleSheet("font-weight:900; color:#2ECC71;");
    } else {
        m_overallHealthLabel->setText("Overall System Health: YELLOW - DEVICE WAITING");
        m_overallHealthLabel->setStyleSheet("font-weight:900; color:#F1C40F;");
    }
}

void DashboardView::handleWorkflowStateChanged(WorkflowEngine::DeviceState state, const QString& reason) {
    m_workflowStageLabel->setText("Current Stage: " + WorkflowEngine::instance().deviceStateToString(state));
    m_workflowProgress->setValue(workflowProgressForState(state));
    if (!reason.isEmpty()) appendActivity(reason);
}

void DashboardView::toggleEmergencyBanner() {
    if (!m_emergencyMode) return;
    m_bannerFlash = !m_bannerFlash;
    m_headerBadge->setText("🚨 EMERGENCY HIGH PRIORITY");
    m_headerBadge->setStyleSheet(m_bannerFlash
        ? "background:#C0392B; color:#FFFFFF; border:1px solid #FF6B6B; border-radius:12px; padding:10px; font-weight:950;"
        : "background:rgba(231,76,60,0.16); color:#FF6B6B; border:1px solid rgba(231,76,60,0.55); border-radius:12px; padding:10px; font-weight:950;");
}

void DashboardView::appendActivity(const QString& message) {
    if (!m_activityLog) return;
    m_activityLog->insertItem(0, QDateTime::currentDateTime().toString("hh:mm") + "  " + message);
    while (m_activityLog->count() > 18) delete m_activityLog->takeItem(m_activityLog->count() - 1);
}

int DashboardView::workflowProgressForState(WorkflowEngine::DeviceState state) const {
    switch (state) {
        case WorkflowEngine::DeviceState::Idle: return 5;
        case WorkflowEngine::DeviceState::PatientSelected: return 18;
        case WorkflowEngine::DeviceState::DigitalTwinReady: return 30;
        case WorkflowEngine::DeviceState::ScanReady: return 42;
        case WorkflowEngine::DeviceState::Scanning: return 58;
        case WorkflowEngine::DeviceState::ScanCompleted: return 72;
        case WorkflowEngine::DeviceState::VoiceRecording: return 82;
        case WorkflowEngine::DeviceState::ReportReady: return 100;
        case WorkflowEngine::DeviceState::Emergency: return 100;
    }
    return 0;
}
