#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QDateTime>
#include <QMetaType>

struct User {
    QString username;
    QString password;
    QString role; // "Administrator" or "Doctor"
    QString fullName;
};

struct Patient {
    int id = 0;
    QString patientId;
    QString qrId;
    QString fullName;
    int age = 0;
    QString gender;
    QString bloodGroup;
    QString phone;
    QString address;
    QString village;
    QString state;
    QString country;
    QString emergencyContact;
    QString medicalNotes;
    QString doctorAssigned;
    QString operatorName;
    QString photoPath;
    QString createdAt;
};

struct Visit {
    int id = 0;
    QString patientId;
    QString visitDate;
    int heartRate = 0;
    int sysBP = 0;
    int diaBP = 0;
    int spo2 = 0;
    double temperature = 0.0;
    QString screeningType; // e.g. "Heart", "Lungs", "Brain"
    QString notes;
    QString operatorName;
};

struct TwinRegionState {
    QString patientId;
    QString regionName; // "Brain", "Heart", "Lungs", "Liver", "Kidney", "Eyes" etc.
    QString status; // "Normal", "Review", "Warning", "Critical"
    QString notes;
    QString lastUpdated;
};

struct FollowUp {
    int id = 0;
    QString patientId;
    QString fullName;
    QString dueDate;
    QString status; // "Pending", "Completed", "Missed"
    QString notes;
};

struct VoiceNote {
    int id = 0;
    QString patientId;
    QString timestamp;
    QString operatorName;
    QString audioFilePath;
    QString transcript;
    int recordingLengthSec = 0;
    int visitNumber = 0;
};

struct PatientReport {
    int id = 0;
    QString patientId;
    QString reportId;
    QString githubUrl;
    QString localPath;
    QString generatedAt;
    int visitNumber = 0;
    bool qrGenerated = false;
};

struct EmergencyEvent {
    int id = 0;
    QString patientId;
    QString timestamp;
    QString operatorName;
    QString priority;
    QString details;
};

Q_DECLARE_METATYPE(User)
Q_DECLARE_METATYPE(Patient)
Q_DECLARE_METATYPE(Visit)
Q_DECLARE_METATYPE(TwinRegionState)
Q_DECLARE_METATYPE(FollowUp)
Q_DECLARE_METATYPE(VoiceNote)
Q_DECLARE_METATYPE(PatientReport)
Q_DECLARE_METATYPE(EmergencyEvent)

#endif // MODELS_H
