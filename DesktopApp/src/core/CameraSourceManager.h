#ifndef CAMERASOURCEMANAGER_H
#define CAMERASOURCEMANAGER_H

#include "CameraProvider.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>

class ESP32CameraProvider;
class LaptopCameraProvider;

class CameraSourceManager final : public QObject {
    Q_OBJECT
public:
    enum class Mode { Auto, Esp32Only, LaptopOnly };
    Q_ENUM(Mode)

    static CameraSourceManager& instance();

    void start();
    void stop();
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }
    CameraProvider::Source activeSource() const { return m_activeSource; }
    QString activeSourceName() const;
    QString modeName() const;
    bool isStreaming() const { return m_streaming; }
    bool esp32Available() const;
    bool laptopAvailable() const;
    QImage latestFrame() const { return m_latestFrame; }
    QSize resolution() const { return m_latestFrame.size(); }
    double fps() const { return m_fps; }
    QDateTime lastConnectionTime() const { return m_lastConnectionTime; }

    static QString sourceName(CameraProvider::Source source);

signals:
    void frameReady(const QImage& frame);
    void sourceChanged(CameraProvider::Source source, const QString& reason);
    void statusChanged();
    void notificationRequested(const QString& message, const QString& tone);

private:
    explicit CameraSourceManager(QObject* parent = nullptr);
    Q_DISABLE_COPY_MOVE(CameraSourceManager)

    ESP32CameraProvider* m_esp32 = nullptr;
    LaptopCameraProvider* m_laptop = nullptr;
    QTimer m_fallbackTimer;
    QTimer m_healthTimer;
    QElapsedTimer m_lastEspFrame;
    QElapsedTimer m_fpsWindow;
    Mode m_mode = Mode::Auto;
    CameraProvider::Source m_activeSource = CameraProvider::Source::None;
    bool m_started = false;
    bool m_streaming = false;
    QImage m_latestFrame;
    int m_windowFrames = 0;
    double m_fps = 0.0;
    QDateTime m_lastConnectionTime;

    void activate(CameraProvider::Source source, const QString& reason, const QString& toast = {});
    void handleEsp32Frame(const QImage& frame);
    void handleLaptopFrame(const QImage& frame);
    void publishFrame(const QImage& frame);
    void useFallback(const QString& reason);
};

#endif
