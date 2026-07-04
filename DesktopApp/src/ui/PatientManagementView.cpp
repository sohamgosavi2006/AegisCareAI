#include "PatientManagementView.h"
#include "../core/CommunicationManager.h"
#include "../core/DatabaseManager.h"
#include "../core/VoiceAssistant.h"
#include "../core/WorkflowEngine.h"
#include "components/ModernCard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QPainter>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopedValueRollback>
#include <QSignalBlocker>

PatientManagementView::PatientManagementView(QWidget* parent) : QWidget(parent) {
    setupUi();
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(180);
    connect(m_searchDebounce, &QTimer::timeout, this, &PatientManagementView::handleSearch);
    m_duplicateDebounce = new QTimer(this);
    m_duplicateDebounce->setSingleShot(true);
    m_duplicateDebounce->setInterval(220);
    connect(m_duplicateDebounce, &QTimer::timeout, this, &PatientManagementView::checkDuplicateRecord);
    CommunicationManager& communication = CommunicationManager::instance();
    connect(&communication, &CommunicationManager::patientChanged,
            this, &PatientManagementView::handleRemotePatientChanged);
    connect(&communication, &CommunicationManager::patientConfirmed,
            this, &PatientManagementView::handleRemotePatientConfirm);
    connect(&communication, &CommunicationManager::arduinoStatusChanged,
            this, &PatientManagementView::handleArduinoConnectionChanged);
    refreshPatientList();
}

PatientManagementView::~PatientManagementView() {}

void PatientManagementView::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(20);

    // Style adjustments
    setStyleSheet(
        "QTableWidget {"
        "  background-color: rgba(255, 255, 255, 0.02);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 8px;"
        "  color: #E1E6EB;"
        "  gridline-color: rgba(255, 255, 255, 0.04);"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: rgba(0, 168, 255, 0.2);"
        "  color: #FFFFFF;"
        "}"
        "QHeaderView::section {"
        "  background-color: #122030;"
        "  color: #A1AAB3;"
        "  padding: 6px;"
        "  border: none;"
        "  font-weight: bold;"
        "}"
    );

    // ==========================================
    // LEFT SIDE: Patient Search & List
    // ==========================================
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(12);

    QLabel* listTitle = new QLabel("PATIENT REGISTRY & CARE MANAGEMENT", this);
    listTitle->setStyleSheet("font-size: 18px; font-weight: 900; color: #FFFFFF; letter-spacing: 0.8px;");
    leftLayout->addWidget(listTitle);
    QLabel* demoNotice = new QLabel("Educational Demonstration Prototype • Not Intended For Clinical Diagnosis", this);
    demoNotice->setStyleSheet("font-size: 10px; font-weight: 800; color: #F5B041;");
    leftLayout->addWidget(demoNotice);

    // Search Controls
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(10);
    
    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("🔍 Search by Name, Patient ID, Address...");
    m_searchBar->setStyleSheet("background-color: rgba(0,0,0,80); padding: 8px; color: #FFFFFF; border-radius: 6px;");
    connect(m_searchBar, &QLineEdit::textChanged, this, [this]() { m_searchDebounce->start(); });
    searchLayout->addWidget(m_searchBar, 2);

    m_filterVillage = new QComboBox(this);
    m_filterVillage->addItem("All Addresses");
    m_filterVillage->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    connect(m_filterVillage, &QComboBox::currentTextChanged, this, &PatientManagementView::handleSearch);
    searchLayout->addWidget(m_filterVillage, 1);
    
    leftLayout->addLayout(searchLayout);

    // Patient Table
    m_patientTable = new QTableWidget(this);
    m_patientTable->setColumnCount(5);
    m_patientTable->setHorizontalHeaderLabels({"ID", "Full Name", "Age", "Gender", "Address"});
    m_patientTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_patientTable->verticalHeader()->setVisible(false);
    m_patientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patientTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_patientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_patientTable, &QTableWidget::cellClicked, this, &PatientManagementView::handleRowSelected);
    leftLayout->addWidget(m_patientTable);

    mainLayout->addLayout(leftLayout, 5);

    // ==========================================
    // RIGHT SIDE: Details Card & Registration Form
    // ==========================================
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // 1. Patient Details Summary Card
    ModernCard* detailCard = new ModernCard(this);
    detailCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    detailCard->setMinimumHeight(220);
    
    QVBoxLayout* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(15, 15, 15, 15);
    detailLayout->setSpacing(8);

    QLabel* cardHeader = new QLabel("DETAILED PATIENT PROFILE", this);
    cardHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF; letter-spacing: 0.5px;");
    detailLayout->addWidget(cardHeader);

    QHBoxLayout* detailBody = new QHBoxLayout();
    detailBody->setSpacing(15);

    m_photoLabel = new QLabel("👤", this);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setFixedSize(92, 92);
    m_photoLabel->setStyleSheet("font-size: 42px; background: rgba(0,168,255,0.10); border: 1px solid rgba(0,168,255,0.35); border-radius: 46px;");
    detailBody->addWidget(m_photoLabel, 0, Qt::AlignTop);

    QVBoxLayout* labelFields = new QVBoxLayout();
    m_detailIdLabel = new QLabel("ID: AEGIS-YYYY-NNNN", this);
    m_detailIdLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #E1E6EB;");
    labelFields->addWidget(m_detailIdLabel);

    m_detailNameLabel = new QLabel("Name: No Patient Selected", this);
    m_detailNameLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #FFFFFF;");
    labelFields->addWidget(m_detailNameLabel);

    m_detailAgeGenderLabel = new QLabel("Profile: Age / Gender / Blood", this);
    m_detailAgeGenderLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailAgeGenderLabel);

    m_detailPhoneLabel = new QLabel("Phone: --", this);
    m_detailPhoneLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailPhoneLabel);

    m_detailAddressLabel = new QLabel("Address: --", this);
    m_detailAddressLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailAddressLabel);

    m_detailEmergencyLabel = new QLabel("Emergency Contact: --", this);
    m_detailEmergencyLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailEmergencyLabel);

    m_detailDoctorLabel = new QLabel("Assigned Doctor: Dr. Soham", this);
    m_detailDoctorLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailDoctorLabel);

    m_detailStatusLabel = new QLabel("Current Status: Ready for demonstration", this);
    m_detailStatusLabel->setStyleSheet("font-size: 12px; color: #2ECC71; font-weight: 800;");
    labelFields->addWidget(m_detailStatusLabel);

    m_detailLastVisitLabel = new QLabel("Last Visit: --", this);
    m_detailLastVisitLabel->setStyleSheet("font-size: 12px; color: #A1AAB3;");
    labelFields->addWidget(m_detailLastVisitLabel);

    m_detailRiskLabel = new QLabel("Risk Level: Demonstration baseline", this);
    m_detailRiskLabel->setStyleSheet("font-size: 12px; color: #F5B041; font-weight: 800;");
    labelFields->addWidget(m_detailRiskLabel);

    m_detailTwinLabel = new QLabel("Digital Twin Status: Synced", this);
    m_detailTwinLabel->setStyleSheet("font-size: 12px; color: #00A8FF; font-weight: 800;");
    labelFields->addWidget(m_detailTwinLabel);

    detailBody->addLayout(labelFields, 3);

    // Vector QR Holder
    m_qrCodeDisplay = new QLabel(this);
    m_qrCodeDisplay->setFixedSize(110, 110);
    m_qrCodeDisplay->setAlignment(Qt::AlignCenter);
    m_qrCodeDisplay->setStyleSheet("border: 1px dashed rgba(0, 168, 255, 0.3); border-radius: 6px; background-color: rgba(0,0,0,50);");
    drawPatientQR("AEGIS_MOCK_QR");
    detailBody->addWidget(m_qrCodeDisplay, 2, Qt::AlignRight);

    detailLayout->addLayout(detailBody);
    rightLayout->addWidget(detailCard);

    ModernCard* careCard = new ModernCard(this);
    careCard->setHoverEffect(false);
    careCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    QVBoxLayout* careLayout = new QVBoxLayout(careCard);
    careLayout->setContentsMargins(15, 15, 15, 15);
    QLabel* careHeader = new QLabel("CARE MANAGEMENT TIMELINE", this);
    careHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    careLayout->addWidget(careHeader);
    QTabWidget* careTabs = new QTabWidget(this);
    careTabs->setStyleSheet("QTabWidget::pane { border: 1px solid rgba(255,255,255,0.08); border-radius: 8px; } QTabBar::tab { background: rgba(255,255,255,0.04); color: #A1AAB3; padding: 8px 10px; } QTabBar::tab:selected { color: #FFFFFF; background: rgba(0,168,255,0.18); }");
    m_previousVisitsList = new QListWidget(this);
    m_voiceNotesList = new QListWidget(this);
    m_reportsList = new QListWidget(this);
    m_followUpsList = new QListWidget(this);
    careTabs->addTab(m_previousVisitsList, "Previous Visits");
    careTabs->addTab(m_voiceNotesList, "Clinical Notes");
    careTabs->addTab(m_reportsList, "Reports");
    careTabs->addTab(m_followUpsList, "Follow-up");
    careLayout->addWidget(careTabs);
    rightLayout->addWidget(careCard, 1);

    // 2. Registration Panel Form
    ModernCard* formCard = new ModernCard(this);
    formCard->setHoverEffect(false);
    formCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* formWrapper = new QVBoxLayout(formCard);
    formWrapper->setContentsMargins(15, 15, 15, 15);
    
    QLabel* formHeader = new QLabel("NEW PATIENT REGISTRATION", this);
    formHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    formWrapper->addWidget(formHeader);
    formWrapper->addSpacing(5);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignLeft);
    formLayout->setSpacing(8);

    // Fields Setup
    m_nameInput = new QLineEdit(this);
    m_nameInput->setPlaceholderText("Enter full name");
    connect(m_nameInput, &QLineEdit::textChanged, this, [this]() { m_duplicateDebounce->start(); });

    m_ageInput = new QSpinBox(this);
    m_ageInput->setRange(0, 120);
    m_ageInput->setValue(30);

    m_genderCombo = new QComboBox(this);
    m_genderCombo->addItems({"Male", "Female", "Other"});

    m_bloodCombo = new QComboBox(this);
    m_bloodCombo->addItems({"A+", "A-", "B+", "B-", "O+", "O-", "AB+", "AB-"});

    m_phoneInput = new QLineEdit(this);
    m_phoneInput->setPlaceholderText("+91 XXXXX XXXXX");
    connect(m_phoneInput, &QLineEdit::textChanged, this, [this]() { m_duplicateDebounce->start(); });

    m_villageInput = new QLineEdit(this);
    m_villageInput->setPlaceholderText("Address / locality");

    m_addressInput = new QLineEdit(this);
    m_addressInput->setPlaceholderText("House / Street No.");

    m_emergencyInput = new QLineEdit(this);
    m_emergencyInput->setPlaceholderText("Emergency contact phone");

    m_doctorCombo = new QComboBox(this);
    m_doctorCombo->addItems({"Dr. Soham"});

    m_notesInput = new QTextEdit(this);
    m_notesInput->setPlaceholderText("Relevant symptoms, medications, or surgical history notes...");
    m_notesInput->setMaximumHeight(70);

    // Add elements to layout
    formLayout->addRow("Full Name:", m_nameInput);
    formLayout->addRow("Age:", m_ageInput);
    formLayout->addRow("Gender:", m_genderCombo);
    formLayout->addRow("Blood Group:", m_bloodCombo);
    formLayout->addRow("Phone Number:", m_phoneInput);
    formLayout->addRow("Address:", m_villageInput);
    formLayout->addRow("Street Address:", m_addressInput);
    formLayout->addRow("Emergency Contact:", m_emergencyInput);
    formLayout->addRow("Assigned MD:", m_doctorCombo);
    formLayout->addRow("Clinical Notes:", m_notesInput);

    formWrapper->addLayout(formLayout);
    formWrapper->addSpacing(10);

    // Register Actions Row
    QHBoxLayout* actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(10);
    
    m_clearBtn = new QPushButton("CLEAR FORM", this);
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 4px; padding: 8px; color: #A1AAB3; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: rgba(255,255,255,0.08); color: #FFFFFF; }"
    );
    connect(m_clearBtn, &QPushButton::clicked, this, &PatientManagementView::clearForm);
    actionsLayout->addWidget(m_clearBtn);

    m_registerBtn = new QPushButton("ADD TO REGISTRY", this);
    m_registerBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #00A8FF; border: none; border-radius: 4px; padding: 8px; color: #FFFFFF; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0088E6; }"
    );
    connect(m_registerBtn, &QPushButton::clicked, this, &PatientManagementView::handleRegister);
    actionsLayout->addWidget(m_registerBtn);

    formWrapper->addLayout(actionsLayout);
    rightLayout->addWidget(formCard);

    mainLayout->addLayout(rightLayout, 4);
}

void PatientManagementView::refreshPatientList() {
    if (m_refreshInProgress || !m_patientTable) return;
    QScopedValueRollback<bool> refreshGuard(m_refreshInProgress, true);
    const QSignalBlocker tableBlocker(m_patientTable);
    const QString selectedId = m_selectedPatientId;
    DatabaseManager& db = DatabaseManager::instance();
    m_currentPatients = db.getPatients();

    m_patientTable->setRowCount(0);
    m_visiblePatients.clear();
    
    // Clear & reload village filters
    m_filterVillage->blockSignals(true);
    m_filterVillage->clear();
    m_filterVillage->addItem("All Addresses");
    
    QStringList villages;
    
    for (int i = 0; i < m_currentPatients.size(); ++i) {
        const Patient& p = m_currentPatients[i];
        m_visiblePatients.append(p);
        m_patientTable->insertRow(i);
        m_patientTable->setItem(i, 0, new QTableWidgetItem(p.patientId));
        m_patientTable->setItem(i, 1, new QTableWidgetItem(p.fullName));
        m_patientTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.age)));
        m_patientTable->setItem(i, 3, new QTableWidgetItem(p.gender));
        m_patientTable->setItem(i, 4, new QTableWidgetItem(p.village));

        if (!villages.contains(p.village) && !p.village.isEmpty()) {
            villages.append(p.village);
        }
    }
    
    m_filterVillage->addItems(villages);
    m_filterVillage->blockSignals(false);
    int selectedRow = 0;
    for (int row = 0; row < m_visiblePatients.size(); ++row) {
        if (m_visiblePatients[row].patientId == selectedId) {
            selectedRow = row;
            m_patientTable->selectRow(row);
            break;
        }
    }
    synchronizePatientWindow(selectedRow);
}

void PatientManagementView::selectPatient(const QString& patientId) {
    for (const Patient& p : m_currentPatients) {
        if (p.patientId == patientId) {
            updateDetailPanel(p);
            
            // Find and highlight in table
            for (int r = 0; r < m_patientTable->rowCount(); ++r) {
                if (m_patientTable->item(r, 0) && m_patientTable->item(r, 0)->text() == patientId) {
                    m_patientTable->selectRow(r);
                    m_patientTable->scrollToItem(m_patientTable->item(r, 0), QAbstractItemView::PositionAtCenter);
                    m_selectedPatientId = patientId;
                    synchronizePatientWindow(r);
                    sendSelectedPatient(r);
                    break;
                }
            }
            break;
        }
    }
}

void PatientManagementView::handleSearch() {
    if (m_refreshInProgress || !m_patientTable) return;
    QScopedValueRollback<bool> refreshGuard(m_refreshInProgress, true);
    const QSignalBlocker tableBlocker(m_patientTable);
    DatabaseManager& db = DatabaseManager::instance();
    QString text = m_searchBar->text().trimmed();
    QString village = m_filterVillage->currentText();
    if (village == "All Addresses") village = "";

    // Re-query database
    m_currentPatients = db.getPatients(text);
    
    m_patientTable->setRowCount(0);
    m_visiblePatients.clear();
    int row = 0;
    for (const Patient& p : m_currentPatients) {
        if (!village.isEmpty() && p.village != village) continue;

        m_visiblePatients.append(p);

        m_patientTable->insertRow(row);
        m_patientTable->setItem(row, 0, new QTableWidgetItem(p.patientId));
        m_patientTable->setItem(row, 1, new QTableWidgetItem(p.fullName));
        m_patientTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.age)));
        m_patientTable->setItem(row, 3, new QTableWidgetItem(p.gender));
        m_patientTable->setItem(row, 4, new QTableWidgetItem(p.village));
        row++;
    }
    synchronizePatientWindow(0);
}

void PatientManagementView::handleRowSelected(int row, int column) {
    Q_UNUSED(column);
    if (row < 0 || row >= m_patientTable->rowCount() || !m_patientTable->item(row, 0)) return;
    QString patientId = m_patientTable->item(row, 0)->text();
    m_selectedPatientId = patientId;

    for (const Patient& p : m_currentPatients) {
        if (p.patientId == patientId) {
            updateDetailPanel(p);
            
            // Advance workflow to queue assignment/load twin stages
            WorkflowEngine::instance().setStage(WorkflowEngine::LoadDigitalTwin);

            emit patientSelected(patientId);
            synchronizePatientWindow(row);
            sendSelectedPatient(row);
            break;
        }
    }
}

void PatientManagementView::handleRemotePatientChanged(int index) {
    if (index < 0 || index >= m_visiblePatients.size() || index >= m_patientTable->rowCount()) {
        return;
    }
    m_applyingRemoteSelection = true;
    m_patientTable->setCurrentCell(index, 0);
    m_patientTable->selectRow(index);
    m_patientTable->scrollToItem(m_patientTable->item(index, 0), QAbstractItemView::PositionAtCenter);
    handleRowSelected(index, 0);
    m_applyingRemoteSelection = false;
}

void PatientManagementView::handleRemotePatientConfirm() {
    const int row = m_patientTable->currentRow();
    if (row < 0 || row >= m_visiblePatients.size()) {
        return;
    }
    const Patient& patient = m_visiblePatients[row];
    updateDetailPanel(patient);
    m_selectedPatientId = patient.patientId;
    emit patientSelected(patient.patientId);
}

void PatientManagementView::handleArduinoConnectionChanged(bool connected) {
    if (connected) {
        int row = m_patientTable->currentRow();
        if (row < 0 && !m_visiblePatients.isEmpty()) {
            row = 0;
            m_patientTable->setCurrentCell(row, 0);
            m_patientTable->selectRow(row);
            updateDetailPanel(m_visiblePatients[row]);
            m_selectedPatientId = m_visiblePatients[row].patientId;
        }
        row = qMax(0, row);
        synchronizePatientWindow(row);
        if (!m_visiblePatients.isEmpty()) sendSelectedPatient(row);
    }
}

void PatientManagementView::synchronizePatientWindow(int selectedIndex) {
    CommunicationManager& communication = CommunicationManager::instance();
    if (!communication.isArduinoConnected() || m_visiblePatients.isEmpty()) {
        return;
    }

    constexpr int windowSize = 4;
    selectedIndex = qBound(0, selectedIndex, m_visiblePatients.size() - 1);
    const int maximumStart = qMax(0, m_visiblePatients.size() - windowSize);
    const int start = qBound(0, selectedIndex - 1, maximumStart);
    const int end = qMin(start + windowSize, m_visiblePatients.size());

    QJsonArray patients;
    for (int index = start; index < end; ++index) {
        const Patient& patient = m_visiblePatients[index];
        patients.append(QJsonObject{{QStringLiteral("index"), index},
                                    {QStringLiteral("id"), patient.patientId},
                                    {QStringLiteral("name"), patient.fullName.left(14)}});
    }
    communication.sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("patient_list")},
                                       {QStringLiteral("total"), m_visiblePatients.size()},
                                       {QStringLiteral("patients"), patients}});
}

void PatientManagementView::sendSelectedPatient(int index) {
    if (index < 0 || index >= m_visiblePatients.size()) {
        return;
    }
    const Patient& patient = m_visiblePatients[index];
    CommunicationManager::instance().sendJSON(
        QJsonObject{{QStringLiteral("type"), QStringLiteral("patient_selected")},
                    {QStringLiteral("index"), index},
                    {QStringLiteral("id"), patient.patientId},
                    {QStringLiteral("name"), patient.fullName}});
}

void PatientManagementView::updateDetailPanel(const Patient& p) {
    m_detailIdLabel->setText("ID: " + p.patientId);
    m_detailNameLabel->setText(p.fullName);
    m_detailAgeGenderLabel->setText(QString("%1 YRS / %2 / BLOOD: %3").arg(p.age).arg(p.gender.toUpper()).arg(p.bloodGroup));
    m_detailPhoneLabel->setText("Phone: " + p.phone);
    m_detailAddressLabel->setText(QString("Address: %1").arg(p.village));
    m_detailEmergencyLabel->setText("Emergency Contact: " + (p.emergencyContact.isEmpty() ? "--" : p.emergencyContact));
    m_detailDoctorLabel->setText("Assigned Doctor: Dr. Soham");

    const QList<Visit> visits = DatabaseManager::instance().getVisitsForPatient(p.patientId);
    const QList<TwinRegionState> twinStates = DatabaseManager::instance().getTwinStatesForPatient(p.patientId);
    const QList<VoiceNote> voiceNotes = DatabaseManager::instance().getVoiceNotesForPatient(p.patientId);
    const QList<PatientReport> reports = DatabaseManager::instance().getReportsForPatient(p.patientId);
    const QList<FollowUp> followUps = DatabaseManager::instance().getAllFollowUps();
    const bool hasEmergency = !DatabaseManager::instance().getEmergencyEventsForPatient(p.patientId).isEmpty();

    m_detailLastVisitLabel->setText(visits.isEmpty() ? "Last Visit: Demonstration visit pending"
                                                     : "Last Visit: " + visits.first().visitDate.left(10));
    m_detailStatusLabel->setText(hasEmergency ? "Current Status: HIGH PRIORITY" : "Current Status: Ready for demonstration");
    m_detailStatusLabel->setStyleSheet(hasEmergency
                                           ? "font-size: 12px; color: #E74C3C; font-weight: 900;"
                                           : "font-size: 12px; color: #2ECC71; font-weight: 800;");
    m_detailRiskLabel->setText(hasEmergency ? "Risk Level: High priority demo flag" : "Risk Level: Demonstration baseline");
    m_detailTwinLabel->setText(QString("Digital Twin Status: %1 regions synced").arg(twinStates.size()));

    if (m_previousVisitsList) {
        m_previousVisitsList->clear();
        if (visits.isEmpty()) {
            m_previousVisitsList->addItem("Routine Health Screening • Report Generated • Voice Notes Available");
            m_previousVisitsList->addItem("Follow-up Visit • Demo Skin Screening • QR Generated • Voice Notes Available");
            m_previousVisitsList->addItem("Annual Check-up • Healthy Demonstration");
        } else {
            for (const Visit& visit : visits.mid(0, 5)) {
                m_previousVisitsList->addItem(QString("%1 • %2 • Report/QR workflow available • Demo only")
                                                  .arg(visit.visitDate.left(10), visit.screeningType));
            }
        }
    }
    if (m_voiceNotesList) {
        m_voiceNotesList->clear();
        if (voiceNotes.isEmpty()) {
            m_voiceNotesList->addItem("Visit 2 • ▶ Play Recording • Transcript: Patient reports mild shoulder pain while lifting arm. No severe symptoms observed.");
        } else {
            for (const VoiceNote& note : voiceNotes.mid(0, 6)) {
                m_voiceNotesList->addItem(QString("%1 • Operator %2 • ▶ Play Audio • Transcript: %3")
                                              .arg(note.timestamp.left(16), note.operatorName, note.transcript.left(110)));
            }
        }
    }
    if (m_reportsList) {
        m_reportsList->clear();
        if (reports.isEmpty()) {
            m_reportsList->addItem("No generated report yet • Complete scan and press Button 3.");
        } else {
            for (const PatientReport& report : reports.mid(0, 6)) {
                m_reportsList->addItem(QString("%1 • QR Generated • GitHub Link: %2 • Open Report • Download HTML")
                                           .arg(report.generatedAt.left(10), report.githubUrl));
            }
        }
    }
    if (m_followUpsList) {
        m_followUpsList->clear();
        int count = 0;
        for (const FollowUp& follow : followUps) {
            if (follow.patientId != p.patientId) continue;
            m_followUpsList->addItem(QString("Next Visit: %1 • %2 • %3 • Voice note reference available")
                                         .arg(follow.dueDate, follow.status, follow.notes));
            ++count;
        }
        if (count == 0) {
            m_followUpsList->addItem("Follow-up Task: Lifestyle advice review");
            m_followUpsList->addItem("Medication Reminder: Demonstration reminder only");
            m_followUpsList->addItem("Doctor Notes: Dr. Soham to review observations after report generation");
            m_followUpsList->addItem("Observation Summary: No clinical diagnosis generated");
        }
    }
    
    drawPatientQR(p.qrId);
}

void PatientManagementView::checkDuplicateRecord() {
    QString name = m_nameInput->text().trimmed();
    QString phone = m_phoneInput->text().trimmed();
    int age = m_ageInput->value();

    if (name.isEmpty() || phone.isEmpty()) {
        m_nameInput->setStyleSheet("");
        m_phoneInput->setStyleSheet("");
        return;
    }

    if (DatabaseManager::instance().checkDuplicate(name, age, phone)) {
        m_nameInput->setStyleSheet("border: 1px solid #E74C3C; background-color: rgba(231, 76, 60, 0.1);");
        m_phoneInput->setStyleSheet("border: 1px solid #E74C3C; background-color: rgba(231, 76, 60, 0.1);");
    } else {
        m_nameInput->setStyleSheet("");
        m_phoneInput->setStyleSheet("");
    }
}

void PatientManagementView::clearForm() {
    m_nameInput->clear();
    m_ageInput->setValue(30);
    m_genderCombo->setCurrentIndex(0);
    m_bloodCombo->setCurrentIndex(0);
    m_phoneInput->clear();
    m_villageInput->clear();
    m_addressInput->clear();
    m_emergencyInput->clear();
    m_notesInput->clear();
    m_nameInput->setStyleSheet("");
    m_phoneInput->setStyleSheet("");
}

void PatientManagementView::handleRegister() {
    if (m_actionInProgress) return;
    QScopedValueRollback<bool> actionGuard(m_actionInProgress, true);
    QString name = m_nameInput->text().trimmed();
    QString phone = m_phoneInput->text().trimmed();
    QString village = m_villageInput->text().trimmed();
    int age = m_ageInput->value();

    if (name.isEmpty() || phone.isEmpty() || village.isEmpty()) {
        QMessageBox::warning(this, "Registry Error", "Full Name, Phone Number, and Address are required fields.");
        return;
    }

    if (DatabaseManager::instance().checkDuplicate(name, age, phone)) {
        QMessageBox::warning(this, "Duplicate Record Prevented",
                             "A patient with the same name, age, and phone number already exists. Search the registry and open that record instead.");
        return;
    }

    Patient p;
    p.fullName = name;
    p.age = age;
    p.gender = m_genderCombo->currentText();
    p.bloodGroup = m_bloodCombo->currentText();
    p.phone = phone;
    p.village = village;
    p.address = m_addressInput->text().trimmed();
    p.emergencyContact = m_emergencyInput->text().trimmed();
    p.doctorAssigned = m_doctorCombo->currentText();
    p.medicalNotes = m_notesInput->toPlainText().trimmed();
    p.operatorName = "Active Operator"; // Dynamically binds

    if (DatabaseManager::instance().addPatient(p)) {
        refreshPatientList();
        selectPatient(p.patientId);
        clearForm();
        
        // Advance workflow from registration step
        WorkflowEngine::instance().nextStage(); // Patient registration -> Queue assignment

        emit patientSelected(p.patientId);
        QMessageBox::information(this, "Registry Success", "Patient registered successfully! Digital Health Twin created.");
    } else {
        QMessageBox::critical(this, "Registry Database Error", "Could not insert patient record into local SQLite database.");
    }
}

void PatientManagementView::drawPatientQR(const QString& qrText) {
    // Generate high-tech simulated QR code
    QPixmap pm(110, 110);
    pm.fill(Qt::transparent);

    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, false); // Keep pixels crisp

    // Draw background
    painter.fillRect(0, 0, 110, 110, QColor(10, 17, 24));

    // Seed visual blocks based on QR text hash
    QByteArray hash = QCryptographicHash::hash(qrText.toUtf8(), QCryptographicHash::Md5);
    
    // Draw pseudorandom QR matrix grid (14x14 blocks)
    int size = 14;
    int blockSize = 6;
    int margin = 13;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(0, 168, 255))); // Neon cyan blocks

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            // Check corner regions for alignment markers
            bool isTopLeftMarker = (x < 4 && y < 4);
            bool isTopRightMarker = (x >= size - 4 && y < 4);
            bool isBottomLeftMarker = (x < 4 && y >= size - 4);
            
            if (isTopLeftMarker || isTopRightMarker || isBottomLeftMarker) {
                continue; // Draw alignment targets separately
            }

            int hashIdx = (y * size + x) % hash.size();
            bool fill = (static_cast<unsigned char>(hash[hashIdx]) & (1 << (x % 8))) != 0;
            if (fill) {
                painter.drawRect(margin + x * blockSize, margin + y * blockSize, blockSize, blockSize);
            }
        }
    }

    // Draw high-tech targeting squares in corners
    auto drawCornerMarker = [&](int px, int py) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 168, 255), 2));
        painter.drawRect(px, py, 20, 20); // Outer ring
        painter.setBrush(QBrush(QColor(0, 168, 255)));
        painter.setPen(Qt::NoPen);
        painter.drawRect(px + 5, py + 5, 10, 10); // Inner center solid
    };

    drawCornerMarker(margin, margin); // Top Left
    drawCornerMarker(margin + (size - 4) * blockSize + 4, margin); // Top Right
    drawCornerMarker(margin, margin + (size - 4) * blockSize + 4); // Bottom Left

    painter.end();
    m_qrCodeDisplay->setPixmap(pm);
}
