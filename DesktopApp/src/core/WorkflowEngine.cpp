#include "WorkflowEngine.h"
#include "VoiceAssistant.h"
#include <QDebug>

WorkflowEngine& WorkflowEngine::instance() {
    static WorkflowEngine inst;
    return inst;
}

WorkflowEngine::WorkflowEngine(QObject* parent) : QObject(parent) {
    resetWorkflow();
}

WorkflowEngine::~WorkflowEngine() {}

void WorkflowEngine::init() {
    resetWorkflow();
}

void WorkflowEngine::resetWorkflow() {
    m_currentStage = OperatorLogin;
    m_deviceState = DeviceState::Idle;
    m_stateBeforeVoice = DeviceState::Idle;
    m_stateBeforeEmergency = DeviceState::Idle;
    m_stageCompletion.clear();
    
    // Set all stages to false initially
    for (int i = OperatorLogin; i <= WorkflowCompleted; ++i) {
        m_stageCompletion[static_cast<Stage>(i)] = false;
    }
    
    emit stageChanged(m_currentStage);
    emit deviceStateChanged(m_deviceState, QStringLiteral("Workflow reset"));
    emit progressChanged(0);
}

QString WorkflowEngine::deviceStateToString(DeviceState state) const {
    switch (state) {
        case DeviceState::Idle: return "IDLE";
        case DeviceState::PatientSelected: return "PATIENT_SELECTED";
        case DeviceState::DigitalTwinReady: return "DIGITAL_TWIN_READY";
        case DeviceState::ScanReady: return "SCAN_READY";
        case DeviceState::Scanning: return "SCANNING";
        case DeviceState::ScanCompleted: return "SCAN_COMPLETED";
        case DeviceState::VoiceRecording: return "VOICE_RECORDING";
        case DeviceState::ReportReady: return "REPORT_READY";
        case DeviceState::Emergency: return "EMERGENCY";
    }
    return "UNKNOWN";
}

bool WorkflowEngine::canGenerateReport() const {
    return m_deviceState == DeviceState::ScanCompleted ||
           m_deviceState == DeviceState::ReportReady ||
           m_stateBeforeVoice == DeviceState::ScanCompleted ||
           m_stateBeforeVoice == DeviceState::ReportReady;
}

bool WorkflowEngine::isScanning() const {
    return m_deviceState == DeviceState::Scanning;
}

bool WorkflowEngine::transitionTo(DeviceState state, const QString& reason) {
    if (state == DeviceState::Emergency) {
        if (m_deviceState != DeviceState::Emergency) m_stateBeforeEmergency = m_deviceState;
        m_deviceState = state;
        emit deviceStateChanged(m_deviceState, reason);
        return true;
    }

    if (m_deviceState == DeviceState::Emergency && state != DeviceState::Idle) {
        m_deviceState = state;
        emit deviceStateChanged(m_deviceState, reason);
        return true;
    }

    const bool allowed =
        state == m_deviceState ||
        (m_deviceState == DeviceState::Idle && state == DeviceState::PatientSelected) ||
        (m_deviceState == DeviceState::PatientSelected && (state == DeviceState::DigitalTwinReady || state == DeviceState::ScanReady)) ||
        (m_deviceState == DeviceState::DigitalTwinReady && state == DeviceState::ScanReady) ||
        (m_deviceState == DeviceState::PatientSelected && state == DeviceState::Scanning) ||
        (m_deviceState == DeviceState::DigitalTwinReady && state == DeviceState::Scanning) ||
        (m_deviceState == DeviceState::ScanCompleted && state == DeviceState::Scanning) ||
        (m_deviceState == DeviceState::ReportReady && state == DeviceState::Scanning) ||
        (m_deviceState == DeviceState::ScanReady && state == DeviceState::Scanning) ||
        (m_deviceState == DeviceState::Scanning && state == DeviceState::ScanCompleted) ||
        (m_deviceState == DeviceState::ScanCompleted && state == DeviceState::ReportReady) ||
        (m_deviceState == DeviceState::ReportReady && state == DeviceState::ScanReady) ||
        (state == DeviceState::VoiceRecording);

    if (!allowed) {
        emit invalidTransition(QStringLiteral("Blocked transition %1 -> %2 (%3)")
                                   .arg(deviceStateToString(m_deviceState),
                                        deviceStateToString(state),
                                        reason));
        return false;
    }

    if (state == DeviceState::VoiceRecording) {
        if (m_deviceState != DeviceState::VoiceRecording) m_stateBeforeVoice = m_deviceState;
    }
    m_deviceState = state;
    emit deviceStateChanged(m_deviceState, reason);
    return true;
}

void WorkflowEngine::selectPatient() {
    transitionTo(DeviceState::PatientSelected, QStringLiteral("Patient selected"));
    setStage(LoadDigitalTwin);
}

void WorkflowEngine::markDigitalTwinReady() {
    transitionTo(DeviceState::DigitalTwinReady, QStringLiteral("Digital twin loaded"));
}

void WorkflowEngine::startScan() {
    if (m_deviceState == DeviceState::Idle) {
        emit invalidTransition(QStringLiteral("Select a patient before starting scan."));
        return;
    }
    transitionTo(DeviceState::Scanning, QStringLiteral("Button 1 start scan"));
    setStage(StartScreening);
}

void WorkflowEngine::completeScan() {
    transitionTo(DeviceState::ScanCompleted, QStringLiteral("Demonstration scan completed"));
    setStage(GenerateReport);
}

void WorkflowEngine::startVoiceRecording() {
    transitionTo(DeviceState::VoiceRecording, QStringLiteral("Button 2 voice notes start"));
}

void WorkflowEngine::stopVoiceRecording() {
    if (m_deviceState == DeviceState::VoiceRecording) {
        m_deviceState = m_stateBeforeVoice;
        emit deviceStateChanged(m_deviceState, QStringLiteral("Button 2 voice notes stop"));
    }
}

void WorkflowEngine::markReportReady() {
    transitionTo(DeviceState::ReportReady, QStringLiteral("Button 3 report ready"));
}

void WorkflowEngine::activateEmergency() {
    transitionTo(DeviceState::Emergency, QStringLiteral("Button 4 emergency"));
}

void WorkflowEngine::clearEmergency() {
    if (m_deviceState == DeviceState::Emergency) {
        m_deviceState = m_stateBeforeEmergency;
        emit deviceStateChanged(m_deviceState, QStringLiteral("Emergency view acknowledged"));
    }
}

void WorkflowEngine::nextStage() {
    if (m_currentStage == WorkflowCompleted) return;

    m_stageCompletion[m_currentStage] = true;
    m_currentStage = static_cast<Stage>(m_currentStage + 1);
    
    // Voice announcements based on workflow state transition
    if (m_currentStage == PatientRegistration) {
        VoiceAssistant::instance().speak("RegisterPatient");
    } else if (m_currentStage == CameraQualityCheck) {
        VoiceAssistant::instance().speak("CameraReady");
    } else if (m_currentStage == StartScreening) {
        VoiceAssistant::instance().speak("ScanStarted");
    } else if (m_currentStage == GenerateReport) {
        VoiceAssistant::instance().speak("GeneratingReport");
    } else if (m_currentStage == SyncDatabase) {
        VoiceAssistant::instance().speak("Synced");
    }

    emit stageChanged(m_currentStage);
    emit progressChanged(progressPercent());

    if (m_currentStage == WorkflowCompleted) {
        emit workflowCompleted();
    }
}

void WorkflowEngine::prevStage() {
    if (m_currentStage == OperatorLogin) return;

    m_stageCompletion[m_currentStage] = false;
    m_currentStage = static_cast<Stage>(m_currentStage - 1);
    m_stageCompletion[m_currentStage] = false; // invalidate previous step too since we went back
    
    emit stageChanged(m_currentStage);
    emit progressChanged(progressPercent());
}

void WorkflowEngine::setStage(Stage stage) {
    m_currentStage = stage;
    
    // Invalidate everything after this stage
    for (int i = stage; i <= WorkflowCompleted; ++i) {
        m_stageCompletion[static_cast<Stage>(i)] = false;
    }
    
    // Mark previous stages as completed
    for (int i = OperatorLogin; i < stage; ++i) {
        m_stageCompletion[static_cast<Stage>(i)] = true;
    }

    emit stageChanged(m_currentStage);
    emit progressChanged(progressPercent());
}

bool WorkflowEngine::isStageCompleted(Stage stage) const {
    return m_stageCompletion.value(stage, false);
}

int WorkflowEngine::progressPercent() const {
    if (m_currentStage == WorkflowCompleted) return 100;
    
    int completedCount = 0;
    for (int i = OperatorLogin; i < WorkflowCompleted; ++i) {
        if (m_stageCompletion.value(static_cast<Stage>(i), false)) {
            completedCount++;
        }
    }
    
    // There are 10 active steps before completion
    return (completedCount * 100) / 10;
}

QString WorkflowEngine::stageToString(Stage stage) const {
    switch (stage) {
        case OperatorLogin: return "Operator Login";
        case PatientRegistration: return "Patient Registration";
        case QueueAssignment: return "Queue Assignment";
        case LoadDigitalTwin: return "Load Digital Twin";
        case CameraQualityCheck: return "Camera Quality Check";
        case StartScreening: return "Screening Scan";
        case GenerateReport: return "Generate Report";
        case SaveDatabase: return "Save Records";
        case ScheduleFollowUp: return "Schedule Follow Up";
        case SyncDatabase: return "Database Sync";
        case WorkflowCompleted: return "Completed";
    }
    return "Unknown";
}

QString WorkflowEngine::stageDescription(Stage stage) const {
    switch (stage) {
        case OperatorLogin: return "Authenticate clinical operator or technician credentials.";
        case PatientRegistration: return "Input patient demographics, take photo, and register record.";
        case QueueAssignment: return "Generate QR code and assign priority queue number.";
        case LoadDigitalTwin: return "Retrieve and load previous medical profile and digital twin status.";
        case CameraQualityCheck: return "Verify ESP32-CAM stream framing, lighting and focus levels.";
        case StartScreening: return "Run the assisted screening simulation on selected body regions.";
        case GenerateReport: return "Compile clinical logs, metrics, and screenshots into a PDF report.";
        case SaveDatabase: return "Commit the session visit record and digital twin updates to local SQLite.";
        case ScheduleFollowUp: return "Assign outpatient checkup visits and update preventive coordinator.";
        case SyncDatabase: return "Push local changes to centralized server and verify logs.";
        case WorkflowCompleted: return "Workstation screening session completed successfully.";
    }
    return "";
}
