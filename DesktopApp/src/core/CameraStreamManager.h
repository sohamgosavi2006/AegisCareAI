#ifndef CAMERASTREAMMANAGER_H
#define CAMERASTREAMMANAGER_H

#include <QElapsedTimer>
#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QThread>

class CameraStreamWorker;

class CameraStreamManager final : public QObject {
    Q_OBJECT
public:
    enum class State { Offline, Connecting, Connected, Reconnecting, Stopped };
    Q_ENUM(State)

    static CameraStreamManager& instance();
    ~CameraStreamManager() override;

    void configure(const QString& host, quint16 port, const QString& path = QStringLiteral("/stream"));
    void start();
    void stop();
    void reconnect();
    void refresh();

    State state() const { return m_state; }
    bool isConnected() const { return m_state == State::Connected; }
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    QImage latestFrame() const { return m_latestFrame; }
    double fps() const { return m_fps; }
    int latencyMs() const { return m_latencyMs; }
    quint64 frameCount() const { return m_frameCount; }
    double bitrateKbps() const { return m_bitrateKbps; }
    QString connectionTime() const;
    QString lastFrameTime() const { return m_lastFrameTime; }
    QString firmwareVersion() const { return m_firmwareVersion; }
    QString reportedResolution() const { return m_reportedResolution; }
    QString reportedIp() const { return m_reportedIp; }
    bool wifiConnected() const { return m_wifiConnected; }
    bool cameraOnline() const { return m_cameraOnline; }
    int wifiRssi() const { return m_wifiRssi; }
    quint64 uptimeMs() const { return m_uptimeMs; }
    double deviceFps() const { return m_deviceFps; }
    static QString stateText(State state);

signals:
    void requestConfigure(const QString& host, quint16 port, const QString& path);
    void requestStart();
    void requestStop();
    void requestReconnect();
    void frameReady(const QImage& frame);
    void statusChanged(CameraStreamManager::State state, const QString& detail);
    void statisticsChanged();
    void telemetryChanged(const QJsonObject& telemetry);

private slots:
    void acceptFrame(const QImage& frame, int decodeLatencyMs, int encodedBytes);
    void acceptState(int state, const QString& detail);
    void acceptTelemetry(const QJsonObject& telemetry);
    void acceptEndpoint(const QString& host, quint16 port);

private:
    explicit CameraStreamManager(QObject* parent = nullptr);
    Q_DISABLE_COPY_MOVE(CameraStreamManager)

    QThread m_workerThread;
    CameraStreamWorker* m_worker = nullptr;
    State m_state = State::Offline;
    QString m_host = QStringLiteral("192.168.4.1");
    quint16 m_port = 80;
    QString m_path = QStringLiteral("/stream");
    QImage m_latestFrame;
    QElapsedTimer m_fpsWindow;
    QElapsedTimer m_connectionTimer;
    int m_windowFrames = 0;
    qint64 m_windowBytes = 0;
    double m_fps = 0.0;
    double m_bitrateKbps = 0.0;
    int m_latencyMs = 0;
    quint64 m_frameCount = 0;
    QString m_lastFrameTime = QStringLiteral("Never");
    QString m_firmwareVersion = QStringLiteral("Unknown");
    QString m_reportedResolution = QStringLiteral("--");
    QString m_reportedIp;
    bool m_wifiConnected = false;
    bool m_cameraOnline = false;
    int m_wifiRssi = 0;
    quint64 m_uptimeMs = 0;
    double m_deviceFps = 0.0;
};

#endif
