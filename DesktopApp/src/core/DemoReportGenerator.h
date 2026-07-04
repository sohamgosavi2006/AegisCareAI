#ifndef DEMOREPORTGENERATOR_H
#define DEMOREPORTGENERATOR_H

#include <QObject>
#include <QString>

#include "../models/Models.h"

class DemoReportGenerator : public QObject {
    Q_OBJECT
public:
    struct Result {
        bool success = false;
        QString error;
        PatientReport report;
    };

    static DemoReportGenerator& instance();
    Result generateForPatient(const QString& patientId,
                              const QString& operatorName = QStringLiteral("Dr. Soham"));

private:
    explicit DemoReportGenerator(QObject* parent = nullptr);
    QString reportsDirectory() const;
    QString nextReportId() const;
    QString githubPagesUrl(const QString& fileName) const;
    QString buildHtml(const Patient& patient,
                      const QList<Visit>& visits,
                      const QList<TwinRegionState>& twinStates,
                      const QList<VoiceNote>& voiceNotes,
                      const QList<FollowUp>& followUps,
                      const QList<EmergencyEvent>& emergencies,
                      const QString& reportId,
                      const QString& operatorName,
                      const QString& url) const;
};

#endif // DEMOREPORTGENERATOR_H
