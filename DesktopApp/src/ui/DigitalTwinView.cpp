#include "DigitalTwinView.h"
#include "../core/DatabaseManager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDateTime>
#include "components/ModernCard.h"
#include <QScopedValueRollback>

DigitalTwinView::DigitalTwinView(QWidget* parent) : QWidget(parent) {
    setupUi();
    handleRegionClicked("Heart"); // Default selection
}

DigitalTwinView::~DigitalTwinView() {}

void DigitalTwinView::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(20);

    // ==========================================
    // LEFT PANEL: Interactive Digital Twin
    // ==========================================
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(12);

    QLabel* twinTitle = new QLabel("LIVING DIGITAL HEALTH TWIN", this);
    twinTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #FFFFFF;");
    leftLayout->addWidget(twinTitle);

    // Twin Canvas Widget
    m_twinWidget = new DigitalTwinWidget(this);
    leftLayout->addWidget(m_twinWidget, 1);

    // Canvas Dials / Controls Row
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(10);

    m_rotateBtn = new QPushButton("🔄 ROTATE 3D BODY", this);
    m_rotateBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 4px; padding: 8px 12px; color: #E1E6EB; font-weight: bold; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: rgba(0, 168, 255, 0.1); border-color: #00A8FF; color: #00A8FF; }"
    );
    connect(m_rotateBtn, &QPushButton::clicked, this, &DigitalTwinView::handleRotateClick);
    controlsLayout->addWidget(m_rotateBtn);

    m_viewCombo = new QComboBox(this);
    m_viewCombo->addItems({"Front Perspective", "Side Perspective", "Back Perspective"});
    m_viewCombo->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    connect(m_viewCombo, &QComboBox::currentTextChanged, this, &DigitalTwinView::handleViewModeCombo);
    controlsLayout->addWidget(m_viewCombo);

    m_resetZoomBtn = new QPushButton("🔍 RESET ZOOM", this);
    m_resetZoomBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 4px; padding: 8px 12px; color: #E1E6EB; font-weight: bold; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: rgba(255,255,255,0.08); }"
    );
    connect(m_resetZoomBtn, &QPushButton::clicked, m_twinWidget, &DigitalTwinWidget::resetZoom);
    controlsLayout->addWidget(m_resetZoomBtn);

    leftLayout->addLayout(controlsLayout);
    mainLayout->addLayout(leftLayout, 5);

    // ==========================================
    // RIGHT PANEL: Screening History & Form
    // ==========================================
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // Timeline Summary Card
    ModernCard* summaryCard = new ModernCard(this);
    summaryCard->setHoverEffect(false);
    summaryCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(15, 15, 15, 15);
    summaryLayout->setSpacing(10);

    QHBoxLayout* headerRow = new QHBoxLayout();
    m_selectedRegionLabel = new QLabel("HEART", this);
    m_selectedRegionLabel->setStyleSheet("font-size: 18px; font-weight: 800; color: #FFFFFF;");
    headerRow->addWidget(m_selectedRegionLabel);

    m_statusBadge = new QLabel("NORMAL", this);
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setFixedSize(90, 24);
    m_statusBadge->setStyleSheet("background-color: #2ECC71; color: #FFFFFF; font-weight: bold; border-radius: 4px; font-size: 11px;");
    headerRow->addWidget(m_statusBadge, 0, Qt::AlignRight);
    summaryLayout->addLayout(headerRow);

    m_lastUpdatedLabel = new QLabel("Last Evaluation: Today", this);
    m_lastUpdatedLabel->setStyleSheet("font-size: 11px; color: #A1AAB3;");
    summaryLayout->addWidget(m_lastUpdatedLabel);

    summaryLayout->addWidget(new QLabel("Chronological Scans Timeline:", this));
    
    m_timelineList = new QListWidget(this);
    m_timelineList->setStyleSheet("background-color: rgba(0,0,0,50); border: 1px solid rgba(255,255,255,0.06); border-radius: 6px; padding: 5px; color: #E1E6EB;");
    summaryLayout->addWidget(m_timelineList, 1);
    
    rightLayout->addWidget(summaryCard, 3);

    // Update region state form card
    ModernCard* editCard = new ModernCard(this);
    editCard->setHoverEffect(false);
    editCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* editLayout = new QVBoxLayout(editCard);
    editLayout->setContentsMargins(15, 15, 15, 15);
    editLayout->setSpacing(10);

    QLabel* editHeader = new QLabel("TWIN ANATOMY RE-EVALUATION", this);
    editHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    editLayout->addWidget(editHeader);

    QFormLayout* formLayout = new QFormLayout();
    m_statusFormCombo = new QComboBox(this);
    m_statusFormCombo->addItems({"Normal", "Review", "Warning", "Critical"});
    m_statusFormCombo->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    formLayout->addRow("Severity Status:", m_statusFormCombo);

    m_notesFormInput = new QTextEdit(this);
    m_notesFormInput->setPlaceholderText("Enter operator observations, clinician notes, reports, or follow-up details. Do not enter an automated diagnosis.");
    m_notesFormInput->setMaximumHeight(70);
    formLayout->addRow("Screening Notes:", m_notesFormInput);
    editLayout->addLayout(formLayout);

    QHBoxLayout* formActions = new QHBoxLayout();
    
    m_screenBtn = new QPushButton("🩺 RUN SCAN", this);
    m_screenBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 168, 255, 0.08); border: 1px solid rgba(0, 168, 255, 0.2); border-radius: 4px; padding: 8px 12px; color: #00A8FF; font-weight: bold; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #00A8FF; color: #FFFFFF; }"
    );
    connect(m_screenBtn, &QPushButton::clicked, this, [this]() {
        emit screeningRequested(m_activeRegion);
    });
    formActions->addWidget(m_screenBtn);

    m_saveStatusBtn = new QPushButton("💾 COMMIT UPDATE", this);
    m_saveStatusBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2ECC71; border: none; border-radius: 4px; padding: 8px 12px; color: #FFFFFF; font-weight: bold; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #27AE60; }"
    );
    connect(m_saveStatusBtn, &QPushButton::clicked, this, &DigitalTwinView::handleSaveStatus);
    formActions->addWidget(m_saveStatusBtn);

    editLayout->addLayout(formActions);
    rightLayout->addWidget(editCard, 2);

    mainLayout->addLayout(rightLayout, 4);

    // Widget triggers
    connect(m_twinWidget, &DigitalTwinWidget::regionClicked, this, &DigitalTwinView::handleRegionClicked);
    connect(m_twinWidget, &DigitalTwinWidget::regionHovered, this, &DigitalTwinView::handleRegionHovered);
}

void DigitalTwinView::setPatient(const QString& patientId) {
    m_patientId = patientId;
    refreshTwinData();
}

void DigitalTwinView::refreshTwinData() {
    if (m_patientId.isEmpty() || m_refreshInProgress || !m_twinWidget) return;
    QScopedValueRollback<bool> refreshGuard(m_refreshInProgress, true);

    DatabaseManager& db = DatabaseManager::instance();
    QList<TwinRegionState> states = db.getTwinStatesForPatient(m_patientId);

    // Push states to twin canvas widget
    for (const auto& state : states) {
        m_twinWidget->setRegionStatus(state.regionName, state.status, state.notes);
    }

    // Refresh active selected region display info
    handleRegionClicked(m_activeRegion.isEmpty() ? "Heart" : m_activeRegion);
}

void DigitalTwinView::handleRegionClicked(const QString& regionName) {
    m_activeRegion = regionName;
    m_selectedRegionLabel->setText(regionName.toUpper());

    DatabaseManager& db = DatabaseManager::instance();
    
    // Read status info
    TwinRegionState state;
    if (!m_patientId.isEmpty()) {
        state = db.getTwinRegionState(m_patientId, regionName);
    } else {
        // Fallback for guest demo preloaded values
        state.regionName = regionName;
        state.status = "Normal";
        state.notes = "Workstation in guest demo mode. No patient selected.";
        state.lastUpdated = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    m_statusBadge->setText(state.status.toUpper());
    
    // Color status badge
    if (state.status == "Critical") {
        m_statusBadge->setStyleSheet("background-color: #E74C3C; color: #FFFFFF; font-weight: bold; border-radius: 4px; font-size: 11px;");
    } else if (state.status == "Warning") {
        m_statusBadge->setStyleSheet("background-color: #F1C40F; color: #FFFFFF; font-weight: bold; border-radius: 4px; font-size: 11px;");
    } else if (state.status == "Review") {
        m_statusBadge->setStyleSheet("background-color: #3498DB; color: #FFFFFF; font-weight: bold; border-radius: 4px; font-size: 11px;");
    } else {
        m_statusBadge->setStyleSheet("background-color: #2ECC71; color: #FFFFFF; font-weight: bold; border-radius: 4px; font-size: 11px;");
    }

    // Format last updated string nicely
    QString dateStr = QDateTime::fromString(state.lastUpdated, Qt::ISODate).toString("dd-MMM-yyyy hh:mm AP");
    m_lastUpdatedLabel->setText(QString("Last Evaluation: %1").arg(dateStr.isEmpty() ? "No scans recorded" : dateStr));

    // Update edit form fields
    m_statusFormCombo->setCurrentText(state.status);
    m_notesFormInput->setText(state.notes);

    loadRegionTimeline(regionName);
}

void DigitalTwinView::handleRegionHovered(const QString& regionName) {
    // Optional HUD logging or audio clicks
}

void DigitalTwinView::loadRegionTimeline(const QString& regionName) {
    m_timelineList->clear();

    if (m_patientId.isEmpty()) {
        m_timelineList->addItem("Select a registered patient to load clinical scans timeline.");
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();
    QList<Visit> visits = db.getVisitsForPatient(m_patientId);

    bool matches = false;
    for (const auto& v : visits) {
        if (v.screeningType == regionName) {
            QString date = QDateTime::fromString(v.visitDate, Qt::ISODate).toString("dd-MMM-yyyy");
            QString item = QString("[%1] HR: %2 bpm | SpO2: %3% | BP: %4/%5 mmHg\nFindings: %6")
                .arg(date)
                .arg(v.heartRate)
                .arg(v.spo2)
                .arg(v.sysBP)
                .arg(v.diaBP)
                .arg(v.notes);
            m_timelineList->addItem(item);
            matches = true;
        }
    }

    if (!matches) {
        m_timelineList->addItem("No screening sessions found on this organ region.");
    }
}

void DigitalTwinView::handleSaveStatus() {
    if (m_saveInProgress) return;
    QScopedValueRollback<bool> saveGuard(m_saveInProgress, true);
    if (m_patientId.isEmpty()) {
        QMessageBox::warning(this, "Save Fail", "Save action ignored. No patient currently loaded on the workstation.");
        return;
    }

    TwinRegionState state;
    state.patientId = m_patientId;
    state.regionName = m_activeRegion;
    state.status = m_statusFormCombo->currentText();
    state.notes = m_notesFormInput->toPlainText().trimmed();
    state.lastUpdated = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (DatabaseManager::instance().updateTwinRegion(state)) {
        refreshTwinData();
        QMessageBox::information(this, "Success", QString("Digital health twin organ state [%1] successfully committed.").arg(m_activeRegion));
    } else {
        QMessageBox::critical(this, "Database Error", "Unable to update twin region state inside SQLite.");
    }
}

void DigitalTwinView::handleRotateClick() {
    if (m_twinWidget) m_twinWidget->triggerRotation();
}

void DigitalTwinView::handleViewModeCombo(const QString& modeText) {
    if (modeText.contains("Front")) {
        m_twinWidget->setViewMode(DigitalTwinWidget::Front);
    } else if (modeText.contains("Side")) {
        m_twinWidget->setViewMode(DigitalTwinWidget::Side);
    } else if (modeText.contains("Back")) {
        m_twinWidget->setViewMode(DigitalTwinWidget::Back);
    }
}
