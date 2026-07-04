#include "AnalyticsView.h"
#include "../core/DatabaseManager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <QtCharts/QPieSlice>
#include <QScopedValueRollback>
#include <QHash>

AnalyticsView::AnalyticsView(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshData();
}

AnalyticsView::~AnalyticsView() {}

void AnalyticsView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // Style adjustments
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

    QLabel* viewTitle = new QLabel("COMMUNITY HEALTH INTELLIGENCE", this);
    viewTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #FFFFFF;");
    mainLayout->addWidget(viewTitle);

    // 1. Top Section: Pie Charts Row
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(15);

    m_ageChartView = new QChartView(this);
    m_ageChartView->setRenderHint(QPainter::Antialiasing);
    m_ageChartView->setStyleSheet("background: transparent; border-radius: 10px;");
    topRow->addWidget(m_ageChartView);

    m_genderChartView = new QChartView(this);
    m_genderChartView->setRenderHint(QPainter::Antialiasing);
    m_genderChartView->setStyleSheet("background: transparent; border-radius: 10px;");
    topRow->addWidget(m_genderChartView);

    mainLayout->addLayout(topRow, 1);

    // 2. Bottom Section: Address Metrics & Advisor recommendations
    QHBoxLayout* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(15);

    // Left Bottom: Table
    QVBoxLayout* tableCol = new QVBoxLayout();
    tableCol->setSpacing(8);
    tableCol->addWidget(new QLabel("Address-wise Screening Coverage:", this));

    m_villageTable = new QTableWidget(this);
    m_villageTable->setColumnCount(4);
    m_villageTable->setHorizontalHeaderLabels({"Address", "Registered Patients", "Follow-up Rate", "Clinic Priority"});
    m_villageTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_villageTable->verticalHeader()->setVisible(false);
    m_villageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableCol->addWidget(m_villageTable);
    bottomRow->addLayout(tableCol, 3);

    // Right Bottom: Recommendation Advisor card
    ModernCard* advisorCard = new ModernCard(this);
    advisorCard->setHoverEffect(false);
    advisorCard->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 12px; }");
    
    QVBoxLayout* advisorLayout = new QVBoxLayout(advisorCard);
    advisorLayout->setContentsMargins(15, 15, 15, 15);
    
    QLabel* advHeader = new QLabel("CLINIC OUTREACH DEMONSTRATION ADVISOR", this);
    advHeader->setStyleSheet("font-size: 11px; font-weight: bold; color: #00A8FF;");
    advisorLayout->addWidget(advHeader);
    advisorLayout->addSpacing(5);

    m_recommendationsText = new QTextEdit(this);
    m_recommendationsText->setReadOnly(true);
    m_recommendationsText->setStyleSheet("background-color: rgba(0,0,0,50); border: 1px solid rgba(255,255,255,0.06); border-radius: 6px; padding: 10px; color: #E1E6EB; font-size: 12px;");
    advisorLayout->addWidget(m_recommendationsText);

    bottomRow->addWidget(advisorCard, 2);
    mainLayout->addLayout(bottomRow, 1);
}

void AnalyticsView::refreshData() {
    if (m_refreshInProgress) return;
    QScopedValueRollback<bool> refreshGuard(m_refreshInProgress, true);
    setupCharts();
    setupVillageTable();
    generateAIRecommendations();
}

void AnalyticsView::setupCharts() {
    DatabaseManager& db = DatabaseManager::instance();

    // ==========================================
    // 1. Age Distribution Pie Chart
    // ==========================================
    QChart* ageChart = new QChart();
    ageChart->setBackgroundBrush(QBrush(QColor(16, 24, 32)));
    ageChart->setTitleBrush(QBrush(QColor(255, 255, 255)));
    ageChart->setTitle("Demographic Age Distribution");
    ageChart->setAnimationOptions(QChart::SeriesAnimations);

    QPieSeries* ageSeries = new QPieSeries();
    QMap<QString, int> ageStats = db.getAgeDistributionStats();

    QStringList colors = {"#3498DB", "#2ECC71", "#E67E22", "#E74C3C", "#9B59B6"};
    int idx = 0;
    for (auto it = ageStats.begin(); it != ageStats.end(); ++it) {
        QPieSlice* slice = ageSeries->append(it.key(), it.value());
        slice->setLabelVisible(it.value() > 0);
        slice->setLabelColor(QColor(161, 170, 179));
        slice->setColor(QColor(colors[idx % colors.size()]));
        idx++;
    }

    ageChart->addSeries(ageSeries);
    ageChart->legend()->setVisible(true);
    ageChart->legend()->setAlignment(Qt::AlignRight);
    ageChart->legend()->setLabelColor(QColor(161, 170, 179));

    QChart* oldAge = m_ageChartView->chart();
    m_ageChartView->setChart(ageChart);
    if (oldAge) oldAge->deleteLater();

    // ==========================================
    // 2. Gender Distribution Donut Chart
    // ==========================================
    QChart* genderChart = new QChart();
    genderChart->setBackgroundBrush(QBrush(QColor(16, 24, 32)));
    genderChart->setTitleBrush(QBrush(QColor(255, 255, 255)));
    genderChart->setTitle("Gender Patient Metrics");
    genderChart->setAnimationOptions(QChart::SeriesAnimations);

    QPieSeries* genderSeries = new QPieSeries();
    genderSeries->setHoleSize(0.35); // Donut style
    QMap<QString, int> genderStats = db.getGenderDistributionStats();

    idx = 0;
    QStringList genColors = {"#00A8FF", "#FF79C6", "#A1AAB3"};
    for (auto it = genderStats.begin(); it != genderStats.end(); ++it) {
        QPieSlice* slice = genderSeries->append(it.key(), it.value());
        slice->setLabelVisible(it.value() > 0);
        slice->setLabelColor(QColor(161, 170, 179));
        slice->setColor(QColor(genColors[idx % genColors.size()]));
        idx++;
    }

    genderChart->addSeries(genderSeries);
    genderChart->legend()->setVisible(true);
    genderChart->legend()->setAlignment(Qt::AlignRight);
    genderChart->legend()->setLabelColor(QColor(161, 170, 179));

    QChart* oldGender = m_genderChartView->chart();
    m_genderChartView->setChart(genderChart);
    if (oldGender) oldGender->deleteLater();
}

void AnalyticsView::setupVillageTable() {
    DatabaseManager& db = DatabaseManager::instance();
    QMap<QString, int> villageStats = db.getPatientsPerVillageStats();
    QList<FollowUp> followUps = db.getAllFollowUps();
    QHash<QString, QString> patientVillages;
    for (const Patient& patient : db.getPatients()) patientVillages.insert(patient.patientId, patient.village);

    m_villageTable->setRowCount(0);
    int row = 0;

    for (auto it = villageStats.begin(); it != villageStats.end(); ++it) {
        QString village = it.key();
        int patientCount = it.value();

        // Calculate follow-up completion rate for this village
        int totalFollows = 0;
        int completedFollows = 0;
        for (const auto& f : followUps) {
            // Find patient's village to link
            if (patientVillages.value(f.patientId) == village) {
                totalFollows++;
                if (f.status == "Completed") {
                    completedFollows++;
                }
            }
        }

        double rate = totalFollows > 0 ? (double(completedFollows) / totalFollows * 100.0) : 100.0;
        QString priority = "NOMINAL";
        if (rate < 50.0 && totalFollows > 1) priority = "HIGH RISK";
        else if (rate < 75.0) priority = "SUPPORT NEEDED";

        m_villageTable->insertRow(row);
        m_villageTable->setItem(row, 0, new QTableWidgetItem(village));
        m_villageTable->setItem(row, 1, new QTableWidgetItem(QString::number(patientCount)));
        m_villageTable->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(rate, 0, 'f', 1)));
        
        QTableWidgetItem* prioItem = new QTableWidgetItem(priority);
        if (priority == "HIGH RISK") {
            prioItem->setForeground(QBrush(QColor(231, 76, 60)));
        } else if (priority == "SUPPORT NEEDED") {
            prioItem->setForeground(QBrush(QColor(241, 196, 15)));
        } else {
            prioItem->setForeground(QBrush(QColor(46, 204, 113)));
        }
        m_villageTable->setItem(row, 3, prioItem);
        row++;
    }
}

void AnalyticsView::generateAIRecommendations() {
    DatabaseManager& db = DatabaseManager::instance();
    QMap<QString, int> villageStats = db.getPatientsPerVillageStats();
    QList<FollowUp> followUps = db.getAllFollowUps();
    QHash<QString, QString> patientVillages;
    for (const Patient& patient : db.getPatients()) patientVillages.insert(patient.patientId, patient.village);

    QString recHtml = "<html><body style='font-family:Monospace;'>";
    recHtml += "<h3 style='color:#00A8FF; margin-top:0;'>RULE-BASED DEMONSTRATION GUIDANCE</h3>";
    recHtml += "<p style='color:#F5B041;'>Educational Prototype — deterministic thresholds, not medical prediction.</p>";

    bool checkAlerts = false;
    for (auto it = villageStats.begin(); it != villageStats.end(); ++it) {
        QString village = it.key();
        
        // Compute overdue checks
        int missedCount = 0;
        int total = 0;
        for (const auto& f : followUps) {
            if (patientVillages.value(f.patientId) == village) {
                total++;
                if (f.status == "Missed") missedCount++;
            }
        }

        if (missedCount > 0) {
            recHtml += QString("<div style='margin-bottom:12px; border-left:3px solid #E74C3C; padding-left:8px;'>") +
                       QString("<b>📍 OUTREACH CAMP: %1</b><br>").arg(village.toUpper()) +
                       QString("<span style='color:#E74C3C;'>CRITICAL WARNING:</span> %1 follow-up check-ups missed. ").arg(missedCount) +
                       QString("A mobile medical screening camp is advised to restore clinic continuity in this region.") +
                       QString("</div>");
            checkAlerts = true;
        } else if (it.value() >= 3) {
            recHtml += QString("<div style='margin-bottom:12px; border-left:3px solid #F1C40F; padding-left:8px;'>") +
                       QString("<b>📍 DOCTOR DEPLOYMENT: %1</b><br>").arg(village.toUpper()) +
                       QString("High active registry load (%1 patients). ").arg(it.value()) +
                       QString("Recommend scheduling a secondary doctor support session next week.") +
                       QString("</div>");
            checkAlerts = true;
        }
    }

    if (!checkAlerts) {
        recHtml += "<div style='color:#2ECC71;'><b>🟢 WORKSTATION METRICS NOMINAL</b><br>"
                   "All village outreach coverage meets clinical thresholds. Continue general follow-up queues.</div>";
    }

    recHtml += "</body></html>";
    m_recommendationsText->setHtml(recHtml);
}
