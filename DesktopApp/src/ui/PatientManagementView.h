#ifndef PATIENTMANAGEMENTVIEW_H
#define PATIENTMANAGEMENTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include "../models/Models.h"

class PatientManagementView : public QWidget {
    Q_OBJECT
public:
    explicit PatientManagementView(QWidget* parent = nullptr);
    virtual ~PatientManagementView();

    void refreshPatientList();
    void selectPatient(const QString& patientId);

signals:
    void patientSelected(const QString& patientId);
    void workflowCompleted();

private slots:
    void handleRegister();
    void handleSearch();
    void handleRowSelected(int row, int column);
    void checkDuplicateRecord();
    void clearForm();
    void drawPatientQR(const QString& qrText);
    void handleRemotePatientChanged(int index);
    void handleRemotePatientConfirm();
    void handleArduinoConnectionChanged(bool connected);

private:
    // Left side: Patient List
    QLineEdit* m_searchBar = nullptr;
    QComboBox* m_filterVillage = nullptr;
    QTableWidget* m_patientTable = nullptr;
    
    // Right side: Registration Form & Card Details
    QLineEdit* m_nameInput = nullptr;
    QSpinBox* m_ageInput = nullptr;
    QComboBox* m_genderCombo = nullptr;
    QComboBox* m_bloodCombo = nullptr;
    QLineEdit* m_phoneInput = nullptr;
    QLineEdit* m_addressInput = nullptr;
    QLineEdit* m_villageInput = nullptr;
    QLineEdit* m_emergencyInput = nullptr;
    QComboBox* m_doctorCombo = nullptr;
    QTextEdit* m_notesInput = nullptr;
    
    QPushButton* m_registerBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    
    // Detail display labels
    QLabel* m_detailIdLabel = nullptr;
    QLabel* m_detailNameLabel = nullptr;
    QLabel* m_detailAgeGenderLabel = nullptr;
    QLabel* m_detailPhoneLabel = nullptr;
    QLabel* m_detailAddressLabel = nullptr;
    QLabel* m_detailEmergencyLabel = nullptr;
    QLabel* m_detailDoctorLabel = nullptr;
    QLabel* m_detailStatusLabel = nullptr;
    QLabel* m_detailLastVisitLabel = nullptr;
    QLabel* m_detailRiskLabel = nullptr;
    QLabel* m_detailTwinLabel = nullptr;
    QLabel* m_photoLabel = nullptr;
    QLabel* m_qrCodeDisplay = nullptr;
    QListWidget* m_previousVisitsList = nullptr;
    QListWidget* m_voiceNotesList = nullptr;
    QListWidget* m_reportsList = nullptr;
    QListWidget* m_followUpsList = nullptr;
    
    QList<Patient> m_currentPatients;
    QList<Patient> m_visiblePatients;
    QString m_selectedPatientId;
    bool m_applyingRemoteSelection = false;
    bool m_refreshInProgress = false;
    bool m_actionInProgress = false;
    QTimer* m_searchDebounce = nullptr;
    QTimer* m_duplicateDebounce = nullptr;

    void setupUi();
    void updateDetailPanel(const Patient& p);
    void synchronizePatientWindow(int selectedIndex);
    void sendSelectedPatient(int index);
};

#endif // PATIENTMANAGEMENTVIEW_H
