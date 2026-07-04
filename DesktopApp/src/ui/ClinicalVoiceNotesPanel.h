#ifndef CLINICALVOICENOTESPANEL_H
#define CLINICALVOICENOTESPANEL_H

#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

#include "../models/Models.h"

class ClinicalVoiceNotesPanel : public QDialog {
    Q_OBJECT
public:
    explicit ClinicalVoiceNotesPanel(QWidget* parent = nullptr);

    bool isRecording() const { return m_recording; }
    void startRecording(const Patient& patient, const QString& operatorName = QStringLiteral("Dr. Soham"));
    void stopRecordingAndSave();
    void cancelRecording();

signals:
    void noteSaved(const VoiceNote& note);
    void recordingStarted();
    void recordingStopped();

private slots:
    void updateRecordingUi();

private:
    QLabel* m_patientLabel = nullptr;
    QLabel* m_operatorLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_timerLabel = nullptr;
    QProgressBar* m_levelMeter = nullptr;
    QTextEdit* m_transcriptEdit = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_continueButton = nullptr;

    QTimer* m_uiTimer = nullptr;
    QTimer* m_draftTimer = nullptr;
    QElapsedTimer m_elapsed;
    Patient m_patient;
    QString m_operatorName = QStringLiteral("Dr. Soham");
    bool m_recording = false;
    bool m_paused = false;
    bool m_savedCurrentRecording = false;
    int m_lastTranscriptPhrase = -1;

    void setupUi();
    QString voiceNotesDirectory() const;
    QString timestampFileBase() const;
    QString createDemoAudioFile(int seconds) const;
    void saveDraft() const;
    void appendDemoTranscriptLine(int elapsedSeconds);
};

#endif // CLINICALVOICENOTESPANEL_H
