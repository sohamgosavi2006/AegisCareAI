#include "DatabaseManager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

namespace {
constexpr int PasswordIterations = 120000;
constexpr int PasswordKeyLength = 32;

QByteArray createPasswordSalt() {
    QByteArray salt;
    salt.reserve(32);
    salt.append(QByteArray::number(QRandomGenerator::system()->generate64(), 16));
    salt.append(QByteArray::number(QRandomGenerator::system()->generate64(), 16));
    return salt;
}

QByteArray passwordDigest(const QString& password, const QByteArray& salt) {
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,
                                               password.toUtf8(), salt,
                                               PasswordIterations, PasswordKeyLength);
}

QString databaseText(const QString& value) {
    return value.isNull() ? QStringLiteral("") : value;
}

bool copyFileAtomically(const QString& sourcePath, const QString& destinationPath) {
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) return false;

    QSaveFile destination(destinationPath);
    if (!destination.open(QIODevice::WriteOnly)) return false;

    while (!source.atEnd()) {
        const QByteArray chunk = source.read(1024 * 1024);
        if (chunk.isEmpty() && source.error() != QFileDevice::NoError) return false;
        if (destination.write(chunk) != chunk.size()) return false;
    }
    return destination.commit();
}
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::init(const QString& dbPath) {
    m_dbPath = dbPath;
    
    // Use writable location if default
    if (m_dbPath == "aegiscare.db") {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataDir.isEmpty() || !QDir().mkpath(appDataDir) || !QFileInfo(appDataDir).isWritable()) {
            appDataDir = QDir::current().filePath("data");
            if (!QDir().mkpath(appDataDir)) {
                appDataDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + QDir::separator() + "AegisCareAI";
                QDir().mkpath(appDataDir);
            }
            qWarning() << "Using portable database directory:" << appDataDir;
        }
        m_dbPath = appDataDir + QDir::separator() + "aegiscare.db";
    }

    qDebug() << "Initializing Database at:" << m_dbPath;

    if (!m_db.isValid()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    } else if (m_db.isOpen()) {
        m_db.close();
    }
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Enable foreign keys
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON;");
    pragma.exec("PRAGMA journal_mode = WAL;");
    pragma.exec("PRAGMA busy_timeout = 5000;");

    // Create Tables
    QSqlQuery query(m_db);
    
    // Users Table
    bool ok = query.exec("CREATE TABLE IF NOT EXISTS users ("
                         "username TEXT PRIMARY KEY, "
                         "password TEXT NOT NULL DEFAULT '', "
                         "password_hash TEXT, "
                         "password_salt TEXT, "
                         "role TEXT NOT NULL, "
                         "full_name TEXT NOT NULL"
                         ");");
    if (!ok) qWarning() << "Error creating users table:" << query.lastError().text();

    // Patients Table
    ok = query.exec("CREATE TABLE IF NOT EXISTS patients ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT UNIQUE NOT NULL, "
                    "qr_id TEXT NOT NULL, "
                    "full_name TEXT NOT NULL, "
                    "age INTEGER NOT NULL, "
                    "gender TEXT NOT NULL, "
                    "blood_group TEXT NOT NULL, "
                    "phone TEXT NOT NULL, "
                    "address TEXT NOT NULL, "
                    "village TEXT NOT NULL, "
                    "state TEXT NOT NULL, "
                    "country TEXT NOT NULL, "
                    "emergency_contact TEXT NOT NULL, "
                    "medical_notes TEXT NOT NULL, "
                    "doctor_assigned TEXT NOT NULL, "
                    "operator_name TEXT NOT NULL, "
                    "photo_path TEXT NOT NULL, "
                    "created_at TEXT NOT NULL"
                    ");");
    if (!ok) qWarning() << "Error creating patients table:" << query.lastError().text();

    // Visits Table
    ok = query.exec("CREATE TABLE IF NOT EXISTS visits ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT NOT NULL, "
                    "visit_date TEXT NOT NULL, "
                    "heart_rate INTEGER NOT NULL, "
                    "sys_bp INTEGER NOT NULL, "
                    "dia_bp INTEGER NOT NULL, "
                    "spo2 INTEGER NOT NULL, "
                    "temperature REAL NOT NULL, "
                    "screening_type TEXT NOT NULL, "
                    "notes TEXT NOT NULL, "
                    "operator_name TEXT NOT NULL, "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating visits table:" << query.lastError().text();

    // Twin States Table
    ok = query.exec("CREATE TABLE IF NOT EXISTS twin_states ("
                    "patient_id TEXT NOT NULL, "
                    "region_name TEXT NOT NULL, "
                    "status TEXT NOT NULL, "
                    "notes TEXT NOT NULL, "
                    "last_updated TEXT NOT NULL, "
                    "PRIMARY KEY(patient_id, region_name), "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating twin_states table:" << query.lastError().text();

    // Follow-ups Table
    ok = query.exec("CREATE TABLE IF NOT EXISTS follow_ups ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT NOT NULL, "
                    "full_name TEXT NOT NULL, "
                    "due_date TEXT NOT NULL, "
                    "status TEXT NOT NULL, "
                    "notes TEXT NOT NULL, "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating follow_ups table:" << query.lastError().text();

    // System Logs Table
    ok = query.exec("CREATE TABLE IF NOT EXISTS system_logs ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "timestamp TEXT NOT NULL, "
                    "username TEXT NOT NULL, "
                    "action TEXT NOT NULL, "
                    "details TEXT NOT NULL"
                    ");");
    if (!ok) qWarning() << "Error creating system_logs table:" << query.lastError().text();

    ok = query.exec("CREATE TABLE IF NOT EXISTS voice_notes ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT NOT NULL, "
                    "timestamp TEXT NOT NULL, "
                    "operator_name TEXT NOT NULL, "
                    "audio_file_path TEXT NOT NULL, "
                    "transcript TEXT NOT NULL, "
                    "recording_length_sec INTEGER NOT NULL, "
                    "visit_number INTEGER NOT NULL DEFAULT 0, "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating voice_notes table:" << query.lastError().text();

    ok = query.exec("CREATE TABLE IF NOT EXISTS patient_reports ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT NOT NULL, "
                    "report_id TEXT NOT NULL UNIQUE, "
                    "github_url TEXT NOT NULL, "
                    "local_path TEXT NOT NULL, "
                    "generated_at TEXT NOT NULL, "
                    "visit_number INTEGER NOT NULL DEFAULT 0, "
                    "qr_generated INTEGER NOT NULL DEFAULT 0, "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating patient_reports table:" << query.lastError().text();

    ok = query.exec("CREATE TABLE IF NOT EXISTS emergency_events ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "patient_id TEXT NOT NULL, "
                    "timestamp TEXT NOT NULL, "
                    "operator_name TEXT NOT NULL, "
                    "priority TEXT NOT NULL, "
                    "details TEXT NOT NULL, "
                    "FOREIGN KEY(patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE"
                    ");");
    if (!ok) qWarning() << "Error creating emergency_events table:" << query.lastError().text();

    if (!migrateSchema()) {
        qCritical() << "Database schema migration failed.";
        return false;
    }

    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_patients_name ON patients(full_name)",
        "CREATE INDEX IF NOT EXISTS idx_patients_phone ON patients(phone)",
        "CREATE INDEX IF NOT EXISTS idx_patients_village ON patients(village)",
        "CREATE INDEX IF NOT EXISTS idx_visits_patient_date ON visits(patient_id, visit_date DESC)",
        "CREATE INDEX IF NOT EXISTS idx_followups_status_due ON follow_ups(status, due_date)",
        "CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON system_logs(timestamp DESC)",
        "CREATE INDEX IF NOT EXISTS idx_voice_notes_patient_time ON voice_notes(patient_id, timestamp DESC)",
        "CREATE INDEX IF NOT EXISTS idx_reports_patient_time ON patient_reports(patient_id, generated_at DESC)",
        "CREATE INDEX IF NOT EXISTS idx_emergency_patient_time ON emergency_events(patient_id, timestamp DESC)"
    };
    for (const QString& statement : indexes) {
        if (!query.exec(statement)) {
            qWarning() << "Unable to create database index:" << query.lastError().text();
        }
    }

    // Populate demo data if table is empty
    QSqlQuery checkUsers("SELECT COUNT(*) FROM users", m_db);
    if (checkUsers.next() && checkUsers.value(0).toInt() == 0) {
        populateDemoData();
    }
    if (!ensureDefaultAccounts()) {
        qCritical() << "Unable to enforce default workstation accounts.";
        return false;
    }

    emit databaseUpdated();
    return true;
}

void DatabaseManager::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// User Operations
bool DatabaseManager::registerUser(const User& user) {
    if (user.username.trimmed().isEmpty() || user.password.isEmpty() ||
        user.role.trimmed().isEmpty() || user.fullName.trimmed().isEmpty()) {
        return false;
    }

    const QByteArray salt = createPasswordSalt();
    const QByteArray digest = passwordDigest(user.password, salt);
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO users (username, password, password_hash, password_salt, role, full_name) "
                  "VALUES (:username, '', :hash, :salt, :role, :fullName)");
    query.bindValue(":username", user.username.trimmed());
    query.bindValue(":hash", QString::fromLatin1(digest.toBase64()));
    query.bindValue(":salt", QString::fromLatin1(salt.toBase64()));
    query.bindValue(":role", user.role.trimmed());
    query.bindValue(":fullName", user.fullName.trimmed());
    
    if (query.exec()) {
        addLog(user.username, "REGISTER_USER", "Registered user: " + user.fullName + " as " + user.role);
        emit databaseUpdated();
        return true;
    }
    return false;
}

User DatabaseManager::authenticateUser(const QString& username, const QString& password) {
    QSqlQuery query(m_db);
    query.prepare("SELECT username, password_hash, password_salt, role, full_name FROM users WHERE username = :username");
    query.bindValue(":username", username);
    
    User user;
    if (query.exec() && query.next()) {
        const QByteArray expected = QByteArray::fromBase64(query.value(1).toString().toLatin1());
        const QByteArray salt = QByteArray::fromBase64(query.value(2).toString().toLatin1());
        if (!expected.isEmpty() && passwordDigest(password, salt) == expected) {
            user.username = query.value(0).toString();
            user.role = query.value(3).toString();
            user.fullName = query.value(4).toString();
            addLog(username, "LOGIN", "Successful login as " + user.role);
            return user;
        }
    }
    addLog(username.isEmpty() ? "Unknown" : username, "LOGIN_FAILED", "Failed login attempt");
    return user;
}

QList<User> DatabaseManager::getUsers() {
    QList<User> list;
    QSqlQuery query("SELECT username, role, full_name FROM users", m_db);
    while (query.next()) {
        User u;
        u.username = query.value(0).toString();
        u.role = query.value(1).toString();
        u.fullName = query.value(2).toString();
        list.append(u);
    }
    return list;
}

// Patient Operations
bool DatabaseManager::addPatient(Patient& patient) {
    patient.fullName = patient.fullName.trimmed();
    patient.phone = patient.phone.trimmed();
    patient.village = patient.village.trimmed();
    if (patient.fullName.isEmpty() || patient.phone.isEmpty() || patient.village.isEmpty() ||
        patient.age < 0 || patient.age > 130) {
        return false;
    }
    if (checkDuplicate(patient.fullName, patient.age, patient.phone)) {
        qWarning() << "Patient registration rejected as duplicate:" << patient.fullName;
        return false;
    }
    if (!m_db.transaction()) {
        qWarning() << "Unable to begin patient registration transaction:" << m_db.lastError().text();
        return false;
    }

    // Generate patient ID
    QSqlQuery idQuery("SELECT MAX(id) FROM patients", m_db);
    int nextId = 1;
    if (idQuery.next()) {
        nextId = idQuery.value(0).toInt() + 1;
    }
    QString yearStr = QString::number(QDateTime::currentDateTime().date().year());
    patient.patientId = QString("AEGIS-%1-%2").arg(yearStr).arg(nextId, 4, 10, QChar('0'));
    patient.qrId = "AEGISQR_" + patient.patientId;
    patient.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO patients (patient_id, qr_id, full_name, age, gender, blood_group, phone, address, village, state, country, emergency_contact, medical_notes, doctor_assigned, operator_name, photo_path, created_at) "
                  "VALUES (:pid, :qrid, :name, :age, :gender, :bg, :phone, :addr, :village, :state, :country, :emerg, :notes, :doctor, :operator, :photo, :created)");
    query.bindValue(":pid", patient.patientId);
    query.bindValue(":qrid", patient.qrId);
    query.bindValue(":name", patient.fullName);
    query.bindValue(":age", patient.age);
    query.bindValue(":gender", databaseText(patient.gender));
    query.bindValue(":bg", databaseText(patient.bloodGroup));
    query.bindValue(":phone", patient.phone);
    query.bindValue(":addr", databaseText(patient.address));
    query.bindValue(":village", patient.village);
    query.bindValue(":state", databaseText(patient.state));
    query.bindValue(":country", databaseText(patient.country));
    query.bindValue(":emerg", databaseText(patient.emergencyContact));
    query.bindValue(":notes", databaseText(patient.medicalNotes));
    query.bindValue(":doctor", databaseText(patient.doctorAssigned));
    query.bindValue(":operator", databaseText(patient.operatorName));
    query.bindValue(":photo", databaseText(patient.photoPath));
    query.bindValue(":created", patient.createdAt);

    if (query.exec()) {
        patient.id = query.lastInsertId().toInt();
        addLog(patient.operatorName, "REGISTER_PATIENT", "Registered new patient: " + patient.fullName + " (ID: " + patient.patientId + ")");
        
        // Populate standard digital twin nodes for the patient as "Normal"
        QStringList organs = {"Head", "Brain", "Eyes", "Neck", "Chest", "Heart", "Lungs", "Abdomen", "Liver", "Kidney", "Hands", "Legs", "Feet"};
        QSqlQuery twinQuery(m_db);
        twinQuery.prepare("INSERT INTO twin_states (patient_id, region_name, status, notes, last_updated) "
                          "VALUES (:pid, :region, 'Normal', :notes, :updated)");
        for (const QString& organ : organs) {
            twinQuery.bindValue(":pid", patient.patientId);
            twinQuery.bindValue(":region", organ);
            twinQuery.bindValue(":notes", "No screening observations recorded.");
            twinQuery.bindValue(":updated", patient.createdAt);
            if (!twinQuery.exec()) {
                qWarning() << "Unable to create digital twin region:" << twinQuery.lastError().text();
                m_db.rollback();
                return false;
            }
        }

        if (!m_db.commit()) {
            qWarning() << "Unable to commit patient registration:" << m_db.lastError().text();
            m_db.rollback();
            return false;
        }
        emit databaseUpdated();
        return true;
    }
    qWarning() << "Unable to insert patient:" << query.lastError().text();
    m_db.rollback();
    return false;
}

bool DatabaseManager::updatePatient(const Patient& patient) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE patients SET full_name = :name, age = :age, gender = :gender, blood_group = :bg, phone = :phone, address = :addr, "
                  "village = :village, state = :state, country = :country, emergency_contact = :emerg, medical_notes = :notes, "
                  "doctor_assigned = :doctor, photo_path = :photo WHERE patient_id = :pid");
    query.bindValue(":name", patient.fullName);
    query.bindValue(":age", patient.age);
    query.bindValue(":gender", patient.gender);
    query.bindValue(":bg", patient.bloodGroup);
    query.bindValue(":phone", patient.phone);
    query.bindValue(":addr", patient.address);
    query.bindValue(":village", patient.village);
    query.bindValue(":state", patient.state);
    query.bindValue(":country", patient.country);
    query.bindValue(":emerg", patient.emergencyContact);
    query.bindValue(":notes", patient.medicalNotes);
    query.bindValue(":doctor", patient.doctorAssigned);
    query.bindValue(":photo", patient.photoPath);
    query.bindValue(":pid", patient.patientId);

    if (query.exec()) {
        addLog(patient.operatorName, "UPDATE_PATIENT", "Updated patient details: " + patient.fullName + " (ID: " + patient.patientId + ")");
        emit databaseUpdated();
        return true;
    }
    return false;
}

bool DatabaseManager::deletePatient(const QString& patientId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM patients WHERE patient_id = :pid");
    query.bindValue(":pid", patientId);
    if (query.exec()) {
        addLog("System", "DELETE_PATIENT", "Deleted patient record: " + patientId);
        emit databaseUpdated();
        return true;
    }
    return false;
}

Patient DatabaseManager::getPatient(const QString& patientId) {
    Patient p;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, patient_id, qr_id, full_name, age, gender, blood_group, phone, address, village, state, country, emergency_contact, medical_notes, doctor_assigned, operator_name, photo_path, created_at FROM patients WHERE patient_id = :pid");
    query.bindValue(":pid", patientId);

    if (query.exec() && query.next()) {
        p.id = query.value(0).toInt();
        p.patientId = query.value(1).toString();
        p.qrId = query.value(2).toString();
        p.fullName = query.value(3).toString();
        p.age = query.value(4).toInt();
        p.gender = query.value(5).toString();
        p.bloodGroup = query.value(6).toString();
        p.phone = query.value(7).toString();
        p.address = query.value(8).toString();
        p.village = query.value(9).toString();
        p.state = query.value(10).toString();
        p.country = query.value(11).toString();
        p.emergencyContact = query.value(12).toString();
        p.medicalNotes = query.value(13).toString();
        p.doctorAssigned = query.value(14).toString();
        p.operatorName = query.value(15).toString();
        p.photoPath = query.value(16).toString();
        p.createdAt = query.value(17).toString();
    }
    return p;
}

QList<Patient> DatabaseManager::getPatients(const QString& search, const QString& sortBy, bool desc) {
    QList<Patient> list;
    QString queryStr = "SELECT id, patient_id, qr_id, full_name, age, gender, blood_group, phone, address, village, state, country, emergency_contact, medical_notes, doctor_assigned, operator_name, photo_path, created_at FROM patients";
    
    if (!search.isEmpty()) {
        queryStr += " WHERE full_name LIKE :search OR patient_id LIKE :search OR village LIKE :search OR phone LIKE :search";
    }

    // Safety checks for SQL injection on sorting strings (internally controlled keys)
    QString validSortBy = "id";
    if (sortBy == "full_name" || sortBy == "patient_id" || sortBy == "age" || sortBy == "village" || sortBy == "created_at") {
        validSortBy = sortBy;
    }
    
    queryStr += QString(" ORDER BY %1 %2").arg(validSortBy).arg(desc ? "DESC" : "ASC");

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    if (!search.isEmpty()) query.bindValue(":search", "%" + search.trimmed() + "%");
    if (!query.exec()) return list;
    while (query.next()) {
        Patient p;
        p.id = query.value(0).toInt();
        p.patientId = query.value(1).toString();
        p.qrId = query.value(2).toString();
        p.fullName = query.value(3).toString();
        p.age = query.value(4).toInt();
        p.gender = query.value(5).toString();
        p.bloodGroup = query.value(6).toString();
        p.phone = query.value(7).toString();
        p.address = query.value(8).toString();
        p.village = query.value(9).toString();
        p.state = query.value(10).toString();
        p.country = query.value(11).toString();
        p.emergencyContact = query.value(12).toString();
        p.medicalNotes = query.value(13).toString();
        p.doctorAssigned = query.value(14).toString();
        p.operatorName = query.value(15).toString();
        p.photoPath = query.value(16).toString();
        p.createdAt = query.value(17).toString();
        list.append(p);
    }
    return list;
}

bool DatabaseManager::checkDuplicate(const QString& fullName, int age, const QString& phone) {
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM patients WHERE LOWER(full_name) = LOWER(:name) AND age = :age AND phone = :phone");
    query.bindValue(":name", fullName.trimmed());
    query.bindValue(":age", age);
    query.bindValue(":phone", phone.trimmed());
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

int DatabaseManager::getPatientCount() {
    QSqlQuery query("SELECT COUNT(*) FROM patients", m_db);
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// Visit Operations
bool DatabaseManager::addVisit(const Visit& visit) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO visits (patient_id, visit_date, heart_rate, sys_bp, dia_bp, spo2, temperature, screening_type, notes, operator_name) "
                  "VALUES (:pid, :date, :hr, :sys, :dia, :spo2, :temp, :type, :notes, :operator)");
    query.bindValue(":pid", visit.patientId);
    query.bindValue(":date", visit.visitDate);
    query.bindValue(":hr", visit.heartRate);
    query.bindValue(":sys", visit.sysBP);
    query.bindValue(":dia", visit.diaBP);
    query.bindValue(":spo2", visit.spo2);
    query.bindValue(":temp", visit.temperature);
    query.bindValue(":type", visit.screeningType);
    query.bindValue(":notes", visit.notes);
    query.bindValue(":operator", visit.operatorName);

    if (query.exec()) {
        addLog(visit.operatorName, "ADD_VISIT", QString("Added visit for Patient: %1, Screening: %2").arg(visit.patientId).arg(visit.screeningType));
        emit databaseUpdated();
        return true;
    }
    return false;
}

QList<Visit> DatabaseManager::getVisitsForPatient(const QString& patientId) {
    QList<Visit> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, patient_id, visit_date, heart_rate, sys_bp, dia_bp, spo2, temperature, screening_type, notes, operator_name FROM visits WHERE patient_id = :pid ORDER BY id DESC");
    query.bindValue(":pid", patientId);

    if (query.exec()) {
        while (query.next()) {
            Visit v;
            v.id = query.value(0).toInt();
            v.patientId = query.value(1).toString();
            v.visitDate = query.value(2).toString();
            v.heartRate = query.value(3).toInt();
            v.sysBP = query.value(4).toInt();
            v.diaBP = query.value(5).toInt();
            v.spo2 = query.value(6).toInt();
            v.temperature = query.value(7).toDouble();
            v.screeningType = query.value(8).toString();
            v.notes = query.value(9).toString();
            v.operatorName = query.value(10).toString();
            list.append(v);
        }
    }
    return list;
}

int DatabaseManager::getVisitCountToday() {
    QString today = QDate::currentDate().toString(Qt::ISODate);
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM visits WHERE visit_date LIKE :today");
    query.bindValue(":today", today + "%");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// Twin State Operations
bool DatabaseManager::updateTwinRegion(const TwinRegionState& state) {
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO twin_states (patient_id, region_name, status, notes, last_updated) "
                  "VALUES (:pid, :region, :status, :notes, :updated)");
    query.bindValue(":pid", state.patientId);
    query.bindValue(":region", state.regionName);
    query.bindValue(":status", state.status);
    query.bindValue(":notes", state.notes);
    query.bindValue(":updated", state.lastUpdated);

    if (query.exec()) {
        emit databaseUpdated();
        return true;
    }
    return false;
}

QList<TwinRegionState> DatabaseManager::getTwinStatesForPatient(const QString& patientId) {
    QList<TwinRegionState> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT patient_id, region_name, status, notes, last_updated FROM twin_states WHERE patient_id = :pid");
    query.bindValue(":pid", patientId);

    if (query.exec()) {
        while (query.next()) {
            TwinRegionState t;
            t.patientId = query.value(0).toString();
            t.regionName = query.value(1).toString();
            t.status = query.value(2).toString();
            t.notes = query.value(3).toString();
            t.lastUpdated = query.value(4).toString();
            list.append(t);
        }
    }
    return list;
}

TwinRegionState DatabaseManager::getTwinRegionState(const QString& patientId, const QString& regionName) {
    TwinRegionState t;
    t.patientId = patientId;
    t.regionName = regionName;
    t.status = "Normal";
    t.notes = "No reports associated.";
    
    QSqlQuery query(m_db);
    query.prepare("SELECT status, notes, last_updated FROM twin_states WHERE patient_id = :pid AND region_name = :region");
    query.bindValue(":pid", patientId);
    query.bindValue(":region", regionName);

    if (query.exec() && query.next()) {
        t.status = query.value(0).toString();
        t.notes = query.value(1).toString();
        t.lastUpdated = query.value(2).toString();
    }
    return t;
}

// Follow Up Operations
bool DatabaseManager::scheduleFollowUp(const FollowUp& followUp) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO follow_ups (patient_id, full_name, due_date, status, notes) "
                  "VALUES (:pid, :name, :due, :status, :notes)");
    query.bindValue(":pid", followUp.patientId);
    query.bindValue(":name", followUp.fullName);
    query.bindValue(":due", followUp.dueDate);
    query.bindValue(":status", followUp.status);
    query.bindValue(":notes", followUp.notes);

    if (query.exec()) {
        addLog("System", "SCHEDULE_FOLLOW_UP", QString("Scheduled follow up for %1 (Due: %2)").arg(followUp.fullName).arg(followUp.dueDate));
        emit databaseUpdated();
        return true;
    }
    return false;
}

bool DatabaseManager::updateFollowUpStatus(int id, const QString& status) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE follow_ups SET status = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", id);

    if (query.exec()) {
        addLog("System", "UPDATE_FOLLOW_UP", QString("Updated follow up ID: %1 to status: %2").arg(id).arg(status));
        emit databaseUpdated();
        return true;
    }
    return false;
}

QList<FollowUp> DatabaseManager::getPendingFollowUps() {
    QList<FollowUp> list;
    QSqlQuery query("SELECT id, patient_id, full_name, due_date, status, notes FROM follow_ups WHERE status = 'Pending' ORDER BY due_date ASC", m_db);
    while (query.next()) {
        FollowUp f;
        f.id = query.value(0).toInt();
        f.patientId = query.value(1).toString();
        f.fullName = query.value(2).toString();
        f.dueDate = query.value(3).toString();
        f.status = query.value(4).toString();
        f.notes = query.value(5).toString();
        list.append(f);
    }
    return list;
}

QList<FollowUp> DatabaseManager::getAllFollowUps() {
    QList<FollowUp> list;
    QSqlQuery query("SELECT id, patient_id, full_name, due_date, status, notes FROM follow_ups ORDER BY due_date DESC", m_db);
    while (query.next()) {
        FollowUp f;
        f.id = query.value(0).toInt();
        f.patientId = query.value(1).toString();
        f.fullName = query.value(2).toString();
        f.dueDate = query.value(3).toString();
        f.status = query.value(4).toString();
        f.notes = query.value(5).toString();
        list.append(f);
    }
    return list;
}

int DatabaseManager::getOverdueFollowUpCount() {
    QString today = QDate::currentDate().toString(Qt::ISODate);
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM follow_ups WHERE status = 'Pending' AND due_date < :today");
    query.bindValue(":today", today);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool DatabaseManager::addVoiceNote(VoiceNote& note) {
    if (note.patientId.trimmed().isEmpty() || note.audioFilePath.trimmed().isEmpty()) {
        return false;
    }
    if (note.timestamp.isEmpty()) note.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (note.operatorName.isEmpty()) note.operatorName = "Dr. Soham";

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO voice_notes (patient_id, timestamp, operator_name, audio_file_path, transcript, recording_length_sec, visit_number) "
                  "VALUES (:pid, :time, :operator, :audio, :transcript, :seconds, :visit)");
    query.bindValue(":pid", note.patientId);
    query.bindValue(":time", note.timestamp);
    query.bindValue(":operator", note.operatorName);
    query.bindValue(":audio", note.audioFilePath);
    query.bindValue(":transcript", note.transcript);
    query.bindValue(":seconds", note.recordingLengthSec);
    query.bindValue(":visit", note.visitNumber);
    if (!query.exec()) {
        qWarning() << "Unable to save voice note:" << query.lastError().text();
        return false;
    }
    note.id = query.lastInsertId().toInt();
    addLog(note.operatorName, "VOICE_NOTE_SAVED",
           QString("Clinical observation saved for %1. Audio and transcript retained.").arg(note.patientId));
    emit databaseUpdated();
    return true;
}

QList<VoiceNote> DatabaseManager::getVoiceNotesForPatient(const QString& patientId) {
    QList<VoiceNote> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, patient_id, timestamp, operator_name, audio_file_path, transcript, recording_length_sec, visit_number "
                  "FROM voice_notes WHERE patient_id = :pid ORDER BY timestamp DESC, id DESC");
    query.bindValue(":pid", patientId);
    if (query.exec()) {
        while (query.next()) {
            VoiceNote note;
            note.id = query.value(0).toInt();
            note.patientId = query.value(1).toString();
            note.timestamp = query.value(2).toString();
            note.operatorName = query.value(3).toString();
            note.audioFilePath = query.value(4).toString();
            note.transcript = query.value(5).toString();
            note.recordingLengthSec = query.value(6).toInt();
            note.visitNumber = query.value(7).toInt();
            list.append(note);
        }
    }
    return list;
}

bool DatabaseManager::addPatientReport(PatientReport& report) {
    if (report.patientId.trimmed().isEmpty() || report.reportId.trimmed().isEmpty()) {
        return false;
    }
    if (report.generatedAt.isEmpty()) report.generatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO patient_reports (patient_id, report_id, github_url, local_path, generated_at, visit_number, qr_generated) "
                  "VALUES (:pid, :rid, :url, :path, :time, :visit, :qr)");
    query.bindValue(":pid", report.patientId);
    query.bindValue(":rid", report.reportId);
    query.bindValue(":url", report.githubUrl);
    query.bindValue(":path", report.localPath);
    query.bindValue(":time", report.generatedAt);
    query.bindValue(":visit", report.visitNumber);
    query.bindValue(":qr", report.qrGenerated ? 1 : 0);
    if (!query.exec()) {
        qWarning() << "Unable to save patient report:" << query.lastError().text();
        return false;
    }
    report.id = query.lastInsertId().toInt();
    addLog("Dr. Soham", "REPORT_GENERATED",
           QString("Educational demonstration report %1 generated for %2.").arg(report.reportId, report.patientId));
    emit databaseUpdated();
    return true;
}

QList<PatientReport> DatabaseManager::getReportsForPatient(const QString& patientId) {
    QList<PatientReport> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, patient_id, report_id, github_url, local_path, generated_at, visit_number, qr_generated "
                  "FROM patient_reports WHERE patient_id = :pid ORDER BY generated_at DESC, id DESC");
    query.bindValue(":pid", patientId);
    if (query.exec()) {
        while (query.next()) {
            PatientReport report;
            report.id = query.value(0).toInt();
            report.patientId = query.value(1).toString();
            report.reportId = query.value(2).toString();
            report.githubUrl = query.value(3).toString();
            report.localPath = query.value(4).toString();
            report.generatedAt = query.value(5).toString();
            report.visitNumber = query.value(6).toInt();
            report.qrGenerated = query.value(7).toInt() != 0;
            list.append(report);
        }
    }
    return list;
}

bool DatabaseManager::addEmergencyEvent(EmergencyEvent& event) {
    if (event.patientId.trimmed().isEmpty()) {
        return false;
    }
    if (event.timestamp.isEmpty()) event.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (event.operatorName.isEmpty()) event.operatorName = "Dr. Soham";
    if (event.priority.isEmpty()) event.priority = "Critical";

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO emergency_events (patient_id, timestamp, operator_name, priority, details) "
                  "VALUES (:pid, :time, :operator, :priority, :details)");
    query.bindValue(":pid", event.patientId);
    query.bindValue(":time", event.timestamp);
    query.bindValue(":operator", event.operatorName);
    query.bindValue(":priority", event.priority);
    query.bindValue(":details", event.details);
    if (!query.exec()) {
        qWarning() << "Unable to save emergency event:" << query.lastError().text();
        return false;
    }
    event.id = query.lastInsertId().toInt();
    addLog(event.operatorName, "EMERGENCY_ACTIVATED",
           QString("Emergency mode activated for %1 with priority %2.").arg(event.patientId, event.priority));
    emit databaseUpdated();
    return true;
}

QList<EmergencyEvent> DatabaseManager::getEmergencyEventsForPatient(const QString& patientId) {
    QList<EmergencyEvent> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, patient_id, timestamp, operator_name, priority, details "
                  "FROM emergency_events WHERE patient_id = :pid ORDER BY timestamp DESC, id DESC");
    query.bindValue(":pid", patientId);
    if (query.exec()) {
        while (query.next()) {
            EmergencyEvent event;
            event.id = query.value(0).toInt();
            event.patientId = query.value(1).toString();
            event.timestamp = query.value(2).toString();
            event.operatorName = query.value(3).toString();
            event.priority = query.value(4).toString();
            event.details = query.value(5).toString();
            list.append(event);
        }
    }
    return list;
}

// Stats & Analytics Queries
QMap<QString, int> DatabaseManager::getPatientsPerVillageStats() {
    QMap<QString, int> stats;
    QSqlQuery query("SELECT village, COUNT(*) FROM patients GROUP BY village", m_db);
    while (query.next()) {
        stats.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return stats;
}

QMap<QString, int> DatabaseManager::getVisitsPerDayStats() {
    QMap<QString, int> stats;
    // Get last 7 days of activity
    QSqlQuery query("SELECT SUBSTR(visit_date, 1, 10) as day, COUNT(*) FROM visits GROUP BY day ORDER BY day DESC LIMIT 7", m_db);
    while (query.next()) {
        stats.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return stats;
}

QMap<QString, int> DatabaseManager::getScreeningTypeStats() {
    QMap<QString, int> stats;
    QSqlQuery query("SELECT screening_type, COUNT(*) FROM visits GROUP BY screening_type", m_db);
    while (query.next()) {
        stats.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return stats;
}

QMap<QString, int> DatabaseManager::getAgeDistributionStats() {
    QMap<QString, int> stats;
    stats["0-18"] = 0;
    stats["19-35"] = 0;
    stats["36-50"] = 0;
    stats["51-65"] = 0;
    stats["65+"] = 0;

    QSqlQuery query("SELECT age FROM patients", m_db);
    while (query.next()) {
        int age = query.value(0).toInt();
        if (age <= 18) stats["0-18"]++;
        else if (age <= 35) stats["19-35"]++;
        else if (age <= 50) stats["36-50"]++;
        else if (age <= 65) stats["51-65"]++;
        else stats["65+"]++;
    }
    return stats;
}

QMap<QString, int> DatabaseManager::getGenderDistributionStats() {
    QMap<QString, int> stats;
    QSqlQuery query("SELECT gender, COUNT(*) FROM patients GROUP BY gender", m_db);
    while (query.next()) {
        stats.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return stats;
}

// Audit Logging
void DatabaseManager::addLog(const QString& username, const QString& action, const QString& details) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO system_logs (timestamp, username, action, details) VALUES (:time, :user, :action, :details)");
    query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":user", username.isEmpty() ? "System" : username);
    query.bindValue(":action", action);
    query.bindValue(":details", details);
    query.exec();
}

QList<QMap<QString, QString>> DatabaseManager::getRecentLogs(int limit) {
    QList<QMap<QString, QString>> logs;
    QSqlQuery query(m_db);
    query.prepare("SELECT timestamp, username, action, details FROM system_logs ORDER BY id DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    
    if (query.exec()) {
        while (query.next()) {
            QMap<QString, QString> log;
            log["timestamp"] = query.value(0).toString();
            log["username"] = query.value(1).toString();
            log["action"] = query.value(2).toString();
            log["details"] = query.value(3).toString();
            logs.append(log);
        }
    }
    return logs;
}

// Backup & Restore
bool DatabaseManager::backupDatabase(const QString& backupPath) {
    if (backupPath.trimmed().isEmpty() || QFileInfo(backupPath) == QFileInfo(m_dbPath)) {
        return false;
    }

    QSqlQuery checkpoint(m_db);
    checkpoint.exec("PRAGMA wal_checkpoint(FULL)");
    m_db.close();
    const bool success = copyFileAtomically(m_dbPath, backupPath);
    
    // Reopen database
    if (!m_db.open()) return false;
    QSqlQuery(m_db).exec("PRAGMA foreign_keys = ON");
    QSqlQuery(m_db).exec("PRAGMA journal_mode = WAL");
    QSqlQuery(m_db).exec("PRAGMA busy_timeout = 5000");
    if (success) {
        addLog("System", "DATABASE_BACKUP", "Successful database backup to: " + backupPath);
    }
    return success;
}

bool DatabaseManager::restoreDatabase(const QString& backupPath) {
    if (!QFile::exists(backupPath) || QFileInfo(backupPath) == QFileInfo(m_dbPath)) return false;

    const QString validationConnection = QString("aegiscare_restore_validation_%1")
                                             .arg(QRandomGenerator::global()->generate64());
    bool validBackup = false;
    {
        QSqlDatabase validationDb = QSqlDatabase::addDatabase("QSQLITE", validationConnection);
        validationDb.setConnectOptions("QSQLITE_OPEN_READONLY");
        validationDb.setDatabaseName(backupPath);
        if (validationDb.open()) {
            QSqlQuery integrity(validationDb);
            validBackup = integrity.exec("PRAGMA integrity_check") && integrity.next() &&
                          integrity.value(0).toString().compare("ok", Qt::CaseInsensitive) == 0;
        }
        validationDb.close();
    }
    QSqlDatabase::removeDatabase(validationConnection);
    if (!validBackup) return false;

    m_db.close();
    const bool success = copyFileAtomically(backupPath, m_dbPath);
    
    // Reopen
    if (!m_db.open()) return false;
    QSqlQuery(m_db).exec("PRAGMA foreign_keys = ON");
    QSqlQuery(m_db).exec("PRAGMA journal_mode = WAL");
    QSqlQuery(m_db).exec("PRAGMA busy_timeout = 5000");
    if (success) {
        addLog("System", "DATABASE_RESTORE", "Successful database restoration from: " + backupPath);
        emit databaseUpdated();
    }
    return success;
}

void DatabaseManager::populateDemoData() {
    // Register default users
    User admin = {"admin", "admin", "Administrator", "Administrator"};
    User doctor = {"soham", "soham", "Doctor", "Dr. Soham"};
    registerUser(admin);
    registerUser(doctor);

    // Register patients
    struct DemoPatient {
        QString name; int age; QString gender; QString blood; QString phone; QString village; QString notes;
    };
    
    QList<DemoPatient> demoPatients = {
        {"John Matthews", 45, "Male", "O+", "+91 98765 43210", "Green Valley, Pune", "Routine demonstration patient. Previous reports and follow-up workflow available."},
        {"Priya Sharma", 32, "Female", "B+", "+91 99887 76655", "Lake View Society, Mumbai", "Demo patient for guided screening, voice notes, and preventive care workflow."},
        {"Rahul Verma", 58, "Male", "A+", "+91 88776 65544", "Sunrise Colony, Nashik", "Demo patient with follow-up and emergency workflow examples."}
    };

    int index = 1;
    for (const auto& dp : demoPatients) {
        Patient p;
        p.fullName = dp.name;
        p.age = dp.age;
        p.gender = dp.gender;
        p.bloodGroup = dp.blood;
        p.phone = dp.phone;
        p.address = dp.village;
        p.village = dp.village;
        p.state = "Maharashtra";
        p.country = "India";
        p.emergencyContact = "+91 90000 12345";
        p.medicalNotes = dp.notes;
        p.doctorAssigned = "Dr. Soham";
        p.operatorName = "Dr. Soham";
        p.photoPath = ""; // Emulated profile picture
        
        addPatient(p);
        
        // Add some visits
        Visit v;
        v.patientId = p.patientId;
        v.visitDate = QDateTime::currentDateTime().addDays(-index * 3).toString(Qt::ISODate);
        v.heartRate = QRandomGenerator::global()->bounded(67, 82);
        v.sysBP = QRandomGenerator::global()->bounded(110, 130);
        v.diaBP = QRandomGenerator::global()->bounded(71, 83);
        v.spo2 = QRandomGenerator::global()->bounded(96, 100);
        v.temperature = 97.8 + QRandomGenerator::global()->bounded(20) / 10.0;
        v.operatorName = "Dr. Soham";
        
        // Random screening types
        QStringList types = {"Routine Health Screening", "Demo Skin Screening", "Annual Check-up"};
        v.screeningType = types[(index - 1) % types.size()];
        v.notes = "Educational demonstration visit. Report generated, QR workflow available, voice notes available. No diagnosis performed.";
        
        addVisit(v);

        for (int visitOffset = 2; visitOffset <= 3; ++visitOffset) {
            Visit previous = v;
            previous.visitDate = QDateTime::currentDateTime().addDays(-(index * 3 + visitOffset * 7)).toString(Qt::ISODate);
            previous.screeningType = visitOffset == 2 ? "Follow-up Visit" : "Annual Check-up";
            previous.notes = visitOffset == 2
                                 ? "Demo Skin Screening. QR generated. Voice notes available. Educational prototype only."
                                 : "Healthy demonstration workflow. No diagnosis performed.";
            addVisit(previous);
        }

        // Add follow-ups
        FollowUp f;
        f.patientId = p.patientId;
        f.fullName = p.fullName;
        f.dueDate = QDate::currentDate().addDays(QRandomGenerator::global()->bounded(-5, 10)).toString(Qt::ISODate);
        f.status = (f.dueDate < QDate::currentDate().toString(Qt::ISODate))
                       ? (QRandomGenerator::global()->bounded(2) ? "Missed" : "Completed")
                       : "Pending";
        f.notes = "Lifestyle advice, next visit planning, medication reminder, and observation summary for demonstration.";
        scheduleFollowUp(f);
        
        // Let's set some digital twin region flags to show high-fidelity risk status
        if (index == 1) {
            TwinRegionState heartState = {p.patientId, "Heart", "Warning", "Screening signal was irregular; clinician review requested. This is not a diagnosis.", QDateTime::currentDateTime().toString(Qt::ISODate)};
            updateTwinRegion(heartState);
        } else if (index == 4) {
            TwinRegionState eyesState = {p.patientId, "Eyes", "Review", "Patient reported visual blur; visual acuity follow-up requested. This is not a diagnosis.", QDateTime::currentDateTime().toString(Qt::ISODate)};
            updateTwinRegion(eyesState);
        }
        
        index++;
    }
}

bool DatabaseManager::migrateSchema() {
    QSet<QString> userColumns;
    QSqlQuery columns(m_db);
    if (!columns.exec("PRAGMA table_info(users)")) return false;
    while (columns.next()) userColumns.insert(columns.value(1).toString());

    QSqlQuery migration(m_db);
    if (!userColumns.contains("password_hash") &&
        !migration.exec("ALTER TABLE users ADD COLUMN password_hash TEXT")) {
        return false;
    }
    if (!userColumns.contains("password_salt") &&
        !migration.exec("ALTER TABLE users ADD COLUMN password_salt TEXT")) {
        return false;
    }

    QList<QPair<QString, QString>> legacyUsers;
    QSqlQuery legacy(m_db);
    if (!legacy.exec("SELECT username, password FROM users WHERE password <> '' AND (password_hash IS NULL OR password_hash = '')")) {
        return false;
    }
    while (legacy.next()) {
        legacyUsers.append({legacy.value(0).toString(), legacy.value(1).toString()});
    }

    for (const auto& [username, password] : legacyUsers) {
        const QByteArray salt = createPasswordSalt();
        QSqlQuery update(m_db);
        update.prepare("UPDATE users SET password = '', password_hash = :hash, password_salt = :salt WHERE username = :username");
        update.bindValue(":hash", QString::fromLatin1(passwordDigest(password, salt).toBase64()));
        update.bindValue(":salt", QString::fromLatin1(salt.toBase64()));
        update.bindValue(":username", username);
        if (!update.exec()) return false;
    }
    return true;
}

bool DatabaseManager::ensureDefaultAccounts() {
    QSqlQuery removeOthers(m_db);
    if (!removeOthers.exec("DELETE FROM users WHERE username NOT IN ('soham', 'admin')")) {
        qWarning() << "Unable to remove obsolete workstation accounts:" << removeOthers.lastError().text();
        return false;
    }
    User doctor = {"soham", "soham", "Doctor", "Dr. Soham"};
    User admin = {"admin", "admin", "Administrator", "Administrator"};
    return registerUser(doctor) && registerUser(admin);
}
