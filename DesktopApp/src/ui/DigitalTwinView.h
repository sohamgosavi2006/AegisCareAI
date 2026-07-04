#ifndef DIGITALTWINVIEW_H
#define DIGITALTWINVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QListWidget>
#include "components/DigitalTwinWidget.h"

class DigitalTwinView : public QWidget {
    Q_OBJECT
public:
    explicit DigitalTwinView(QWidget* parent = nullptr);
    virtual ~DigitalTwinView();

    void setPatient(const QString& patientId);
    void refreshTwinData();

signals:
    void screeningRequested(const QString& regionName);

private slots:
    void handleRegionClicked(const QString& regionName);
    void handleRegionHovered(const QString& regionName);
    void handleSaveStatus();
    void handleRotateClick();
    void handleViewModeCombo(const QString& modeText);

private:
    DigitalTwinWidget* m_twinWidget = nullptr;
    
    // Controls for twin widget
    QPushButton* m_rotateBtn = nullptr;
    QPushButton* m_resetZoomBtn = nullptr;
    QComboBox* m_viewCombo = nullptr;

    // Right panel: region summary
    QLabel* m_selectedRegionLabel = nullptr;
    QLabel* m_statusBadge = nullptr;
    QLabel* m_lastUpdatedLabel = nullptr;
    QListWidget* m_timelineList = nullptr;
    
    // Status modifier form
    QComboBox* m_statusFormCombo = nullptr;
    QTextEdit* m_notesFormInput = nullptr;
    QPushButton* m_saveStatusBtn = nullptr;
    QPushButton* m_screenBtn = nullptr;

    QString m_patientId;
    QString m_activeRegion;
    bool m_refreshInProgress = false;
    bool m_saveInProgress = false;

    void setupUi();
    void loadRegionTimeline(const QString& regionName);
};

#endif // DIGITALTWINVIEW_H
