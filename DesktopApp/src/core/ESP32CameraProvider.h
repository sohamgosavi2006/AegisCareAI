#ifndef ESP32CAMERAPROVIDER_H
#define ESP32CAMERAPROVIDER_H

#include "CameraProvider.h"

class ESP32CameraProvider final : public CameraProvider {
    Q_OBJECT
public:
    explicit ESP32CameraProvider(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool isAvailable() const override { return m_available; }
    Source source() const override { return Source::Esp32; }
    QString displayName() const override { return QStringLiteral("ESP32-CAM"); }

private:
    bool m_running = false;
    bool m_available = false;
    void setAvailable(bool available);
};

#endif
