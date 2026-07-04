#ifndef TELEMEDICINEVIEW_H
#define TELEMEDICINEVIEW_H

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class TelemedicineView : public QWidget {
    Q_OBJECT
public:
    explicit TelemedicineView(QWidget* parent = nullptr);
    ~TelemedicineView() override;

    void setPatient(const QString& patientId);

private slots:
    void filterDoctors();
    void handleDoctorSelected(int row);
    void handleConnect();
    void typeNextCharacter();
    void saveConsultation();
    void exportConsultation();

private:
    struct Doctor {
        QString name;
        QString specialization;
        QString hospital;
        QString city;
        QString availability;
        QString qualification;
        int experienceYears = 0;
        double rating = 0.0;
    };

    QList<Doctor> m_doctors;
    QList<int> m_visibleDoctorIndexes;
    QListWidget* m_doctorList = nullptr;
    QComboBox* m_cityFilter = nullptr;
    QComboBox* m_specializationFilter = nullptr;
    QComboBox* m_availabilityFilter = nullptr;
    QLabel* m_doctorPhoto = nullptr;
    QLabel* m_doctorName = nullptr;
    QLabel* m_doctorDetails = nullptr;
    QLabel* m_onlineIndicator = nullptr;
    QLabel* m_connectionStatus = nullptr;
    QPushButton* m_connectButton = nullptr;
    QTextEdit* m_consultNotes = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_attachButton = nullptr;
    QPushButton* m_exportButton = nullptr;
    QTimer* m_typingTimer = nullptr;
    QString m_typingText;
    int m_typingIndex = 0;
    int m_selectedDoctorIndex = -1;
    QString m_patientId;
    bool m_connectionActive = false;
    bool m_connectionInProgress = false;

    void setupUi();
    void populateDoctors();
    void updateDoctorDetails();
    void beginTypingResponse();
};

#endif // TELEMEDICINEVIEW_H
