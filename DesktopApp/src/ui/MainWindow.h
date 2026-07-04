#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QTimer>
#include "../core/WorkflowEngine.h"
#include "../core/VoiceAssistant.h"
#include "../models/Models.h"

// Forward declarations of views
class DashboardView;
class PatientManagementView;
class DigitalTwinView;
class ScreeningView;
class TelemedicineView;
class SettingsView;
class ClinicalVoiceNotesPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(int sidebarWidth READ sidebarWidth WRITE setSidebarWidth)

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    void setOperatorInfo(const QString& username, const QString& role, const QString& fullName);
    void selectActivePatient(const QString& patientId);
    QString activePatientId() const { return m_activePatientId; }

    int sidebarWidth() const { return m_sidebarWidth; }
    void setSidebarWidth(int width);

private slots:
    void handleNavigation(int index);
    void toggleSidebar();
    void updateHardwareStatus();
    void updateWorkflowProgress(WorkflowEngine::Stage stage);
    void handleVoiceSpeech(const QString& text);
    void refreshVisibleView();
    void handleControllerButton(int buttonNum);
    void handleControllerJoystick(const QString& direction);
    void handleScanControl(const QString& action);
    void showDeveloperAbout();
    void handleVoiceNoteSaved(const VoiceNote& note);
    void handleScanCompleted();

private:
    // UI layout components
    QWidget* m_sidebar = nullptr;
    QPushButton* m_expandRailButton = nullptr;
    QPushButton* m_collapseButton = nullptr;
    QListWidget* m_navList = nullptr;
    QStackedWidget* m_viewStack = nullptr;
    
    // Status Bar HUD elements
    QLabel* m_operatorLabel = nullptr;
    QLabel* m_patientLabel = nullptr;
    QLabel* m_espStatusDot = nullptr;
    QLabel* m_arduinoStatusDot = nullptr;
    QLabel* m_networkStatusLabel = nullptr;
    QLabel* m_voiceAssistantFeedback = nullptr;
    
    // Workflow Timeline elements
    QWidget* m_timelineWidget = nullptr;
    QLabel* m_currentStepLabel = nullptr;
    QProgressBar* m_workflowProgress = nullptr;
    
    // View references
    DashboardView* m_dashboardView = nullptr;
    PatientManagementView* m_patientView = nullptr;
    DigitalTwinView* m_digitalTwinView = nullptr;
    ScreeningView* m_screeningView = nullptr;
    TelemedicineView* m_telemedicineView = nullptr;
    SettingsView* m_settingsView = nullptr;
    ClinicalVoiceNotesPanel* m_voiceNotesPanel = nullptr;

    // Operator and Patient state
    QString m_username;
    QString m_role;
    QString m_operatorFullName;
    QString m_activePatientId;

    // Sidebar animation properties
    int m_sidebarWidth = 240;
    bool m_sidebarCollapsed = false;
    QPropertyAnimation* m_sidebarAnim = nullptr;
    QTimer* m_databaseRefreshTimer = nullptr;
    bool m_navigationInProgress = false;
    bool m_emergencyViewActive = false;

    void setupUi();
    void createViews();
    void setupStatusHUD();
    void setupTimelineBar();
    void generatePatientReport();
    void activateEmergencyMode();
    void sendOledStatus(const QString& status);
    void applyLanguage(VoiceAssistant::Language language);
    void showCameraToast(const QString& message, const QString& tone);
};

#endif // MAINWINDOW_H
