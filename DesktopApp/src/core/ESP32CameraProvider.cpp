#include "ESP32CameraProvider.h"
#include "CameraStreamManager.h"

ESP32CameraProvider::ESP32CameraProvider(QObject* parent) : CameraProvider(parent) {
    auto& stream = CameraStreamManager::instance();
    connect(&stream, &CameraStreamManager::frameReady, this, [this](const QImage& frame) {
        if (!m_running || frame.isNull()) return;
        setAvailable(true);
        emit frameReady(frame);
    });
    connect(&stream, &CameraStreamManager::statusChanged, this,
            [this](CameraStreamManager::State state, const QString& detail) {
        if (state == CameraStreamManager::State::Offline ||
            state == CameraStreamManager::State::Stopped) {
            setAvailable(false);
        }
        if (state == CameraStreamManager::State::Offline && !detail.isEmpty()) emit providerError(detail);
    });
}

void ESP32CameraProvider::start() {
    if (m_running) return;
    m_running = true;
    CameraStreamManager::instance().start();
}

void ESP32CameraProvider::stop() {
    m_running = false;
    setAvailable(false);
    CameraStreamManager::instance().stop();
}

void ESP32CameraProvider::setAvailable(bool available) {
    if (m_available == available) return;
    m_available = available;
    emit availabilityChanged(available);
}

