#include "VoiceAssistant.h"
#include <QDebug>
#include <QProcess>

VoiceAssistant& VoiceAssistant::instance() {
    static VoiceAssistant inst;
    return inst;
}

VoiceAssistant::VoiceAssistant(QObject* parent) : QObject(parent) {
    m_fallbackProcess = new QProcess(this);
    initPhraseBook();
}

VoiceAssistant::~VoiceAssistant() {
#ifdef AEGISCARE_HAS_QT_SPEECH
    if (m_speech) {
        delete m_speech;
    }
#endif
}

void VoiceAssistant::init() {
    if (m_initialized) return;
    m_initialized = true;
    qDebug() << "Initializing VoiceAssistant...";
#ifdef AEGISCARE_HAS_QT_SPEECH
    try {
        m_speech = new QTextToSpeech(this);
        setLanguage(m_currentLang);
        setVolume(m_volume);
    } catch (...) {
        qWarning() << "QTextToSpeech failed to initialize. Relying on macOS CLI fallback.";
        m_speech = nullptr;
    }
#else
    qInfo() << "Qt speech backend unavailable. Using the native voice service.";
#endif
}

void VoiceAssistant::setLanguage(Language lang) {
    m_currentLang = lang;
#ifdef AEGISCARE_HAS_QT_SPEECH
    if (m_speech) {
        QLocale locale;
        switch (lang) {
            case English:
                locale = QLocale(QLocale::English, QLocale::UnitedStates);
                break;
            case Hindi:
                locale = QLocale(QLocale::Hindi, QLocale::India);
                break;
            case Marathi:
                locale = QLocale(QLocale::Marathi, QLocale::India);
                break;
        }
        m_speech->setLocale(locale);
    }
#endif
    emit languageChanged(lang);
}

VoiceAssistant::Language VoiceAssistant::currentLanguage() const {
    return m_currentLang;
}

void VoiceAssistant::setVolume(double volume) {
    m_volume = qBound(0.0, volume, 1.0);
#ifdef AEGISCARE_HAS_QT_SPEECH
    if (m_speech) {
        m_speech->setVolume(m_volume);
    }
#endif
}

double VoiceAssistant::volume() const {
    return m_volume;
}

void VoiceAssistant::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool VoiceAssistant::isEnabled() const {
    return m_enabled;
}

void VoiceAssistant::speak(const QString& phraseKey) {
    if (!m_enabled) return;

    QString text = getPhrase(phraseKey, m_currentLang);
    if (text.isEmpty()) {
        text = phraseKey;
    }
    speakTextDirectly(text);
}

void VoiceAssistant::speakTextDirectly(const QString& text) {
    if (!m_enabled || text.isEmpty()) return;

    qDebug() << "[Voice Assistant]" << text;
    emit spoken(text);

#ifdef AEGISCARE_HAS_QT_SPEECH
    if (m_speech && m_speech->state() != QTextToSpeech::BackendError) {
        m_speech->stop();
        m_speech->say(text);
        return;
    }
#endif
    {
        // Fallback for macOS standard 'say' utility
        // This is safe since we invoke a system command synchronously or in a non-blocking process
        QStringList args;
        
        // Select voice dynamically based on language
        if (m_currentLang == Hindi) {
            args << "-v" << "Lekha" << text; // Lekha is Apple's Hindi voice
        } else if (m_currentLang == Marathi) {
            args << "-v" << "Lekha" << text; // Falls back to Lekha or English if Marathi isn't there
        } else {
            args << text;
        }
        
        if (m_fallbackProcess->state() != QProcess::NotRunning) {
            m_fallbackProcess->kill();
        }
        m_fallbackProcess->start("say", args);
    }
}

QString VoiceAssistant::getPhrase(const QString& phraseKey, Language lang) const {
    if (m_phraseBook.contains(phraseKey) && m_phraseBook[phraseKey].contains(lang)) {
        return m_phraseBook[phraseKey][lang];
    }
    return QString();
}

void VoiceAssistant::initPhraseBook() {
    // Welcome
    m_phraseBook["Welcome"][English] = "Welcome to AegisCare AI.";
    m_phraseBook["Welcome"][Hindi] = "इजीसकेयर मेडिकल वर्कस्टेशन में आपका स्वागत है।";
    m_phraseBook["Welcome"][Marathi] = "इजीसकेअर मेडिकल वर्कस्टेशनमध्ये आपले स्वागत आहे.";

    // RegisterPatient
    m_phraseBook["RegisterPatient"][English] = "Please register the patient or scan the patient QR code.";
    m_phraseBook["RegisterPatient"][Hindi] = "कृपया मरीज का पंजीकरण करें या क्यूआर कोड स्कैन करें।";
    m_phraseBook["RegisterPatient"][Marathi] = "कृपया रुग्णाची नोंदणी करा किंवा क्यूआर कोड स्कॅन करा.";

    // CameraReady
    m_phraseBook["CameraReady"][English] = "Camera stream is connected and ready.";
    m_phraseBook["CameraReady"][Hindi] = "कैमरा स्ट्रीम कनेक्टेड और तैयार है।";
    m_phraseBook["CameraReady"][Marathi] = "कॅमेरा प्रवाह कनेक्ट झाला आहे आणि तयार आहे.";

    // LightingTooLow
    m_phraseBook["LightingTooLow"][English] = "Warning: Lighting is too low. Adjust exposure.";
    m_phraseBook["LightingTooLow"][Hindi] = "चेतावनी: रोशनी कम है। रोशनी बढ़ाएं।";
    m_phraseBook["LightingTooLow"][Marathi] = "इशारा: प्रकाश खूप कमी आहे. प्रकाश वाढवा.";

    // MoveCameraLeft
    m_phraseBook["MoveCameraLeft"][English] = "Please move the camera slightly to the left.";
    m_phraseBook["MoveCameraLeft"][Hindi] = "कृपया कैमरे को थोड़ा बाईं ओर ले जाएं।";
    m_phraseBook["MoveCameraLeft"][Marathi] = "कृपया कॅमेरा डावीकडे सरकवा.";

    // CameraStable
    m_phraseBook["CameraStable"][English] = "Camera position is stable. Initiating scan.";
    m_phraseBook["CameraStable"][Hindi] = "कैमरा स्थिर है। स्कैन शुरू हो रहा है।";
    m_phraseBook["CameraStable"][Marathi] = "कॅमेरा स्थिर आहे. स्कॅनिंग सुरू होत आहे.";

    // PatientMoving
    m_phraseBook["PatientMoving"][English] = "Patient movement detected. Please hold steady.";
    m_phraseBook["PatientMoving"][Hindi] = "मरीज का हिलना-डुलना महसूस हुआ। कृपया स्थिर रहें।";
    m_phraseBook["PatientMoving"][Marathi] = "रुग्णाची हालचाल आढळली. कृपया स्थिर रहा.";

    // ScanStarted
    m_phraseBook["ScanStarted"][English] = "Assisted screening simulation started. Hold steady.";
    m_phraseBook["ScanStarted"][Hindi] = "नैदानिक स्क्रीनिंग स्कैन शुरू हो गया है। स्थिर रहें।";
    m_phraseBook["ScanStarted"][Marathi] = "स्क्रीनिंग स्कॅनिंग सुरू झाले आहे. स्थिर रहा.";

    // GeneratingReport
    m_phraseBook["GeneratingReport"][English] = "Demonstration scan complete. Preparing an educational prototype report.";
    m_phraseBook["GeneratingReport"][Hindi] = "विश्लेषण पूरा हुआ। पीडीएफ मेडिकल रिपोर्ट बनाई जा रही है।";
    m_phraseBook["GeneratingReport"][Marathi] = "विश्लेषण पूर्ण झाले. पीडीएफ अहवाल तयार केला जात आहे.";

    // OfflineMode
    m_phraseBook["OfflineMode"][English] = "Offline mode enabled. Cache active.";
    m_phraseBook["OfflineMode"][Hindi] = "ऑफ़लाइन मोड सक्रिय। कैश सक्षम है।";
    m_phraseBook["OfflineMode"][Marathi] = "ऑफलाइन मोड सक्रिय. कॅश चालू आहे.";

    // Synced
    m_phraseBook["Synced"][English] = "Data synchronization complete.";
    m_phraseBook["Synced"][Hindi] = "डेटा सिंक्रनाइज़ेशन पूरा हुआ।";
    m_phraseBook["Synced"][Marathi] = "डेटा सिंक्रोनाइझेशन पूर्ण झाले.";

    // Error
    m_phraseBook["Error"][English] = "Error occurred. Check warning logs.";
    m_phraseBook["Error"][Hindi] = "त्रुटि आई है। चेतावनी लॉग जांचें।";
    m_phraseBook["Error"][Marathi] = "त्रुटी आढळली. वॉर्निंग लॉग तपासा.";
}
