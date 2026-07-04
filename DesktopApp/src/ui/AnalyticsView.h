#ifndef ANALYTICSVIEW_H
#define ANALYTICSVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QTextEdit>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include "components/ModernCard.h"

class AnalyticsView : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsView(QWidget* parent = nullptr);
    virtual ~AnalyticsView();

    void refreshData();

private:
    QChartView* m_ageChartView = nullptr;
    QChartView* m_genderChartView = nullptr;
    QTableWidget* m_villageTable = nullptr;
    QTextEdit* m_recommendationsText = nullptr;
    bool m_refreshInProgress = false;

    void setupUi();
    void setupCharts();
    void setupVillageTable();
    void generateAIRecommendations();
};

#endif // ANALYTICSVIEW_H
