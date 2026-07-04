#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QMap>
#include <QDateTime>
#include <QFile>
#include "../models/Models.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool init(const QString& dbPath = "aegiscare.db");
    void close();
    QString databasePath() const { return m_dbPath; }

    // User Operations
    bool registerUser(const User& user);
    User authenticateUser(const QString& username, const QString& password);
    QList<User> getUsers();

    // Patient Operations
    bool addPatient(Patient& patient);
    bool updatePatient(const Patient& patient);
    bool deletePatient(const QString& patientId);
    Patient getPatient(const QString& patientId);
    QList<Patient> getPatients(const QString& search = "", const QString& sortBy = "id", bool desc = true);
    bool checkDuplicate(const QString& fullName, int age, const QString& phone);
    int getPatientCount();

    // Visit Operations
    bool addVisit(const Visit& visit);
    QList<Visit> getVisitsForPatient(const QString& patientId);
    int getVisitCountToday();

    // Twin State Operations
    bool updateTwinRegion(const TwinRegionState& state);
    QList<TwinRegionState> getTwinStatesForPatient(const QString& patientId);
    TwinRegionState getTwinRegionState(const QString& patientId, const QString& regionName);

    // Follow Up Operations
    bool scheduleFollowUp(const FollowUp& followUp);
    bool updateFollowUpStatus(int id, const QString& status);
    QList<FollowUp> getPendingFollowUps();
    QList<FollowUp> getAllFollowUps();
    int getOverdueFollowUpCount();

    // Clinical Voice Notes
    bool addVoiceNote(VoiceNote& note);
    QList<VoiceNote> getVoiceNotesForPatient(const QString& patientId);

    // Demonstration Reports
    bool addPatientReport(PatientReport& report);
    QList<PatientReport> getReportsForPatient(const QString& patientId);

    // Emergency Workflow
    bool addEmergencyEvent(EmergencyEvent& event);
    QList<EmergencyEvent> getEmergencyEventsForPatient(const QString& patientId);

    // Analytics and Village Reports
    QMap<QString, int> getPatientsPerVillageStats();
    QMap<QString, int> getVisitsPerDayStats();
    QMap<QString, int> getScreeningTypeStats();
    QMap<QString, int> getAgeDistributionStats();
    QMap<QString, int> getGenderDistributionStats();

    // Audit and Logs
    void addLog(const QString& username, const QString& action, const QString& details);
    QList<QMap<QString, QString>> getRecentLogs(int limit = 50);

    // Backup & Restore
    bool backupDatabase(const QString& backupPath);
    bool restoreDatabase(const QString& backupPath);

signals:
    void databaseUpdated();

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
    QString m_dbPath;

    void populateDemoData();
    bool migrateSchema();
    bool ensureDefaultAccounts();
};

#endif // DATABASEMANAGER_H
