#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QWidget>
#include <QHash>
#include "../core/CameraStreamManager.h"
#include "../core/CameraSourceManager.h"

#include "../core/VoiceAssistant.h"

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(QWidget* parent = nullptr);
    ~SettingsView() override;

    void setCurrentUser(const QString& username, const QString& role, const QString& fullName);
    void setActive(bool active);

signals:
    void languageChanged(VoiceAssistant::Language lang);
    void logoutRequested();

private slots:
    void handleLanguageChange(int idx);
    void handleVolumeChange(int value);
    void handleSpeedChange(int value);
    void handleVoiceToggle(bool checked);
    void refreshStatus();
    void handleCompleteBackup();
    void handleCompleteRestore();
    void handleDatabaseReset();
    void updateCameraFrame(const QImage& frame);
    void updateCameraStatus(CameraStreamManager::State state, const QString& detail);
    void captureSnapshot();
    void showCameraFullscreen();
    void handleCameraSourceChange(int index);

private:
    QLabel* m_accountName = nullptr;
    QLabel* m_accountUsername = nullptr;
    QLabel* m_accountRole = nullptr;
    QLabel* m_lastLogin = nullptr;
    QComboBox* m_langCombo = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QSlider* m_speedSlider = nullptr;
    QCheckBox* m_voiceCheck = nullptr;
    QComboBox* m_inputDevice = nullptr;
    QComboBox* m_outputDevice = nullptr;
    QLineEdit* m_espIpEdit = nullptr;
    QLineEdit* m_espPortEdit = nullptr;
    QComboBox* m_cameraSourceCombo = nullptr;
    QLabel* m_arduinoStatus = nullptr;
    QLabel* m_espStatus = nullptr;
    QLabel* m_wifiStatus = nullptr;
    QLabel* m_cameraHardwareStatus = nullptr;
    QLabel* m_oledStatus = nullptr;
    QLabel* m_inputStatus = nullptr;
    QLabel* m_databaseInfo = nullptr;
    QLineEdit* m_githubRepoEdit = nullptr;
    QLineEdit* m_githubPagesEdit = nullptr;
    QLineEdit* m_reportDoctorEdit = nullptr;
    QLineEdit* m_exportFolderEdit = nullptr;
    QLabel* m_applicationInfo = nullptr;
    QLabel* m_cameraPreview = nullptr;
    QLabel* m_cameraConnection = nullptr;
    QLabel* m_cameraMetrics = nullptr;
    QLabel* m_cameraDiagnostics = nullptr;
    QLabel* m_hardwareTests = nullptr;
    QPushButton* m_backupBtn = nullptr;
    QPushButton* m_restoreBtn = nullptr;
    QPushButton* m_resetDbBtn = nullptr;
    QTimer* m_refreshTimer = nullptr;
    bool m_databaseActionInProgress = false;
    bool m_active = false;
    QHash<QString, QPushButton*> m_testButtons;

    void setupUi();
    void loadSettings();
    void setHardwareTestResult(const QString& key, bool passed, const QString& detail = {});
};

#endif // SETTINGSVIEW_H
