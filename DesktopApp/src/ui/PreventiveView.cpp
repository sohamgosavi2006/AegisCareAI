#include "PreventiveView.h"
#include "../core/DatabaseManager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScopedValueRollback>

PreventiveView::PreventiveView(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshData();
}

PreventiveView::~PreventiveView() {}

void PreventiveView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // Styling
    setStyleSheet(
        "QTableWidget {"
        "  background-color: rgba(255, 255, 255, 0.02);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 8px;"
        "  color: #E1E6EB;"
        "  gridline-color: rgba(255, 255, 255, 0.04);"
        "}"
        "QHeaderView::section {"
        "  background-color: #122030;"
        "  color: #A1AAB3;"
        "  padding: 6px;"
        "  border: none;"
        "  font-weight: bold;"
        "}"
    );

    QLabel* viewTitle = new QLabel("PREVENTIVE CARE COORDINATOR", this);
    viewTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #FFFFFF;");
    mainLayout->addWidget(viewTitle);

    // 1. Top summary metrics
    QHBoxLayout* topMetrics = new QHBoxLayout();
    topMetrics->setSpacing(15);

    auto makeMetricBox = [&](const QString& label, const QString& val, const QString& colorHex) {
        ModernCard* card = new ModernCard(this);
        card->setMinimumHeight(80);
        card->setHoverEffect(false);
        card->setStyleSheet("QFrame { background-color: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; }");
        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setContentsMargins(15, 8, 15, 8);
        QLabel* tl = new QLabel(label, card);
        tl->setStyleSheet("font-size: 9px; font-weight: bold; color: #A1AAB3;");
        QLabel* vl = new QLabel(val, card);
        vl->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(colorHex));
        cl->addWidget(tl);
        cl->addWidget(vl);
        topMetrics->addWidget(card);
        return vl;
    };

    m_missedCountLabel = makeMetricBox("OVERDUE / MISSED CARE VISITS", "0", "#E74C3C");
    m_pendingCountLabel = makeMetricBox("PENDING CLINIC OUTREACHES", "0", "#F1C40F");
    m_scorecardLabel = makeMetricBox("PREVENTIVE CARE COMPLETION SCORECARD", "100%", "#2ECC71");

    mainLayout->addLayout(topMetrics);

    // 2. Main layout split
    QHBoxLayout* mainGrid = new QHBoxLayout();
    mainGrid->setSpacing(15);

    // Left Column: Outreach Table
    QVBoxLayout* tableCol = new QVBoxLayout();
    tableCol->setSpacing(8);
    tableCol->addWidget(new QLabel("Outreach Calendar & Action Items:", this));
    
    m_outreachTable = new QTableWidget(this);
    m_outreachTable->setColumnCount(6);
    m_outreachTable->setHorizontalHeaderLabels({"Patient ID", "Full Name", "Target Date", "Type", "Status", "Quick Action"});
    m_outreachTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_outreachTable->verticalHeader()->setVisible(false);
    m_outreachTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_outreachTable, &QTableWidget::cellClicked, this, &PreventiveView::handleActionTrigger);
    
    tableCol->addWidget(m_outreachTable);
    mainGrid->addLayout(tableCol, 3);

    // Right Column: Scheduler Form
    ModernCard* schedulerCard = new ModernCard(this);
    schedulerCard->setHoverEffect(false);
    schedulerCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* sLayout = new QVBoxLayout(schedulerCard);
    sLayout->setContentsMargins(15, 15, 15, 15);
    sLayout->setSpacing(12);

    QLabel* sHeader = new QLabel("🗓️ CLINICAL CARE VISIT SCHEDULER", this);
    sHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    sLayout->addWidget(sHeader);

    QFormLayout* formLayout = new QFormLayout();
    m_patientCombo = new QComboBox(this);
    m_patientCombo->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    formLayout->addRow("Select Patient:", m_patientCombo);

    m_outreachTypeCombo = new QComboBox(this);
    m_outreachTypeCombo->addItems({"Annual Health Checkup", "Cardiology Followup", "Pulmonary Screening", "Ophthalmology Review", "Immunization Booster", "Maternal Wellness Visit"});
    m_outreachTypeCombo->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    formLayout->addRow("Care Type:", m_outreachTypeCombo);

    m_dueDateEdit = new QDateEdit(QDate::currentDate().addDays(7), this);
    m_dueDateEdit->setStyleSheet("background-color: rgba(0,0,0,80); padding: 7px; color: #E1E6EB;");
    m_dueDateEdit->setCalendarPopup(true);
    formLayout->addRow("Scheduled Date:", m_dueDateEdit);

    m_notesInput = new QLineEdit(this);
    m_notesInput->setPlaceholderText("Vaccine batch details, dosage instructions...");
    m_notesInput->setStyleSheet("background-color: rgba(0,0,0,80); padding: 8px; color: #FFFFFF;");
    formLayout->addRow("Schedule Notes:", m_notesInput);

    sLayout->addLayout(formLayout);

    m_commitBtn = new QPushButton("ADD TO CARE CALENDAR", this);
    m_commitBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #00A8FF; border: none; border-radius: 4px; padding: 10px; color: #FFFFFF; font-weight: bold; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #0088E6; }"
    );
    connect(m_commitBtn, &QPushButton::clicked, this, &PreventiveView::handleScheduleOutreach);
    sLayout->addWidget(m_commitBtn);
    
    // Outreach Templates info
    QLabel* templatesHeader = new QLabel("Clinic Campaign Templates:", this);
    templatesHeader->setStyleSheet("font-size: 10px; font-weight: bold; color: #A1AAB3; margin-top: 10px;");
    sLayout->addWidget(templatesHeader);

    QLabel* flyerDesc = new QLabel(
        "📄 Annual Screening Camp Circular\n"
        "💬 Follow-up Outreach SMS Prompt\n"
        "💉 Vaccination Reminder Script", this
    );
    flyerDesc->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.4); line-height: 15px;");
    sLayout->addWidget(flyerDesc);

    mainGrid->addWidget(schedulerCard, 2);
    mainLayout->addLayout(mainGrid);
}

void PreventiveView::refreshData() {
    if (m_refreshInProgress) return;
    QScopedValueRollback<bool> refreshGuard(m_refreshInProgress, true);
    DatabaseManager& db = DatabaseManager::instance();
    QList<FollowUp> list = db.getAllFollowUps();
    QList<Patient> patients = db.getPatients();

    // 1. Refresh Patient Selector Dropdown
    m_patientCombo->blockSignals(true);
    m_patientCombo->clear();
    for (const auto& p : patients) {
        m_patientCombo->addItem(QString("%1 (%2)").arg(p.fullName).arg(p.patientId), p.patientId);
    }
    m_patientCombo->blockSignals(false);

    // 2. Count metrics
    int missed = 0;
    int pending = 0;
    int completed = 0;
    
    for (const auto& f : list) {
        if (f.status == "Missed") missed++;
        else if (f.status == "Pending") pending++;
        else if (f.status == "Completed") completed++;
    }

    int total = list.size();
    double compliance = total > 0 ? (double(completed) / total * 100.0) : 100.0;

    m_missedCountLabel->setText(QString::number(missed));
    m_pendingCountLabel->setText(QString::number(pending));
    m_scorecardLabel->setText(QString("%1%").arg(compliance, 0, 'f', 1));

    // 3. Populate table
    m_outreachTable->setRowCount(0);
    int row = 0;
    for (const auto& f : list) {
        m_outreachTable->insertRow(row);
        m_outreachTable->setItem(row, 0, new QTableWidgetItem(f.patientId));
        m_outreachTable->setItem(row, 1, new QTableWidgetItem(f.fullName));
        
        QString dateStr = QDate::fromString(f.dueDate, Qt::ISODate).toString("dd-MMM-yyyy");
        m_outreachTable->setItem(row, 2, new QTableWidgetItem(dateStr.isEmpty() ? f.dueDate : dateStr));
        m_outreachTable->setItem(row, 3, new QTableWidgetItem(f.notes)); // Type/Notes
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(f.status);
        if (f.status == "Missed") {
            statusItem->setForeground(QBrush(QColor(231, 76, 60))); // Red
        } else if (f.status == "Pending") {
            statusItem->setForeground(QBrush(QColor(241, 196, 15))); // Yellow
        } else {
            statusItem->setForeground(QBrush(QColor(46, 204, 113))); // Green
        }
        statusItem->setFont(QFont("Inter", 11, QFont::Bold));
        m_outreachTable->setItem(row, 4, statusItem);

        // Action column
        QTableWidgetItem* actionItem = new QTableWidgetItem(f.status == "Completed" ? "🔒 DONE" : "⚡ COMPLETE");
        actionItem->setForeground(QBrush(f.status == "Completed" ? QColor(161, 170, 179) : QColor(0, 168, 255)));
        actionItem->setFont(QFont("Inter", 10, QFont::Bold));
        actionItem->setData(Qt::UserRole, f.id);
        m_outreachTable->setItem(row, 5, actionItem);

        row++;
    }
}

void PreventiveView::handleScheduleOutreach() {
    if (m_actionInProgress) return;
    QScopedValueRollback<bool> actionGuard(m_actionInProgress, true);
    if (m_patientCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Schedule Error", "Please select a valid registered patient.");
        return;
    }

    QString patientId = m_patientCombo->currentData().toString();
    QString pText = m_patientCombo->currentText();
    QString fullName = pText.left(pText.indexOf("(")).trimmed();

    FollowUp f;
    f.patientId = patientId;
    f.fullName = fullName;
    f.dueDate = m_dueDateEdit->date().toString(Qt::ISODate);
    f.status = "Pending";
    
    QString type = m_outreachTypeCombo->currentText();
    QString note = m_notesInput->text().trimmed();
    f.notes = type + (note.isEmpty() ? "" : ": " + note);

    if (DatabaseManager::instance().scheduleFollowUp(f)) {
        m_notesInput->clear();
        refreshData();
        QMessageBox::information(this, "Care Scheduled", "Outreach visit scheduled successfully!");
    } else {
        QMessageBox::critical(this, "Database Error", "Could not write follow-up schedule to SQLite.");
    }
}

void PreventiveView::handleActionTrigger(int row, int col) {
    if (m_actionInProgress || row < 0 || row >= m_outreachTable->rowCount()) return;
    if (col == 5) {
        QTableWidgetItem* item = m_outreachTable->item(row, col);
        if (!item) return;
        QScopedValueRollback<bool> actionGuard(m_actionInProgress, true);
        if (item->text().contains("DONE")) return;

        int dbId = item->data(Qt::UserRole).toInt();
        
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Outreach Resolution", 
                                      "Confirm that this preventive outreach checkup session was successfully completed?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (DatabaseManager::instance().updateFollowUpStatus(dbId, "Completed")) {
                refreshData();
            }
        }
    }
}
