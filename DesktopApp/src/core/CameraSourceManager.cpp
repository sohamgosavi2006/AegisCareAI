#include "CameraSourceManager.h"
#include "ESP32CameraProvider.h"
#include "LaptopCameraProvider.h"

CameraSourceManager& CameraSourceManager::instance() {
    static CameraSourceManager manager;
    return manager;
}

CameraSourceManager::CameraSourceManager(QObject* parent) : QObject(parent) {
    m_esp32 = new ESP32CameraProvider(this);
    m_laptop = new LaptopCameraProvider(this);
    m_fallbackTimer.setSingleShot(true);
    m_fallbackTimer.setInterval(2800);
    m_healthTimer.setInterval(1000);

    connect(m_esp32, &CameraProvider::frameReady, this, &CameraSourceManager::handleEsp32Frame);
    connect(m_laptop, &CameraProvider::frameReady, this, &CameraSourceManager::handleLaptopFrame);
    connect(&m_fallbackTimer, &QTimer::timeout, this, [this]() {
        if (m_mode == Mode::Auto && (!m_lastEspFrame.isValid() || m_lastEspFrame.elapsed() > 2500)) {
            useFallback(QStringLiteral("ESP32 Camera unavailable"));
        }
    });
    connect(&m_healthTimer, &QTimer::timeout, this, [this]() {
        if (m_mode == Mode::Auto && m_activeSource == CameraProvider::Source::Esp32 &&
            m_lastEspFrame.isValid() && m_lastEspFrame.elapsed() > 4000) {
            emit notificationRequested(QStringLiteral("ESP32 Connection Lost"), QStringLiteral("orange"));
            useFallback(QStringLiteral("ESP32 stream timed out"));
        }
    });
    connect(m_laptop, &CameraProvider::providerError, this, [this](const QString& message) {
        if (m_activeSource == CameraProvider::Source::Laptop) {
            m_streaming = false;
            emit notificationRequested(message, QStringLiteral("orange"));
            emit statusChanged();
        }
    });
}

void CameraSourceManager::start() {
    if (m_started) return;
    m_started = true;
    m_healthTimer.start();
    if (m_mode == Mode::LaptopOnly) {
        m_laptop->start();
        activate(CameraProvider::Source::Laptop, QStringLiteral("Laptop camera selected"),
                 QStringLiteral("Using Laptop Camera"));
        return;
    }
    m_esp32->start();
    if (m_mode == Mode::Auto) {
        m_fallbackTimer.start();
        emit notificationRequested(QStringLiteral("Searching for ESP32-CAM"), QStringLiteral("blue"));
    } else {
        activate(CameraProvider::Source::Esp32, QStringLiteral("ESP32-CAM selected"));
    }
}

void CameraSourceManager::stop() {
    m_started = false;
    m_streaming = false;
    m_fallbackTimer.stop();
    m_healthTimer.stop();
    m_esp32->stop();
    m_laptop->stop();
    activate(CameraProvider::Source::None, QStringLiteral("Camera service stopped"));
}

void CameraSourceManager::setMode(Mode mode) {
    if (m_mode == mode && m_started) return;
    m_mode = mode;
    m_fallbackTimer.stop();
    if (!m_started) {
        emit statusChanged();
        return;
    }
    if (mode == Mode::LaptopOnly) {
        m_esp32->stop();
        m_laptop->start();
        activate(CameraProvider::Source::Laptop, QStringLiteral("Manual laptop camera mode"),
                 QStringLiteral("Using Laptop Camera"));
    } else if (mode == Mode::Esp32Only) {
        m_laptop->stop();
        m_esp32->start();
        activate(CameraProvider::Source::Esp32, QStringLiteral("Manual ESP32-CAM mode"));
    } else {
        m_esp32->start();
        if (m_esp32->isAvailable()) {
            activate(CameraProvider::Source::Esp32, QStringLiteral("ESP32-CAM preferred in Auto mode"));
        } else {
            m_fallbackTimer.start();
            if (m_laptop->isAvailable()) activate(CameraProvider::Source::Laptop, QStringLiteral("Automatic fallback active"));
        }
    }
    emit statusChanged();
}

void CameraSourceManager::handleEsp32Frame(const QImage& frame) {
    m_lastEspFrame.restart();
    if (m_mode == Mode::LaptopOnly) return;
    if (m_mode == Mode::Auto && m_activeSource != CameraProvider::Source::Esp32) {
        m_fallbackTimer.stop();
        activate(CameraProvider::Source::Esp32, QStringLiteral("ESP32-CAM became available"),
                 QStringLiteral("ESP32 Connected"));
    }
    if (m_activeSource == CameraProvider::Source::Esp32) publishFrame(frame);
}

void CameraSourceManager::handleLaptopFrame(const QImage& frame) {
    if (m_activeSource == CameraProvider::Source::Laptop) publishFrame(frame);
}

void CameraSourceManager::publishFrame(const QImage& frame) {
    if (frame.isNull()) return;
    if (!m_streaming) m_lastConnectionTime = QDateTime::currentDateTime();
    m_latestFrame = frame;
    m_streaming = true;
    ++m_windowFrames;
    if (!m_fpsWindow.isValid()) m_fpsWindow.start();
    if (m_fpsWindow.elapsed() >= 1000) {
        m_fps = m_windowFrames * 1000.0 / m_fpsWindow.elapsed();
        m_windowFrames = 0;
        m_fpsWindow.restart();
        emit statusChanged();
    }
    emit frameReady(frame);
}

void CameraSourceManager::useFallback(const QString& reason) {
    if (m_mode != Mode::Auto) return;
    emit notificationRequested(QStringLiteral("ESP32 Camera unavailable — switching to Laptop Camera"),
                               QStringLiteral("blue"));
    m_laptop->start();
    activate(CameraProvider::Source::Laptop, reason, QStringLiteral("Switched to Laptop Camera"));
}

void CameraSourceManager::activate(CameraProvider::Source source, const QString& reason, const QString& toast) {
    if (m_activeSource == source) return;
    m_activeSource = source;
    m_streaming = false;
    m_latestFrame = QImage();
    m_windowFrames = 0;
    m_fps = 0.0;
    m_fpsWindow.invalidate();
    if (source == CameraProvider::Source::Esp32) {
        m_laptop->stop();
    }
    emit sourceChanged(source, reason);
    emit statusChanged();
    if (!toast.isEmpty()) emit notificationRequested(
        toast, source == CameraProvider::Source::Esp32 ? QStringLiteral("green") : QStringLiteral("blue"));
}

QString CameraSourceManager::sourceName(CameraProvider::Source source) {
    switch (source) {
        case CameraProvider::Source::Esp32: return QStringLiteral("ESP32-CAM");
        case CameraProvider::Source::Laptop: return QStringLiteral("Laptop Camera");
        case CameraProvider::Source::None: return QStringLiteral("Searching...");
    }
    return QStringLiteral("Searching...");
}

QString CameraSourceManager::activeSourceName() const { return sourceName(m_activeSource); }

QString CameraSourceManager::modeName() const {
    switch (m_mode) {
        case Mode::Auto: return QStringLiteral("Auto");
        case Mode::Esp32Only: return QStringLiteral("ESP32-CAM");
        case Mode::LaptopOnly: return QStringLiteral("Laptop Webcam");
    }
    return QStringLiteral("Auto");
}

bool CameraSourceManager::esp32Available() const { return m_esp32->isAvailable(); }
bool CameraSourceManager::laptopAvailable() const { return m_laptop->isAvailable(); }
