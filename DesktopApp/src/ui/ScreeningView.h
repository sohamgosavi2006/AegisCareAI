#ifndef SCREENINGVIEW_H
#define SCREENINGVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QTimer>
#include <QCheckBox>
#include "../core/CameraQualityEngine.h"

class ScreeningView : public QWidget {
    Q_OBJECT
public:
    explicit ScreeningView(QWidget* parent = nullptr);
    virtual ~ScreeningView();

    void setPatient(const QString& patientId);
    void selectOrganRegion(const QString& regionName);
    bool isScanning() const { return m_isScanningActive; }
    bool hasCompletedScan() const { return m_scanCompleted; }
    void setActive(bool active);

public slots:
    void startScreeningScan();
    void stopScreeningScan();
    void moveScanTarget(const QString& action);

private slots:
    void handleFrameReceived(const QImage& frame);
    void handleQualityStats(const CameraQualityEngine::QualityStats& stats);
    void handleSensorData(int hr, int spo2, double temp, int sys, int dia);
    
    // Core screening routines
    void updateScanProgress();
    void toggleEmulationMode(bool checked);
    void handleHardwareConnect();

private:
    // Center: Camera Viewfinder
    QLabel* m_viewfinder = nullptr;
    QLabel* m_cameraOverlay = nullptr;
    QImage m_currentFrame;

    // Left Panel: Screening Workflow Checklist
    QListWidget* m_checklistWidget = nullptr;
    QProgressBar* m_scanProgressBar = nullptr;
    QPushButton* m_startScanBtn = nullptr;
    QLabel* m_organLabel = nullptr;

    // Right Panel: Live Sensor Metrics
    QLabel* m_hrLabel = nullptr;
    QLabel* m_spo2Label = nullptr;
    QLabel* m_tempLabel = nullptr;
    QLabel* m_bpLabel = nullptr;

    // Camera Quality Overlay Metrics
    QLabel* m_lightScoreLabel = nullptr;
    QLabel* m_clarityScoreLabel = nullptr;
    QLabel* m_motionScoreLabel = nullptr;
    QLabel* m_overallScoreLabel = nullptr;
    QLabel* m_guidanceBanner = nullptr;

    // Buttons
    QPushButton* m_connectHardwareBtn = nullptr;
    QCheckBox* m_emulationCheck = nullptr;

    // Scanning states
    QString m_patientId;
    QString m_targetOrgan = "Heart";
    
    QTimer* m_scanTimer = nullptr;
    int m_scanProgressCounter = 0;
    int m_frameCounter = 0;
    bool m_isScanningActive = false;
    bool m_scanCompleted = false;
    bool m_targetLocked = false;
    bool m_active = false;
    QPoint m_targetOffset = QPoint(0, 0);

    // Cached values
    int m_cachedHR = 72;
    int m_cachedSPO2 = 98;
    double m_cachedTemp = 98.4;
    int m_cachedSys = 118;
    int m_cachedDia = 78;

    void setupUi();
    void initChecklist();
    void updateChecklistItem(int index, bool completed);
    void commitVisitData();
    void renderFrameWithOverlay();

signals:
    void scanStarted();
    void scanStopped();
    void scanCompleted();
};

#endif // SCREENINGVIEW_H
