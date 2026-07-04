#include "DemoReportGenerator.h"

#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>

namespace {
QString esc(QString value) {
    return value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
                .replace("\"", "&quot;").replace("'", "&#39;");
}
}

DemoReportGenerator& DemoReportGenerator::instance() {
    static DemoReportGenerator generator;
    return generator;
}

DemoReportGenerator::DemoReportGenerator(QObject* parent) : QObject(parent) {}

DemoReportGenerator::Result DemoReportGenerator::generateForPatient(const QString& patientId,
                                                                    const QString& operatorName) {
    Result result;
    if (patientId.trimmed().isEmpty()) {
        result.error = "No active patient selected.";
        return result;
    }

    DatabaseManager& db = DatabaseManager::instance();
    const Patient patient = db.getPatient(patientId);
    if (patient.patientId.isEmpty()) {
        result.error = "Patient record was not found.";
        return result;
    }

    const QString dirPath = reportsDirectory();
    if (!QDir().mkpath(dirPath)) {
        result.error = "Could not create reports directory.";
        return result;
    }

    const QString reportId = nextReportId();
    const QString fileName = reportId + ".html";
    const QString url = githubPagesUrl(fileName);
    const QString localPath = QDir(dirPath).filePath(fileName);
    const QString html = buildHtml(patient,
                                   db.getVisitsForPatient(patientId),
                                   db.getTwinStatesForPatient(patientId),
                                   db.getVoiceNotesForPatient(patientId),
                                   db.getAllFollowUps(),
                                   db.getEmergencyEventsForPatient(patientId),
                                   reportId,
                                   operatorName,
                                   url);

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        result.error = "Could not write the HTML report.";
        return result;
    }
    QTextStream out(&file);
    out << html;
    file.close();

    PatientReport report;
    report.patientId = patientId;
    report.reportId = reportId;
    report.githubUrl = url;
    report.localPath = localPath;
    report.generatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    report.visitNumber = db.getVisitsForPatient(patientId).size();
    report.qrGenerated = true;
    if (!db.addPatientReport(report)) {
        result.error = "HTML was generated but the report database entry failed.";
        return result;
    }

    result.success = true;
    result.report = report;
    return result;
}

QString DemoReportGenerator::reportsDirectory() const {
    QDir cwd(QDir::currentPath());
    if (cwd.dirName() == "DesktopApp") {
        return cwd.filePath("../reports");
    }
    if (QDir(cwd.filePath("DesktopApp")).exists()) {
        return cwd.filePath("reports");
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath("reports");
}

QString DemoReportGenerator::nextReportId() const {
    QDir dir(reportsDirectory());
    const QStringList files = dir.entryList(QStringList{"AEGIS-*.html"}, QDir::Files, QDir::Name);
    int maxNumber = 0;
    const QRegularExpression pattern("^AEGIS-(\\d+)\\.html$");
    for (const QString& file : files) {
        const auto match = pattern.match(file);
        if (match.hasMatch()) maxNumber = qMax(maxNumber, match.captured(1).toInt());
    }
    return QString("AEGIS-%1").arg(maxNumber + 1, 4, 10, QChar('0'));
}

QString DemoReportGenerator::githubPagesUrl(const QString& fileName) const {
    const QString base = QSettings().value("reports/github_pages_base",
                                           "https://username.github.io/AegisCareAI/reports").toString();
    return base.trimmed().replace(QRegularExpression("/+$"), "") + "/" + fileName;
}

QString DemoReportGenerator::buildHtml(const Patient& patient,
                                       const QList<Visit>& visits,
                                       const QList<TwinRegionState>& twinStates,
                                       const QList<VoiceNote>& voiceNotes,
                                       const QList<FollowUp>& followUps,
                                       const QList<EmergencyEvent>& emergencies,
                                       const QString& reportId,
                                       const QString& operatorName,
                                       const QString& url) const {
    QString html;
    QTextStream s(&html);
    s << "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
      << "<title>AegisCare AI " << esc(reportId) << "</title>"
      << "<style>body{margin:0;background:#0d1621;color:#e1e6eb;font-family:Inter,Arial,sans-serif}"
      << ".wrap{max-width:1120px;margin:auto;padding:28px}.hero{background:linear-gradient(135deg,#122030,#0b111a);border:1px solid rgba(255,255,255,.1);border-radius:20px;padding:26px}"
      << ".badge{display:inline-block;background:rgba(245,176,65,.14);color:#f5b041;padding:8px 12px;border-radius:999px;font-weight:800;margin:8px 0}"
      << ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:16px;margin-top:18px}.card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);border-radius:16px;padding:18px}"
      << "h1,h2{margin:.2rem 0}.muted{color:#a1aab3}.danger{color:#ff6b6b;font-weight:900}.ok{color:#2ecc71;font-weight:800}li{margin:.45rem 0}</style></head><body><div class='wrap'>";
    s << "<section class='hero'><h1>AegisCare AI</h1><h2>Educational Demonstration Report</h2>"
      << "<div class='badge'>Educational Demonstration Prototype • Not Intended For Clinical Diagnosis</div>"
      << "<p class='muted'>Generated by " << esc(operatorName) << " on "
      << esc(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) << "</p>"
      << "<p class='muted'>Public report URL: " << esc(url) << "</p></section>";

    s << "<div class='grid'><section class='card'><h2>Patient Information</h2>"
      << "<p><b>Name:</b> " << esc(patient.fullName) << "</p>"
      << "<p><b>ID:</b> " << esc(patient.patientId) << "</p>"
      << "<p><b>Age/Gender:</b> " << patient.age << " / " << esc(patient.gender) << "</p>"
      << "<p><b>Blood Group:</b> " << esc(patient.bloodGroup) << "</p>"
      << "<p><b>Address:</b> " << esc(patient.village) << "</p></section>";

    s << "<section class='card'><h2>Guided Screening Summary</h2>"
      << "<p><b>Current scan provider:</b> ESP32-CAM demonstration provider</p>"
      << "<p><b>Future compatible providers:</b> CT, MRI, X-Ray, Ultrasound, Dermoscopy, ECG, Blood Reports</p>"
      << "<p class='ok'>Demo Scan Complete</p>"
      << "<p>Simulated findings only. No real clinical analysis was performed.</p></section></div>";

    s << "<div class='grid'><section class='card'><h2>Digital Twin Summary</h2><ul>";
    for (const TwinRegionState& state : twinStates.mid(0, 8)) {
        s << "<li><b>" << esc(state.regionName) << ":</b> " << esc(state.status)
          << " — " << esc(state.notes) << "</li>";
    }
    s << "</ul></section>";

    s << "<section class='card'><h2>Clinical Voice Notes</h2>";
    if (voiceNotes.isEmpty()) {
        s << "<p class='muted'>No clinical voice note saved for this visit.</p>";
    } else {
        for (const VoiceNote& note : voiceNotes.mid(0, 3)) {
            s << "<p><b>" << esc(note.operatorName) << "</b> • " << esc(note.timestamp)
              << " • Audio available locally</p><p>" << esc(note.transcript) << "</p>";
        }
    }
    s << "</section></div>";

    s << "<div class='grid'><section class='card'><h2>Care Management Follow-up</h2><ul>";
    int followCount = 0;
    for (const FollowUp& follow : followUps) {
        if (follow.patientId != patient.patientId) continue;
        s << "<li>" << esc(follow.dueDate) << " — " << esc(follow.status)
          << ": " << esc(follow.notes) << "</li>";
        if (++followCount >= 5) break;
    }
    if (followCount == 0) s << "<li>No pending preventive tasks for this demonstration patient.</li>";
    s << "</ul></section>";

    s << "<section class='card'><h2>Visit Timeline</h2><ul>";
    for (const Visit& visit : visits.mid(0, 5)) {
        s << "<li>" << esc(visit.visitDate) << " — " << esc(visit.screeningType)
          << " demo screening. " << esc(visit.notes) << "</li>";
    }
    for (const EmergencyEvent& event : emergencies.mid(0, 3)) {
        s << "<li class='danger'>" << esc(event.timestamp) << " — Emergency "
          << esc(event.priority) << " activated by " << esc(event.operatorName) << "</li>";
    }
    s << "<li>Report generated: " << esc(reportId) << "</li></ul></section></div>";

    s << "<section class='card'><h2>Important Safety Notice</h2>"
      << "<p class='danger'>Educational Demonstration Prototype. Not Intended For Clinical Diagnosis.</p>"
      << "<p>This report demonstrates workflow, synchronization, documentation, and report-delivery concepts. It does not diagnose disease, detect pathology, or provide clinical decision support.</p>"
      << "</section></div></body></html>";
    return html;
}
