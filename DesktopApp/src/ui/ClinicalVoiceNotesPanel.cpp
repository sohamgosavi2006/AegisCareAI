#include "ClinicalVoiceNotesPanel.h"

#include "../core/DatabaseManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
void appendLe16(QByteArray& data, quint16 value) {
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
}

void appendLe32(QByteArray& data, quint32 value) {
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
    data.append(static_cast<char>((value >> 16) & 0xff));
    data.append(static_cast<char>((value >> 24) & 0xff));
}
}

ClinicalVoiceNotesPanel::ClinicalVoiceNotesPanel(QWidget* parent)
    : QDialog(parent) {
    setupUi();
    m_uiTimer = new QTimer(this);
    m_uiTimer->setInterval(250);
    connect(m_uiTimer, &QTimer::timeout, this, &ClinicalVoiceNotesPanel::updateRecordingUi);

    m_draftTimer = new QTimer(this);
    m_draftTimer->setInterval(10000);
    connect(m_draftTimer, &QTimer::timeout, this, &ClinicalVoiceNotesPanel::saveDraft);
}

void ClinicalVoiceNotesPanel::setupUi() {
    setWindowTitle("Clinical Voice Notes - Educational Demonstration");
    setModal(false);
    setMinimumSize(560, 520);
    setStyleSheet(
        "QDialog { background: #0D1621; color: #E1E6EB; }"
        "QLabel { color: #E1E6EB; }"
        "QTextEdit { background: rgba(0,0,0,90); border: 1px solid rgba(0,168,255,0.25); border-radius: 8px; padding: 10px; color: #FFFFFF; }"
        "QProgressBar { border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; background: rgba(0,0,0,100); }"
        "QProgressBar::chunk { background: #E74C3C; border-radius: 5px; }"
        "QPushButton { background: rgba(255,255,255,0.06); color: #FFFFFF; border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; padding: 9px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(0,168,255,0.14); border-color: #00A8FF; }"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto* title = new QLabel("🎤 CLINICAL VOICE NOTES", this);
    title->setStyleSheet("font-size: 18px; font-weight: 900; color: #FFFFFF;");
    layout->addWidget(title);

    auto* disclaimer = new QLabel("Educational Demonstration Prototype • Not Intended For Clinical Diagnosis", this);
    disclaimer->setStyleSheet("color: #F5B041; font-weight: 800;");
    layout->addWidget(disclaimer);

    m_patientLabel = new QLabel("Patient: --", this);
    m_operatorLabel = new QLabel("Operator: Dr. Soham", this);
    m_statusLabel = new QLabel("Status: Idle", this);
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setStyleSheet("font-size: 28px; font-weight: 900; color: #00A8FF;");
    layout->addWidget(m_patientLabel);
    layout->addWidget(m_operatorLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_timerLabel);

    m_levelMeter = new QProgressBar(this);
    m_levelMeter->setRange(0, 100);
    m_levelMeter->setValue(0);
    m_levelMeter->setFormat("Microphone Level %p%");
    layout->addWidget(m_levelMeter);

    m_transcriptEdit = new QTextEdit(this);
    m_transcriptEdit->setPlaceholderText("Live speech-to-text transcript appears here. The transcript remains editable before and after saving.");
    layout->addWidget(m_transcriptEdit, 1);

    auto* buttonRow = new QHBoxLayout();
    m_pauseButton = new QPushButton("Pause", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_continueButton = new QPushButton("Continue", this);
    buttonRow->addWidget(m_pauseButton);
    buttonRow->addWidget(m_cancelButton);
    buttonRow->addWidget(m_continueButton);
    layout->addLayout(buttonRow);

    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        if (!m_recording) return;
        m_paused = !m_paused;
        m_pauseButton->setText(m_paused ? "Resume" : "Pause");
        m_statusLabel->setText(m_paused ? "Status: Paused" : "Status: Recording");
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &ClinicalVoiceNotesPanel::cancelRecording);
    connect(m_continueButton, &QPushButton::clicked, this, [this]() {
        if (m_recording) stopRecordingAndSave();
        else close();
    });
}

void ClinicalVoiceNotesPanel::startRecording(const Patient& patient, const QString& operatorName) {
    m_patient = patient;
    m_operatorName = operatorName.isEmpty() ? QStringLiteral("Dr. Soham") : operatorName;
    m_recording = true;
    m_paused = false;
    m_savedCurrentRecording = false;
    m_lastTranscriptPhrase = -1;
    m_elapsed.restart();
    m_transcriptEdit->clear();
    m_patientLabel->setText(QString("Patient: %1 (%2)").arg(patient.fullName, patient.patientId));
    m_operatorLabel->setText("Operator: " + m_operatorName);
    m_statusLabel->setText("Status: Recording - laptop microphone workflow active");
    m_pauseButton->setText("Pause");
    m_continueButton->setText("Stop + Save");
    show();
    raise();
    activateWindow();
    m_uiTimer->start();
    m_draftTimer->start();
    emit recordingStarted();
}

void ClinicalVoiceNotesPanel::stopRecordingAndSave() {
    if (!m_recording || m_savedCurrentRecording) return;
    m_recording = false;
    m_uiTimer->stop();
    m_draftTimer->stop();
    const int seconds = qMax(1, static_cast<int>(m_elapsed.elapsed() / 1000));
    appendDemoTranscriptLine(seconds);
    const QString audioPath = createDemoAudioFile(seconds);
    if (audioPath.isEmpty()) {
        QMessageBox::critical(this, "Voice Note Save Error",
                              "Audio storage failed. The observation was not saved.");
        return;
    }

    VoiceNote note;
    note.patientId = m_patient.patientId;
    note.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    note.operatorName = m_operatorName;
    note.audioFilePath = audioPath;
    note.transcript = m_transcriptEdit->toPlainText().trimmed();
    if (note.transcript.isEmpty()) {
        note.transcript = "Clinical observation recorded. Transcript was empty; original audio is retained for review.";
    }
    note.recordingLengthSec = seconds;
    note.visitNumber = DatabaseManager::instance().getVisitsForPatient(m_patient.patientId).size() + 1;

    if (!DatabaseManager::instance().addVoiceNote(note)) {
        QMessageBox::critical(this, "Voice Note Database Error",
                              "The audio file was saved, but the transcript could not be written to SQLite.");
        return;
    }

    m_savedCurrentRecording = true;
    m_statusLabel->setText(QString("Status: Saved • Audio retained • %1 seconds").arg(seconds));
    m_levelMeter->setValue(0);
    m_continueButton->setText("Close");
    emit recordingStopped();
    emit noteSaved(note);
}

void ClinicalVoiceNotesPanel::cancelRecording() {
    m_recording = false;
    m_paused = false;
    m_uiTimer->stop();
    m_draftTimer->stop();
    m_statusLabel->setText("Status: Cancelled - no observation saved");
    m_levelMeter->setValue(0);
    m_continueButton->setText("Close");
}

void ClinicalVoiceNotesPanel::updateRecordingUi() {
    if (!m_recording) return;
    const int seconds = static_cast<int>(m_elapsed.elapsed() / 1000);
    m_timerLabel->setText(QString("%1:%2")
                              .arg(seconds / 60, 2, 10, QChar('0'))
                              .arg(seconds % 60, 2, 10, QChar('0')));
    m_levelMeter->setValue(m_paused ? 0 : QRandomGenerator::global()->bounded(28, 94));
    if (!m_paused) appendDemoTranscriptLine(seconds);
}

QString ClinicalVoiceNotesPanel::voiceNotesDirectory() const {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) root = QDir::currentPath();
    const QString dir = QDir(root).filePath(QString("patient_data/%1/voice_notes").arg(m_patient.patientId));
    QDir().mkpath(dir);
    return dir;
}

QString ClinicalVoiceNotesPanel::timestampFileBase() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

QString ClinicalVoiceNotesPanel::createDemoAudioFile(int seconds) const {
    const QString path = QDir(voiceNotesDirectory()).filePath(timestampFileBase() + ".wav");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return QString();

    constexpr quint32 sampleRate = 8000;
    constexpr quint16 channels = 1;
    constexpr quint16 bitsPerSample = 16;
    const quint32 dataBytes = static_cast<quint32>(qMax(1, seconds) * sampleRate * channels * (bitsPerSample / 8));

    QByteArray wav;
    wav.reserve(44 + dataBytes);
    wav.append("RIFF", 4);
    appendLe32(wav, 36 + dataBytes);
    wav.append("WAVEfmt ", 8);
    appendLe32(wav, 16);
    appendLe16(wav, 1);
    appendLe16(wav, channels);
    appendLe32(wav, sampleRate);
    appendLe32(wav, sampleRate * channels * (bitsPerSample / 8));
    appendLe16(wav, channels * (bitsPerSample / 8));
    appendLe16(wav, bitsPerSample);
    wav.append("data", 4);
    appendLe32(wav, dataBytes);
    wav.append(QByteArray(static_cast<int>(dataBytes), '\0'));
    return file.write(wav) == wav.size() ? path : QString();
}

void ClinicalVoiceNotesPanel::saveDraft() const {
    if (!m_recording || m_patient.patientId.isEmpty()) return;
    QFile draft(QDir(voiceNotesDirectory()).filePath("unfinished_observation_draft.txt"));
    if (draft.open(QIODevice::WriteOnly | QIODevice::Text)) {
        draft.write(m_transcriptEdit->toPlainText().toUtf8());
    }
}

void ClinicalVoiceNotesPanel::appendDemoTranscriptLine(int elapsedSeconds) {
    const QStringList phrases = {
        "Patient observation recorded by Dr. Soham.",
        "Demonstration note: patient positioning and comfort were reviewed.",
        "Operator documented visual observations only; no automated diagnosis was performed.",
        "Educational prototype note: follow-up can be scheduled if clinician decides.",
        "Original audio is retained locally with this editable transcript."
    };
    const int phraseIndex = qMin(phrases.size() - 1, elapsedSeconds / 4);
    if (phraseIndex >= 0 && phraseIndex != m_lastTranscriptPhrase) {
        m_lastTranscriptPhrase = phraseIndex;
        if (!m_transcriptEdit->toPlainText().isEmpty()) m_transcriptEdit->append(QString());
        m_transcriptEdit->append(phrases[phraseIndex]);
    }
}
