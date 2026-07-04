#include "ScreeningView.h"
#include "../core/CommunicationManager.h"
#include "../core/CameraStreamManager.h"
#include "../core/CameraSourceManager.h"
#include "../core/VoiceAssistant.h"
#include "../core/WorkflowEngine.h"
#include "../core/DatabaseManager.h"
#include "components/ModernCard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QDateTime>
#include <QMessageBox>

ScreeningView::ScreeningView(QWidget* parent) : QWidget(parent) {
    setupUi();
    initChecklist();

    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, &QTimer::timeout, this, &ScreeningView::updateScanProgress);

    // Connect to CommunicationManager hardware links
    CommunicationManager& comms = CommunicationManager::instance();
    connect(&comms, &CommunicationManager::frameReceived, this, [this](const QImage& frame) {
        if (m_emulationCheck && m_emulationCheck->isChecked()) handleFrameReceived(frame);
    });
    connect(&comms, &CommunicationManager::sensorDataReceived, this, &ScreeningView::handleSensorData);
    
    // Connect to Quality Engine
    connect(&CameraQualityEngine::instance(), &CameraQualityEngine::qualityAnalyzed, this, &ScreeningView::handleQualityStats);
    connect(&CameraSourceManager::instance(), &CameraSourceManager::frameReady,
            this, &ScreeningView::handleFrameReceived);
    connect(&CameraSourceManager::instance(), &CameraSourceManager::sourceChanged, this,
            [this](CameraProvider::Source source, const QString& reason) {
        m_currentFrame = QImage();
        m_viewfinder->clear();
        m_viewfinder->setText("SWITCHING CAMERA SOURCE...");
        if (!m_active || m_isScanningActive) return;
        m_guidanceBanner->setText(QString("%1 • %2 • EDUCATIONAL DEMONSTRATION ONLY")
                                      .arg(CameraSourceManager::sourceName(source).toUpper(), reason));
    });
}

ScreeningView::~ScreeningView() {}

void ScreeningView::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(14);

    // ==========================================
    // LEFT PANEL: Progress Checklist
    // ==========================================
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(15);

    QLabel* leftTitle = new QLabel("AUTOMATED WORKFLOW TRACKER", this);
    leftTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #FFFFFF;");
    leftLayout->addWidget(leftTitle);

    ModernCard* checkCard = new ModernCard(this);
    checkCard->setHoverEffect(false);
    checkCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* checkLayout = new QVBoxLayout(checkCard);
    checkLayout->setContentsMargins(12, 12, 12, 12);
    checkLayout->setSpacing(8);

    m_organLabel = new QLabel("ACTIVE PROFILE: HEART SCAN", this);
    m_organLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    checkLayout->addWidget(m_organLabel);

    m_checklistWidget = new QListWidget(this);
    m_checklistWidget->setStyleSheet("background-color: rgba(0,0,0,50); border: 1px solid rgba(255,255,255,0.06); border-radius: 6px; padding: 5px; color: #E1E6EB;");
    checkLayout->addWidget(m_checklistWidget, 1);

    m_scanProgressBar = new QProgressBar(this);
    m_scanProgressBar->setValue(0);
    m_scanProgressBar->setFormat("Scan Idle %p%");
    checkLayout->addWidget(m_scanProgressBar);

    m_startScanBtn = new QPushButton("🩺 START SCREENING", this);
    m_startScanBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2ECC71; border: none; border-radius: 4px; padding: 10px; color: #FFFFFF; font-weight: bold; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #27AE60; }"
        "QPushButton:disabled { background-color: rgba(255, 255, 255, 0.05); color: #7f8c8d; }"
    );
    connect(m_startScanBtn, &QPushButton::clicked, this, &ScreeningView::startScreeningScan);
    checkLayout->addWidget(m_startScanBtn);

    leftLayout->addWidget(checkCard);
    mainLayout->addLayout(leftLayout, 4);

    // ==========================================
    // CENTER PANEL: Live Camera Stream HUD
    // ==========================================
    QVBoxLayout* centerLayout = new QVBoxLayout();
    centerLayout->setSpacing(12);

    QLabel* centerTitle = new QLabel("LIVE STREAM VIEWPORT HUD", this);
    centerTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #FFFFFF;");
    centerLayout->addWidget(centerTitle);

    m_cameraOverlay = new QLabel("SOURCE: SEARCHING  •  RESOLUTION: --  •  LIVE  •  AI PIPELINE READY  •  REC: STANDBY", this);
    m_cameraOverlay->setWordWrap(true);
    m_cameraOverlay->setStyleSheet(
        "background:rgba(9,27,43,.92); color:#DDEEFF; border:1px solid rgba(81,169,255,.34);"
        "border-radius:9px; padding:8px 12px; font-size:10px; font-weight:800;");
    centerLayout->addWidget(m_cameraOverlay);

    m_viewfinder = new QLabel(this);
    m_viewfinder->setMinimumSize(320, 240);
    m_viewfinder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_viewfinder->setAlignment(Qt::AlignCenter);
    m_viewfinder->setStyleSheet("background-color: #04080D; border: 2px solid rgba(0, 168, 255, 0.15); border-radius: 8px;");
    centerLayout->addWidget(m_viewfinder, 1);

    m_guidanceBanner = new QLabel("WAITING FOR HARDWARE CONNECTION...", this);
    m_guidanceBanner->setAlignment(Qt::AlignCenter);
    m_guidanceBanner->setWordWrap(true);
    m_guidanceBanner->setStyleSheet("background-color: rgba(241, 196, 15, 0.15); color: #F1C40F; border: 1px solid #F1C40F; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
    centerLayout->addWidget(m_guidanceBanner);

    mainLayout->addLayout(centerLayout, 6);

    // ==========================================
    // RIGHT PANEL: Quality Index & Sensors
    // ==========================================
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // Quality Indices Card
    ModernCard* qualCard = new ModernCard(this);
    qualCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    qualCard->setFixedHeight(160);
    
    QVBoxLayout* qualLayout = new QVBoxLayout(qualCard);
    qualLayout->setContentsMargins(15, 12, 15, 12);
    qualLayout->setSpacing(5);

    QLabel* qualHeader = new QLabel("SIGNAL QUALITY INDEX", this);
    qualHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    qualLayout->addWidget(qualHeader);

    m_lightScoreLabel = new QLabel("Luminance Level: --", this);
    m_clarityScoreLabel = new QLabel("Focus Clarity: --", this);
    m_motionScoreLabel = new QLabel("Sensor Stability: --", this);
    m_overallScoreLabel = new QLabel("Overall Integrity: --", this);
    m_overallScoreLabel->setStyleSheet("font-weight: bold; color: #FFFFFF; font-size: 12px;");

    qualLayout->addWidget(m_lightScoreLabel);
    qualLayout->addWidget(m_clarityScoreLabel);
    qualLayout->addWidget(m_motionScoreLabel);
    qualLayout->addWidget(m_overallScoreLabel);
    rightLayout->addWidget(qualCard);

    // Live Sensors Card
    ModernCard* sensorCard = new ModernCard(this);
    sensorCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* sensLayout = new QVBoxLayout(sensorCard);
    sensLayout->setContentsMargins(15, 15, 15, 15);
    sensLayout->setSpacing(10);

    QLabel* sensHeader = new QLabel("INTEGRATED SENSOR HUB", this);
    sensHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    sensLayout->addWidget(sensHeader);

    auto makeSensorRow = [&](const QString& label, const QString& id) {
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* title = new QLabel(label, this);
        title->setStyleSheet("font-size: 12px; color: #A1AAB3;");
        QLabel* val = new QLabel("--", this);
        val->setObjectName(id);
        val->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
        row->addWidget(title);
        row->addWidget(val, 0, Qt::AlignRight);
        sensLayout->addLayout(row);
        return val;
    };

    m_hrLabel = makeSensorRow("❤️ HEART RATE:", "hr");
    m_spo2Label = makeSensorRow("🫁 OXYGEN SpO2:", "spo2");
    m_tempLabel = makeSensorRow("🌡️ TEMPERATURE:", "temp");
    m_bpLabel = makeSensorRow("🩸 BLOOD PRESSURE:", "bp");

    sensLayout->addSpacing(10);

    // Emulation checkbox at bottom
    m_emulationCheck = new QCheckBox("Use synthetic test frames", this);
    m_emulationCheck->setChecked(false);
    connect(m_emulationCheck, &QCheckBox::toggled, this, &ScreeningView::toggleEmulationMode);
    sensLayout->addWidget(m_emulationCheck);

    m_connectHardwareBtn = new QPushButton("RETRY ESP32-CAM", this);
    m_connectHardwareBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 4px; padding: 6px; color: #E1E6EB; font-size: 10px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: rgba(0, 168, 255, 0.08); border-color: #00A8FF; }"
    );
    connect(m_connectHardwareBtn, &QPushButton::clicked, this, &ScreeningView::handleHardwareConnect);
    sensLayout->addWidget(m_connectHardwareBtn);

    rightLayout->addWidget(sensorCard, 1);
    mainLayout->addLayout(rightLayout, 4);
}

void ScreeningView::initChecklist() {
    m_checklistWidget->clear();
    m_checklistWidget->addItem("⬜ 1. Lock Target Organ Silhouette");
    m_checklistWidget->addItem("⬜ 2. Standardize Luminance (Lighting)");
    m_checklistWidget->addItem("⬜ 3. Calibrate Lens Focus Clarity");
    m_checklistWidget->addItem("⬜ 4. Stabilize Camera Node (Hold Steady)");
    m_checklistWidget->addItem("⬜ 5. Sweep Simulated Screening Array");
    m_checklistWidget->addItem("⬜ 6. Acquire Biomarker Samples (HR, SpO2)");
    m_checklistWidget->addItem("⬜ 7. Save Screening Visit Record");
}

void ScreeningView::updateChecklistItem(int index, bool completed) {
    if (index >= 0 && index < m_checklistWidget->count()) {
        QListWidgetItem* item = m_checklistWidget->item(index);
        QString text = item->text();
        text.replace(0, 1, completed ? "✅" : "⬜");
        item->setText(text);
        
        // Highlight active items
        if (completed) {
            item->setForeground(QBrush(QColor(46, 204, 113)));
        } else {
            item->setForeground(QBrush(QColor(225, 230, 235)));
        }
    }
}

void ScreeningView::setPatient(const QString& patientId) {
    m_patientId = patientId;
    m_scanProgressBar->setValue(0);
    m_scanCompleted = false;
    m_targetLocked = false;
    m_targetOffset = QPoint(0, 0);
    initChecklist();
}

void ScreeningView::setActive(bool active) {
    m_active = active;
    if (active && !m_currentFrame.isNull()) renderFrameWithOverlay();
}

void ScreeningView::selectOrganRegion(const QString& regionName) {
    m_targetOrgan = regionName;
    m_organLabel->setText(QString("ACTIVE PROFILE: %1 SCREENING").arg(regionName.toUpper()));
}

void ScreeningView::handleFrameReceived(const QImage& frame) {
    if (frame.isNull() || !m_viewfinder) return;
    // Keep only a bounded working frame. Fast scaling avoids a costly smooth
    // resample at camera frame rate; final display scaling remains smooth.
    m_currentFrame = frame.size().width() > 640
        ? frame.scaled(640, 480, Qt::KeepAspectRatio, Qt::FastTransformation)
        : frame;
    const auto& source = CameraSourceManager::instance();
    if (m_cameraOverlay) {
        const QString sourceName = m_emulationCheck->isChecked()
            ? QStringLiteral("Synthetic Test") : source.activeSourceName();
        m_cameraOverlay->setText(QString("SOURCE: %1  •  RESOLUTION: %2×%3  •  LIVE  •  AI RUNNING  •  REC: STANDBY")
                                     .arg(sourceName).arg(m_currentFrame.width()).arg(m_currentFrame.height()));
    }
    if (!m_active && !m_isScanningActive) return;

    // Keep video responsive while running deterministic quality analysis at
    // about 2 Hz instead of blocking the GUI on every 30 FPS frame.
    if (++m_frameCounter % 15 == 0) {
        CameraQualityEngine::instance().analyzeFrame(m_currentFrame);
    } else {
        renderFrameWithOverlay();
    }
}

void ScreeningView::handleQualityStats(const CameraQualityEngine::QualityStats& stats) {
    // 1. Update text logs
    m_lightScoreLabel->setText(QString("Luminance Level: %1%").arg(stats.lighting, 0, 'f', 1));
    m_clarityScoreLabel->setText(QString("Focus Clarity: %1%").arg(stats.clarity, 0, 'f', 1));
    m_motionScoreLabel->setText(QString("Sensor Stability: %1%").arg(stats.stability, 0, 'f', 1));
    m_overallScoreLabel->setText(QString("Overall Integrity: %1%").arg(stats.totalScore, 0, 'f', 1));

    // 2. Draw overlay tracking boxes directly on viewfinder pixmap
    QPixmap pm = QPixmap::fromImage(m_currentFrame);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);

    if (stats.faceDetected) {
        painter.setPen(QPen(QColor(46, 204, 113, 200), 2, Qt::SolidLine));
        painter.drawRect(stats.faceLocation);
        
        painter.setPen(QColor(46, 204, 113));
        painter.setFont(QFont("Monospace", 8, QFont::Bold));
        painter.drawText(stats.faceLocation.x() + 5, stats.faceLocation.y() + 15, "FACE LOCK ACQUIRED");
    }

    painter.end();
    m_currentFrame = pm.toImage();
    renderFrameWithOverlay();

    // 3. Trigger guidance alerts
    if (m_isScanningActive) return; // Ignore guide alerts during active scanning loops

    if (stats.lighting < 55.0) {
        m_guidanceBanner->setText("🚨 ADJUST LIGHTING: EXPOSURE TOO LOW. ADD LIGHT OR POSITION CAMERA TOWARD BRIGHT ANGLE.");
        m_guidanceBanner->setStyleSheet("background-color: rgba(231, 76, 60, 0.15); color: #E74C3C; border: 1px solid #E74C3C; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
        updateChecklistItem(1, false);
    } else if (stats.clarity < 50.0) {
        m_guidanceBanner->setText("⚠️ ADJUST LENS: edge blur detected. Rotate focus ring on camera lens.");
        m_guidanceBanner->setStyleSheet("background-color: rgba(241, 196, 15, 0.15); color: #F1C40F; border: 1px solid #F1C40F; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
        updateChecklistItem(2, false);
    } else if (stats.stability < 60.0) {
        m_guidanceBanner->setText("⚠️ MOTION DETECTED: hold camera steady or ask patient to sit still.");
        m_guidanceBanner->setStyleSheet("background-color: rgba(241, 196, 15, 0.15); color: #F1C40F; border: 1px solid #F1C40F; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
        updateChecklistItem(3, false);
    } else {
        m_guidanceBanner->setText("🟢 FEED STATUS: STABLE AND LOCK ON FOCUS CODES. PRESS SCAN TO START PROCEDURES.");
        m_guidanceBanner->setStyleSheet("background-color: rgba(46, 204, 113, 0.15); color: #2ECC71; border: 1px solid #2ECC71; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
        
        // Complete guidance milestones
        updateChecklistItem(0, stats.faceDetected);
        updateChecklistItem(1, true);
        updateChecklistItem(2, true);
        updateChecklistItem(3, true);
    }
}

void ScreeningView::handleSensorData(int hr, int spo2, double temp, int sys, int dia) {
    m_cachedHR = hr;
    m_cachedSPO2 = spo2;
    m_cachedTemp = temp;
    m_cachedSys = sys;
    m_cachedDia = dia;

    m_hrLabel->setText(QString("%1 bpm").arg(hr));
    m_spo2Label->setText(QString("%1%").arg(spo2));
    m_tempLabel->setText(QString("%1 °F").arg(temp, 0, 'f', 1));
    m_bpLabel->setText(QString("%1/%2 mmHg").arg(sys).arg(dia));
}

void ScreeningView::toggleEmulationMode(bool checked) {
    CommunicationManager::instance().toggleEmulation(checked);
    if (!checked) CameraSourceManager::instance().start();
}

void ScreeningView::handleHardwareConnect() {
    m_emulationCheck->setChecked(false);
    CommunicationManager::instance().toggleEmulation(false);
    CameraStreamManager::instance().reconnect();
    CameraSourceManager::instance().setMode(CameraSourceManager::Mode::Auto);
    m_guidanceBanner->setText("Retrying ESP32-CAM; laptop fallback remains available...");
}

void ScreeningView::startScreeningScan() {
    if (m_patientId.isEmpty()) {
        QMessageBox::warning(this, "Procedure Blocked", "Please select or register a patient prior to starting screening.");
        return;
    }

    if (m_isScanningActive) return;

    if (m_currentFrame.isNull() && !m_emulationCheck->isChecked()) {
        CameraSourceManager::instance().start();
        m_guidanceBanner->setText("CAMERA STARTING: ESP32-CAM is preferred; Laptop Camera fallback activates automatically.");
        m_guidanceBanner->setStyleSheet("background:#10263A; color:#8CC8FF; border:1px solid #397FB5; border-radius:8px; padding:10px; font-weight:800;");
        return;
    }

    // Check quality before launching
    CameraQualityEngine::QualityStats stats = CameraQualityEngine::instance().analyzeFrame(m_currentFrame);
    if (stats.totalScore < 60.0 && !m_emulationCheck->isChecked()) {
        QMessageBox::critical(this, "Calibration Failed", "Image score is below minimal clinical screening standards. Fix lighting or stability and try again.");
        return;
    }

    m_isScanningActive = true;
    m_scanCompleted = false;
    m_targetLocked = false;
    m_scanProgressCounter = 0;
    m_scanProgressBar->setValue(0);
    m_startScanBtn->setEnabled(false);

    initChecklist();
    updateChecklistItem(0, true);
    updateChecklistItem(1, true);
    updateChecklistItem(2, true);
    updateChecklistItem(3, true);
    
    // Advance workflow state to screening
    WorkflowEngine::instance().setStage(WorkflowEngine::StartScreening);
    WorkflowEngine::instance().startScan();
    VoiceAssistant::instance().speakTextDirectly("Demonstration scan started.");

    m_scanTimer->start(100); // 10 seconds total scan (100 steps * 100ms)
    emit scanStarted();
}

void ScreeningView::stopScreeningScan() {
    if (!m_isScanningActive) return;
    m_scanTimer->stop();
    m_isScanningActive = false;
    m_startScanBtn->setEnabled(true);
    m_scanProgressBar->setFormat("Scan stopped. Joystick navigation restored.");
    m_guidanceBanner->setText("SCAN STOPPED: Demonstration scan provider released. No diagnosis was performed.");
    m_guidanceBanner->setStyleSheet("background-color: rgba(241, 196, 15, 0.15); color: #F1C40F; border: 1px solid #F1C40F; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 11px;");
    VoiceAssistant::instance().speakTextDirectly("Demonstration scan stopped.");
    emit scanStopped();
}

void ScreeningView::moveScanTarget(const QString& action) {
    if (!m_isScanningActive) return;
    const int step = 12;
    if (action == "UP") m_targetOffset.ry() -= step;
    else if (action == "DOWN") m_targetOffset.ry() += step;
    else if (action == "LEFT") m_targetOffset.rx() -= step;
    else if (action == "RIGHT") m_targetOffset.rx() += step;
    else if (action == "LOCK") m_targetLocked = true;
    else if (action == "UNLOCK") m_targetLocked = false;
    m_targetOffset.setX(qBound(-160, m_targetOffset.x(), 160));
    m_targetOffset.setY(qBound(-110, m_targetOffset.y(), 110));
    m_guidanceBanner->setText(m_targetLocked
                                  ? "TARGET LOCKED: Press joystick again to unlock, or Button 1 to stop scan."
                                  : "SCAN TARGET MOVED: Align the demonstration crosshair using the joystick.");
    renderFrameWithOverlay();
}

void ScreeningView::updateScanProgress() {
    if (!m_isScanningActive || !m_scanTimer->isActive()) return;
    m_scanProgressCounter++;
    m_scanProgressBar->setValue(m_scanProgressCounter);

    // Speak stages progress
    if (m_scanProgressCounter == 20) {
        m_scanProgressBar->setFormat("Focusing Ultrasound Arrays... 20%");
        VoiceAssistant::instance().speakTextDirectly("Calibrating sonar array.");
    } else if (m_scanProgressCounter == 50) {
        m_scanProgressBar->setFormat("Acquiring Tissue Thickness Slice... 50%");
        VoiceAssistant::instance().speakTextDirectly("Acquiring tissue slice.");
        updateChecklistItem(4, true);
    } else if (m_scanProgressCounter == 80) {
        m_scanProgressBar->setFormat("Constructing Organ Digital Geometry... 80%");
        VoiceAssistant::instance().speakTextDirectly("Reconstructing organ volume.");
        updateChecklistItem(5, true);
    }

    if (m_scanProgressCounter >= 100) {
        m_scanTimer->stop();
        m_isScanningActive = false;
        m_scanCompleted = true;
        m_startScanBtn->setEnabled(true);
        m_scanProgressBar->setFormat("Screening Success. Commit database.");
        
        updateChecklistItem(6, true);
        
        // Save scan data
        commitVisitData();
        WorkflowEngine::instance().completeScan();
        emit scanCompleted();
    }
}

void ScreeningView::renderFrameWithOverlay() {
    if (m_currentFrame.isNull() || !m_viewfinder) return;
    QPixmap pm = QPixmap::fromImage(m_currentFrame);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);
    const QPoint center(pm.width() / 2 + m_targetOffset.x(), pm.height() / 2 + m_targetOffset.y());
    const QColor targetColor = m_targetLocked ? QColor(46, 204, 113) : QColor(0, 168, 255);
    painter.setPen(QPen(targetColor, 2));
    painter.drawLine(center.x() - 34, center.y(), center.x() + 34, center.y());
    painter.drawLine(center.x(), center.y() - 34, center.x(), center.y() + 34);
    painter.drawEllipse(center, 42, 42);
    painter.setFont(QFont("Monospace", 9, QFont::Bold));
    painter.drawText(14, pm.height() - 18,
                     m_targetLocked ? "TARGET LOCKED • EDUCATIONAL DEMO ONLY"
                                    : "JOYSTICK ALIGNMENT OVERLAY • NO DIAGNOSIS");
    painter.end();
    const QSize target = m_viewfinder->contentsRect().size();
    m_viewfinder->setPixmap(pm.scaled(target, Qt::KeepAspectRatio, Qt::FastTransformation));
}

void ScreeningView::commitVisitData() {
    DatabaseManager& db = DatabaseManager::instance();
    
    // 1. Add visit record
    Visit v;
    v.patientId = m_patientId;
    v.visitDate = QDateTime::currentDateTime().toString(Qt::ISODate);
    v.heartRate = m_cachedHR;
    v.spo2 = m_cachedSPO2;
    v.temperature = m_cachedTemp;
    v.sysBP = m_cachedSys;
    v.diaBP = m_cachedDia;
    v.screeningType = m_targetOrgan;
    v.operatorName = "Active Operator";
    
    // Record operator-facing observations only. This workflow never diagnoses.
    if (m_cachedHR > 100 || m_cachedHR < 55) {
        v.notes = QString("Simulation completed. Heart-rate telemetry (%1 bpm) was outside the configured demonstration range; clinician review is recommended. This is not a diagnosis.")
                      .arg(m_cachedHR);
    } else {
        v.notes = "Simulation completed and telemetry captured. No automated clinical interpretation was performed.";
    }

    const bool visitSaved = db.addVisit(v);

    // 2. Update digital twin organ status to Normal/Review based on HR alert
    TwinRegionState ts;
    ts.patientId = m_patientId;
    ts.regionName = m_targetOrgan;
    ts.status = (m_cachedHR > 100 || m_cachedHR < 55) ? "Review" : "Normal";
    ts.notes = v.notes;
    ts.lastUpdated = v.visitDate;
    const bool twinSaved = db.updateTwinRegion(ts);

    if (!visitSaved || !twinSaved) {
        QMessageBox::critical(this, "Demonstration Save Error",
                              "The simulated screening completed, but its local demonstration record could not be saved.");
        return;
    }

    // Advance workflow state to next (Generate report -> Save DB)
    WorkflowEngine::instance().nextStage(); // Scanning -> Generate report
    WorkflowEngine::instance().nextStage(); // Generate report -> Save Database

    QMessageBox::information(this, "Demonstration Completed",
                             QString("%1 assisted screening simulation saved for patient %2.\n\nDemonstration Mode • Educational Prototype\nThis result is not a diagnosis.")
                             .arg(m_targetOrgan).arg(m_patientId));
}
