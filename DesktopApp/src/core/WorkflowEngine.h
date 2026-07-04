#ifndef WORKFLOWENGINE_H
#define WORKFLOWENGINE_H

#include <QObject>
#include <QMap>

class WorkflowEngine : public QObject {
    Q_OBJECT
public:
    static WorkflowEngine& instance();

    enum Stage {
        OperatorLogin,
        PatientRegistration,
        QueueAssignment,
        LoadDigitalTwin,
        CameraQualityCheck,
        StartScreening,
        GenerateReport,
        SaveDatabase,
        ScheduleFollowUp,
        SyncDatabase,
        WorkflowCompleted
    };

    enum class DeviceState {
        Idle,
        PatientSelected,
        DigitalTwinReady,
        ScanReady,
        Scanning,
        ScanCompleted,
        VoiceRecording,
        ReportReady,
        Emergency
    };
    Q_ENUM(DeviceState)

    void init();
    Stage currentStage() const { return m_currentStage; }
    QString stageToString(Stage stage) const;
    QString stageDescription(Stage stage) const;
    DeviceState deviceState() const { return m_deviceState; }
    QString deviceStateToString(DeviceState state) const;
    bool canGenerateReport() const;
    bool isScanning() const;
    
    void nextStage();
    void prevStage();
    void setStage(Stage stage);
    void resetWorkflow();
    bool transitionTo(DeviceState state, const QString& reason = QString());
    void selectPatient();
    void markDigitalTwinReady();
    void startScan();
    void completeScan();
    void startVoiceRecording();
    void stopVoiceRecording();
    void markReportReady();
    void activateEmergency();
    void clearEmergency();
    
    bool isStageCompleted(Stage stage) const;
    int progressPercent() const;

signals:
    void stageChanged(WorkflowEngine::Stage stage);
    void progressChanged(int percent);
    void workflowCompleted();
    void deviceStateChanged(WorkflowEngine::DeviceState state, const QString& reason);
    void invalidTransition(const QString& reason);

private:
    WorkflowEngine(QObject* parent = nullptr);
    ~WorkflowEngine();
    WorkflowEngine(const WorkflowEngine&) = delete;
    WorkflowEngine& operator=(const WorkflowEngine&) = delete;

    Stage m_currentStage = OperatorLogin;
    DeviceState m_deviceState = DeviceState::Idle;
    DeviceState m_stateBeforeVoice = DeviceState::Idle;
    DeviceState m_stateBeforeEmergency = DeviceState::Idle;
    QMap<Stage, bool> m_stageCompletion;
};

#endif // WORKFLOWENGINE_H
