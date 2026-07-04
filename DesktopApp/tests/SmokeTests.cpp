#include "core/CameraQualityEngine.h"
#include "core/DatabaseManager.h"
#include "core/VoiceAssistant.h"
#include "core/WorkflowEngine.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTemporaryDir>

namespace {
bool require(bool condition, const char* message) {
    if (!condition) qCritical() << "Smoke test failed:" << message;
    return condition;
}
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("AegisCareSmokeTests");

    QTemporaryDir directory;
    if (!require(directory.isValid(), "temporary directory unavailable")) return 1;

    DatabaseManager& database = DatabaseManager::instance();
    if (!require(database.init(directory.filePath("aegiscare-test.db")), "database initialization")) return 1;
    if (!require(database.getPatientCount() >= 3, "demo patient seed")) return 1;

    const User operatorUser = database.authenticateUser("soham", "soham");
    if (!require(operatorUser.role == "Doctor", "hashed credential authentication")) return 1;
    if (!require(database.authenticateUser("soham", "wrong").username.isEmpty(), "invalid credential rejection")) return 1;

    Patient patient;
    patient.fullName = "Smoke Test Patient";
    patient.age = 31;
    patient.gender = "Other";
    patient.bloodGroup = "Unknown";
    patient.phone = "+91 90000 00000";
    patient.address = "Test Address";
    patient.village = "Test Village";
    patient.state = "Maharashtra";
    patient.country = "India";
    patient.emergencyContact = "+91 90000 00001";
    patient.medicalNotes = "Automated smoke-test record.";
    patient.doctorAssigned = "Unassigned";
    patient.operatorName = "Smoke Test";
    if (!require(database.addPatient(patient), "patient registration")) return 1;
    if (!require(database.checkDuplicate(patient.fullName, patient.age, patient.phone), "duplicate detection")) return 1;
    if (!require(database.getTwinStatesForPatient(patient.patientId).size() >= 10, "digital twin creation")) return 1;

    const QString backupPath = directory.filePath("backup.db");
    if (!require(database.backupDatabase(backupPath), "database backup")) return 1;
    if (!require(database.restoreDatabase(backupPath), "database restore validation")) return 1;

    VoiceAssistant::instance().setEnabled(false);
    WorkflowEngine::instance().resetWorkflow();
    WorkflowEngine::instance().nextStage();
    if (!require(WorkflowEngine::instance().currentStage() == WorkflowEngine::PatientRegistration,
                 "workflow transition")) return 1;

    QImage frame(640, 480, QImage::Format_RGB32);
    frame.fill(QColor(128, 128, 128));
    const auto quality = CameraQualityEngine::instance().analyzeFrame(frame);
    if (!require(quality.lighting > 95.0 && quality.totalScore >= 0.0 && quality.totalScore <= 100.0,
                 "camera quality bounds")) return 1;

    qInfo() << "AegisCare core smoke tests passed.";
    return 0;
}
