#ifndef DASHBOARDVIEW_H
#define DASHBOARDVIEW_H

#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "../core/WorkflowEngine.h"
#include "components/ModernCard.h"

class DashboardView : public QWidget {
    Q_OBJECT
public:
    explicit DashboardView(QWidget* parent = nullptr);
    ~DashboardView() override;

    void refreshData();
    void setOperatorName(const QString& name);
    void setActivePatient(const QString& patientName, const QString& patientId);
    void setEmergencyMode(bool enabled);
    void setActive(bool active);

signals:
    void emergencyRequested();

private slots:
    void refreshSystemStatus();
    void handleWorkflowStateChanged(WorkflowEngine::DeviceState state, const QString& reason);
    void toggleEmergencyBanner();

private:
    QLabel* m_headerBadge = nullptr;
    QLabel* m_overallHealthLabel = nullptr;
    QLabel* m_operatorLabel = nullptr;
    QLabel* m_patientLabel = nullptr;
    QLabel* m_workflowStageLabel = nullptr;
    QProgressBar* m_workflowProgress = nullptr;
    QListWidget* m_activityLog = nullptr;
    QLabel* m_emergencyStatusLabel = nullptr;
    QLabel* m_lastEmergencyLabel = nullptr;
    QLabel* m_arduinoLabel = nullptr;
    QLabel* m_espLabel = nullptr;
    QLabel* m_oledLabel = nullptr;
    QLabel* m_voiceLabel = nullptr;
    QLabel* m_databaseLabel = nullptr;
    QLabel* m_githubLabel = nullptr;
    QLabel* m_totalPatientsLabel = nullptr;
    QLabel* m_todayVisitsLabel = nullptr;
    QLabel* m_scansLabel = nullptr;
    QLabel* m_reportsLabel = nullptr;
    QLabel* m_voiceNotesLabel = nullptr;
    QLabel* m_emergencyCasesLabel = nullptr;
    QLabel* m_followupsLabel = nullptr;
    QTimer* m_statusTimer = nullptr;
    QTimer* m_emergencyFlashTimer = nullptr;
    bool m_emergencyMode = false;
    bool m_bannerFlash = false;
    QString m_operatorName = QStringLiteral("Dr. Soham");
    QString m_activePatientDisplay = QStringLiteral("No patient selected");

    void setupUi();
    ModernCard* createStatusCard(const QString& title, QLabel** targetLabel, const QString& color);
    ModernCard* createMetricCard(const QString& title, QLabel** targetLabel, const QString& icon, const QString& color);
    void appendActivity(const QString& message);
    int workflowProgressForState(WorkflowEngine::DeviceState state) const;
};

#endif // DASHBOARDVIEW_H
