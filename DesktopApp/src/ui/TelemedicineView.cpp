#include "TelemedicineView.h"

#include "../core/DatabaseManager.h"
#include "components/ModernCard.h"

#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSaveFile>
#include <QVBoxLayout>

TelemedicineView::TelemedicineView(QWidget* parent) : QWidget(parent) {
    populateDoctors();
    setupUi();
    m_typingTimer = new QTimer(this);
    m_typingTimer->setInterval(28);
    connect(m_typingTimer, &QTimer::timeout, this, &TelemedicineView::typeNextCharacter);
    filterDoctors();
}

TelemedicineView::~TelemedicineView() = default;

void TelemedicineView::populateDoctors() {
    m_doctors = {
        {"Dr. Neha Kapoor", "Cardiologist", "Lotus Heart Institute", "Mumbai", "Online", "MD Cardiology", 12, 4.9},
        {"Dr. Rohan Iyer", "Dermatologist", "Pune Skin Centre", "Pune", "Online", "MD Dermatology", 9, 4.8},
        {"Dr. Vivek Shah", "Neurologist", "Ahmedabad Neuro Hospital", "Ahmedabad", "Busy", "DM Neurology", 15, 4.7},
        {"Dr. Aditi Mehta", "Radiologist", "National Imaging Centre", "Delhi", "Online", "MD Radiodiagnosis", 11, 4.9},
        {"Dr. Kavya Nair", "General Medicine", "Coastal Medical Centre", "Kochi", "Online", "MD Medicine", 8, 4.6},
        {"Dr. Arjun Rao", "Orthopaedic", "Mobility Care Hospital", "Bengaluru", "Offline", "MS Orthopaedics", 14, 4.8},
        {"Dr. Simran Kaur", "Pulmonologist", "BreathWell Institute", "Chandigarh", "Online", "DM Pulmonary Medicine", 10, 4.7},
        {"Dr. Mihir Joshi", "Endocrinologist", "Metabolic Care Clinic", "Nashik", "Busy", "DM Endocrinology", 13, 4.8},
        {"Dr. Farah Khan", "Paediatrician", "Children's Health Centre", "Hyderabad", "Online", "MD Paediatrics", 7, 4.7}
    };
}

void TelemedicineView::setupUi() {
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(22, 22, 22, 22);
    main->setSpacing(14);

    auto* title = new QLabel("REMOTE SPECIALIST CONSULTATION", this);
    title->setStyleSheet("font-size:24px; font-weight:950; color:#FFFFFF;");
    auto* notice = new QLabel("Educational Demonstration Prototype • No real telemedicine or medical advice", this);
    notice->setStyleSheet("font-size:10px; color:#F5B041; font-weight:850;");
    main->addWidget(title);
    main->addWidget(notice);

    auto* panels = new QHBoxLayout();
    panels->setSpacing(16);

    QVBoxLayout* directoryLayout = nullptr;
    auto* directoryCard = new ModernCard(this);
    directoryCard->setHoverEffect(false);
    directoryCard->setStyleSheet("QFrame { background:rgba(255,255,255,.035); border:1px solid rgba(255,255,255,.08); border-radius:14px; }");
    directoryLayout = new QVBoxLayout(directoryCard);
    directoryLayout->setContentsMargins(16, 16, 16, 16);
    auto* directoryTitle = new QLabel("ONLINE DOCTORS", directoryCard);
    directoryTitle->setStyleSheet("font-size:12px; color:#00A8FF; font-weight:900;");
    directoryLayout->addWidget(directoryTitle);
    m_cityFilter = new QComboBox(directoryCard);
    m_specializationFilter = new QComboBox(directoryCard);
    m_availabilityFilter = new QComboBox(directoryCard);
    m_cityFilter->addItem("All Cities");
    m_specializationFilter->addItem("All Specializations");
    m_availabilityFilter->addItems({"All Availability", "Online", "Busy", "Offline"});
    QStringList cities;
    QStringList specialties;
    for (const Doctor& doctor : m_doctors) {
        if (!cities.contains(doctor.city)) cities << doctor.city;
        if (!specialties.contains(doctor.specialization)) specialties << doctor.specialization;
    }
    m_cityFilter->addItems(cities);
    m_specializationFilter->addItems(specialties);
    directoryLayout->addWidget(m_cityFilter);
    directoryLayout->addWidget(m_specializationFilter);
    directoryLayout->addWidget(m_availabilityFilter);
    m_doctorList = new QListWidget(directoryCard);
    directoryLayout->addWidget(m_doctorList, 1);
    panels->addWidget(directoryCard, 3);

    auto* detailCard = new ModernCard(this);
    detailCard->setHoverEffect(false);
    detailCard->setStyleSheet("QFrame { background:rgba(255,255,255,.035); border:1px solid rgba(255,255,255,.08); border-radius:14px; }");
    auto* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(20, 18, 20, 18);
    auto* detailTitle = new QLabel("SPECIALIST PROFILE", detailCard);
    detailTitle->setStyleSheet("font-size:12px; color:#00A8FF; font-weight:900;");
    m_doctorPhoto = new QLabel("👩‍⚕️", detailCard);
    m_doctorPhoto->setAlignment(Qt::AlignCenter);
    m_doctorPhoto->setMinimumHeight(120);
    m_doctorPhoto->setStyleSheet("font-size:64px; background:rgba(0,168,255,.08); border-radius:12px;");
    m_doctorName = new QLabel("Select a doctor", detailCard);
    m_doctorName->setAlignment(Qt::AlignCenter);
    m_doctorName->setStyleSheet("font-size:19px; font-weight:900; color:#FFFFFF;");
    m_onlineIndicator = new QLabel("● Availability", detailCard);
    m_onlineIndicator->setAlignment(Qt::AlignCenter);
    m_doctorDetails = new QLabel(detailCard);
    m_doctorDetails->setWordWrap(true);
    m_doctorDetails->setStyleSheet("color:#B7C4CF; line-height:150%;");
    m_connectionStatus = new QLabel("CONSULTATION CHANNEL IDLE", detailCard);
    m_connectionStatus->setAlignment(Qt::AlignCenter);
    m_connectionStatus->setWordWrap(true);
    m_connectionStatus->setStyleSheet("background:rgba(255,255,255,.04); border-radius:8px; padding:10px; color:#A1AAB3; font-weight:800;");
    m_connectButton = new QPushButton("CONNECT TO SPECIALIST", detailCard);
    detailLayout->addWidget(detailTitle);
    detailLayout->addWidget(m_doctorPhoto);
    detailLayout->addWidget(m_doctorName);
    detailLayout->addWidget(m_onlineIndicator);
    detailLayout->addWidget(m_doctorDetails);
    detailLayout->addStretch();
    detailLayout->addWidget(m_connectionStatus);
    detailLayout->addWidget(m_connectButton);
    panels->addWidget(detailCard, 3);

    auto* notesCard = new ModernCard(this);
    notesCard->setHoverEffect(false);
    notesCard->setStyleSheet("QFrame { background:rgba(255,255,255,.035); border:1px solid rgba(255,255,255,.08); border-radius:14px; }");
    auto* notesLayout = new QVBoxLayout(notesCard);
    notesLayout->setContentsMargins(18, 16, 18, 16);
    auto* notesTitle = new QLabel("CONSULTATION NOTES", notesCard);
    notesTitle->setStyleSheet("font-size:12px; color:#00A8FF; font-weight:900;");
    m_consultNotes = new QTextEdit(notesCard);
    m_consultNotes->setPlaceholderText("After connecting, the demonstration specialist response will appear with a typing animation...");
    m_saveButton = new QPushButton("SAVE NOTES", notesCard);
    m_attachButton = new QPushButton("ATTACH TO PATIENT", notesCard);
    m_exportButton = new QPushButton("EXPORT CONSULTATION", notesCard);
    m_saveButton->setEnabled(false);
    m_attachButton->setEnabled(false);
    m_exportButton->setEnabled(false);
    notesLayout->addWidget(notesTitle);
    notesLayout->addWidget(m_consultNotes, 1);
    notesLayout->addWidget(m_saveButton);
    notesLayout->addWidget(m_attachButton);
    notesLayout->addWidget(m_exportButton);
    panels->addWidget(notesCard, 4);
    main->addLayout(panels, 1);

    connect(m_cityFilter, &QComboBox::currentTextChanged, this, &TelemedicineView::filterDoctors);
    connect(m_specializationFilter, &QComboBox::currentTextChanged, this, &TelemedicineView::filterDoctors);
    connect(m_availabilityFilter, &QComboBox::currentTextChanged, this, &TelemedicineView::filterDoctors);
    connect(m_doctorList, &QListWidget::currentRowChanged, this, &TelemedicineView::handleDoctorSelected);
    connect(m_connectButton, &QPushButton::clicked, this, &TelemedicineView::handleConnect);
    connect(m_saveButton, &QPushButton::clicked, this, &TelemedicineView::saveConsultation);
    connect(m_attachButton, &QPushButton::clicked, this, &TelemedicineView::saveConsultation);
    connect(m_exportButton, &QPushButton::clicked, this, &TelemedicineView::exportConsultation);
}

void TelemedicineView::setPatient(const QString& patientId) {
    m_patientId = patientId;
    m_consultNotes->clear();
    m_saveButton->setEnabled(false);
    m_attachButton->setEnabled(false);
    m_exportButton->setEnabled(false);
}

void TelemedicineView::filterDoctors() {
    m_doctorList->clear();
    m_visibleDoctorIndexes.clear();
    for (int index = 0; index < m_doctors.size(); ++index) {
        const Doctor& doctor = m_doctors[index];
        if (m_cityFilter->currentText() != "All Cities" && doctor.city != m_cityFilter->currentText()) continue;
        if (m_specializationFilter->currentText() != "All Specializations" &&
            doctor.specialization != m_specializationFilter->currentText()) continue;
        if (m_availabilityFilter->currentText() != "All Availability" &&
            doctor.availability != m_availabilityFilter->currentText()) continue;
        m_visibleDoctorIndexes.append(index);
        auto* item = new QListWidgetItem(QString("%1\n%2 • %3\n● %4")
                                             .arg(doctor.name, doctor.specialization, doctor.city, doctor.availability));
        item->setSizeHint(QSize(220, 65));
        if (doctor.availability == "Online") item->setForeground(QColor("#DDF9E8"));
        m_doctorList->addItem(item);
    }
    if (!m_visibleDoctorIndexes.isEmpty()) m_doctorList->setCurrentRow(0);
}

void TelemedicineView::handleDoctorSelected(int row) {
    if (row < 0 || row >= m_visibleDoctorIndexes.size()) {
        m_selectedDoctorIndex = -1;
        return;
    }
    m_selectedDoctorIndex = m_visibleDoctorIndexes[row];
    updateDoctorDetails();
}

void TelemedicineView::updateDoctorDetails() {
    if (m_selectedDoctorIndex < 0 || m_selectedDoctorIndex >= m_doctors.size()) return;
    const Doctor& doctor = m_doctors[m_selectedDoctorIndex];
    m_doctorName->setText(doctor.name);
    m_doctorDetails->setText(QString("Specialization: %1\nQualification: %2\nExperience: %3 years\nHospital: %4\nCity: %5\nRating: %6 / 5.0")
                                 .arg(doctor.specialization, doctor.qualification)
                                 .arg(doctor.experienceYears)
                                 .arg(doctor.hospital, doctor.city)
                                 .arg(doctor.rating, 0, 'f', 1));
    const QString color = doctor.availability == "Online" ? "#2ECC71"
                           : doctor.availability == "Busy" ? "#F1C40F" : "#E74C3C";
    m_onlineIndicator->setText("● " + doctor.availability);
    m_onlineIndicator->setStyleSheet(QString("color:%1; font-weight:900;").arg(color));
    m_connectButton->setEnabled(doctor.availability == "Online" && !m_connectionInProgress);
}

void TelemedicineView::handleConnect() {
    if (m_selectedDoctorIndex < 0 || m_connectionInProgress) return;
    if (m_patientId.isEmpty()) {
        QMessageBox::warning(this, "Select Patient", "Select a patient before starting the demonstration consultation.");
        return;
    }
    m_connectionInProgress = true;
    m_connectionActive = false;
    m_connectButton->setEnabled(false);
    m_connectionStatus->setText("Connecting...");
    m_connectionStatus->setStyleSheet("background:rgba(241,196,15,.12); border-radius:8px; padding:10px; color:#F1C40F; font-weight:900;");
    m_consultNotes->clear();
    QTimer::singleShot(1200, this, [this]() {
        if (!m_connectionInProgress) return;
        m_connectionStatus->setText("Connected. Doctor reviewing report...");
    });
    QTimer::singleShot(3000, this, [this]() {
        if (!m_connectionInProgress) return;
        m_connectionInProgress = false;
        m_connectionActive = true;
        m_connectionStatus->setText("CONNECTED • EDUCATIONAL CONSULTATION");
        m_connectionStatus->setStyleSheet("background:rgba(46,204,113,.12); border-radius:8px; padding:10px; color:#2ECC71; font-weight:900;");
        beginTypingResponse();
    });
}

void TelemedicineView::beginTypingResponse() {
    m_typingText =
        "Based on the uploaded demonstration scan, I recommend scheduling a routine follow-up.\n\n"
        "No emergency intervention appears necessary within this simulated workflow. "
        "Continue observation and repeat screening after the recommended interval.\n\n"
        "Educational Demonstration Prototype — this is not medical advice or a clinical diagnosis.";
    m_typingIndex = 0;
    m_consultNotes->clear();
    m_typingTimer->start();
}

void TelemedicineView::typeNextCharacter() {
    if (m_typingIndex >= m_typingText.size()) {
        m_typingTimer->stop();
        m_saveButton->setEnabled(true);
        m_attachButton->setEnabled(true);
        m_exportButton->setEnabled(true);
        return;
    }
    m_consultNotes->insertPlainText(QString(m_typingText[m_typingIndex++]));
    m_consultNotes->moveCursor(QTextCursor::End);
}

void TelemedicineView::saveConsultation() {
    if (m_patientId.isEmpty() || m_consultNotes->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Consultation Not Ready", "A selected patient and completed demonstration note are required.");
        return;
    }
    const Doctor& doctor = m_doctors[m_selectedDoctorIndex];
    const QString notes = m_consultNotes->toPlainText().trimmed();
    DatabaseManager::instance().addLog(
        "Dr. Soham", "TELEMEDICINE_DEMO",
        QString("Patient %1 | Specialist %2 | %3").arg(m_patientId, doctor.name, notes));

    TwinRegionState state;
    state.patientId = m_patientId;
    state.regionName = "Telemedicine Consultation";
    state.status = "Documented";
    state.notes = QString("%1 (%2): %3").arg(doctor.name, doctor.specialization, notes);
    state.lastUpdated = QDateTime::currentDateTime().toString(Qt::ISODate);
    DatabaseManager::instance().updateTwinRegion(state);
    QMessageBox::information(this, "Consultation Attached",
                             "The demonstration consultation was added to patient history, Digital Twin notes, and timeline.\n\nNot medical advice.");
}

void TelemedicineView::exportConsultation() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Demonstration Consultation",
                                                      "AegisCare_Consultation.txt", "Text Document (*.txt)");
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Failed", "Could not create the consultation document.");
        return;
    }
    file.write(("AegisCare AI Educational Demonstration Consultation\n"
                "Not Intended For Clinical Diagnosis\n\nPatient: " + m_patientId + "\n\n" +
                m_consultNotes->toPlainText()).toUtf8());
    if (!file.commit()) {
        QMessageBox::critical(this, "Export Failed", "Could not finalize the consultation document.");
    }
}
