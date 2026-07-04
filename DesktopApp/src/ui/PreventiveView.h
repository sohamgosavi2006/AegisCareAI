#ifndef PREVENTIVEVIEW_H
#define PREVENTIVEVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "components/ModernCard.h"

class PreventiveView : public QWidget {
    Q_OBJECT
public:
    explicit PreventiveView(QWidget* parent = nullptr);
    virtual ~PreventiveView();

    void refreshData();

private slots:
    void handleScheduleOutreach();
    void handleActionTrigger(int row, int col);

private:
    QTableWidget* m_outreachTable = nullptr;
    
    // Schedule form controls
    QComboBox* m_patientCombo = nullptr;
    QComboBox* m_outreachTypeCombo = nullptr;
    QDateEdit* m_dueDateEdit = nullptr;
    QLineEdit* m_notesInput = nullptr;
    QPushButton* m_commitBtn = nullptr;

    // Summary widgets
    QLabel* m_missedCountLabel = nullptr;
    QLabel* m_pendingCountLabel = nullptr;
    QLabel* m_scorecardLabel = nullptr;
    bool m_refreshInProgress = false;
    bool m_actionInProgress = false;

    void setupUi();
};

#endif // PREVENTIVEVIEW_H
