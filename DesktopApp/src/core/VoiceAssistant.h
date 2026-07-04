#ifndef VOICEASSISTANT_H
#define VOICEASSISTANT_H

#include <QObject>
#include <QMap>
#include <QLocale>
#include <QProcess>

#ifdef AEGISCARE_HAS_QT_SPEECH
#include <QTextToSpeech>
#endif

class VoiceAssistant : public QObject {
    Q_OBJECT
public:
    static VoiceAssistant& instance();

    enum Language {
        English,
        Hindi,
        Marathi
    };

    void init();
    void setLanguage(Language lang);
    Language currentLanguage() const;
    void setVolume(double volume); // 0.0 to 1.0
    double volume() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;

    void speak(const QString& phraseKey);
    void speakTextDirectly(const QString& text);
    QString getPhrase(const QString& phraseKey, Language lang) const;

signals:
    void spoken(const QString& text);
    void languageChanged(Language lang);

private:
    VoiceAssistant(QObject* parent = nullptr);
    ~VoiceAssistant();
    VoiceAssistant(const VoiceAssistant&) = delete;
    VoiceAssistant& operator=(const VoiceAssistant&) = delete;

#ifdef AEGISCARE_HAS_QT_SPEECH
    QTextToSpeech* m_speech = nullptr;
#endif
    Language m_currentLang = English;
    double m_volume = 1.0;
    bool m_enabled = true;
    bool m_initialized = false;
    QProcess* m_fallbackProcess = nullptr;

    QMap<QString, QMap<Language, QString>> m_phraseBook;
    void initPhraseBook();
};

#endif // VOICEASSISTANT_H
