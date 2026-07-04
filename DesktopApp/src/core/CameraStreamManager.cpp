#include "CameraStreamManager.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTcpSocket>
#include <QTimer>

class CameraStreamWorker final : public QObject {
    Q_OBJECT
public:
    explicit CameraStreamWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void configure(const QString& host, quint16 port, const QString& path) {
        m_host = host.trimmed();
        m_port = port;
        m_path = path.startsWith('/') ? path : '/' + path;
        m_candidates = {m_host, QStringLiteral("aegiscare-cam.local"), QStringLiteral("192.168.4.1")};
        m_candidates.removeDuplicates();
        m_candidateIndex = 0;
    }

    void start() {
        m_enabled = true;
        ensureObjects();
        connectNow(false);
    }

    void stop() {
        m_enabled = false;
        if (m_reconnectTimer) m_reconnectTimer->stop();
        if (m_statusTimer) m_statusTimer->stop();
        m_buffer.clear();
        if (m_socket) m_socket->abort();
        emit stateChanged(int(CameraStreamManager::State::Stopped), QStringLiteral("Stream stopped"));
    }

    void reconnect() {
        m_enabled = true;
        ensureObjects();
        m_attempt = 0;
        m_reconnectTimer->stop();
        m_socket->abort();
        connectNow(true);
    }

signals:
    void frameDecoded(const QImage& frame, int decodeLatencyMs, int encodedBytes);
    void stateChanged(int state, const QString& detail);
    void telemetryDecoded(const QJsonObject& telemetry);
    void endpointResolved(const QString& host, quint16 port);

private:
    QTcpSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QTimer* m_statusTimer = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    QByteArray m_buffer;
    QString m_host = QStringLiteral("192.168.4.1");
    quint16 m_port = 80;
    QString m_path = QStringLiteral("/stream");
    bool m_enabled = false;
    int m_attempt = 0;
    QStringList m_candidates{QStringLiteral("192.168.4.1"), QStringLiteral("aegiscare-cam.local")};
    int m_candidateIndex = 0;
    QString m_activeHost;

    void ensureObjects() {
        if (m_socket) return;
        m_socket = new QTcpSocket(this);
        m_reconnectTimer = new QTimer(this);
        m_statusTimer = new QTimer(this);
        m_network = new QNetworkAccessManager(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, [this]() { connectNow(true); });
        m_statusTimer->setInterval(2000);
        connect(m_statusTimer, &QTimer::timeout, this, &CameraStreamWorker::pollStatus);
        connect(m_socket, &QTcpSocket::connected, this, [this]() {
            m_attempt = 0;
            m_buffer.clear();
            m_activeHost = currentHost();
            const QByteArray request = "GET " + m_path.toUtf8() + " HTTP/1.1\r\nHost: " +
                                       m_activeHost.toUtf8() + "\r\nConnection: keep-alive\r\n\r\n";
            m_socket->write(request);
            emit endpointResolved(m_activeHost, m_port);
            emit stateChanged(int(CameraStreamManager::State::Connected), QStringLiteral("MJPEG stream connected"));
            m_statusTimer->start();
            pollStatus();
        });
        connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
            m_buffer += m_socket->readAll();
            if (m_buffer.size() > 4 * 1024 * 1024) m_buffer = m_buffer.right(2 * 1024 * 1024);
            for (;;) {
                const int begin = m_buffer.indexOf("\xFF\xD8", 0);
                if (begin < 0) return;
                const int end = m_buffer.indexOf("\xFF\xD9", begin + 2);
                if (end < 0) {
                    if (begin > 0) m_buffer.remove(0, begin);
                    return;
                }
                const QByteArray jpeg = m_buffer.mid(begin, end - begin + 2);
                m_buffer.remove(0, end + 2);
                QElapsedTimer decodeTimer;
                decodeTimer.start();
                const QImage frame = QImage::fromData(jpeg, "JPG");
                if (!frame.isNull()) emit frameDecoded(frame, int(decodeTimer.elapsed()), jpeg.size());
            }
        });
        connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
            m_statusTimer->stop();
            if (m_enabled) scheduleReconnect(QStringLiteral("Camera disconnected"));
        });
        connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            if (m_enabled) scheduleReconnect(m_socket->errorString());
        });
    }

    void connectNow(bool reconnecting) {
        if (!m_enabled || currentHost().isEmpty() || m_port == 0) return;
        ensureObjects();
        if (m_socket->state() == QAbstractSocket::ConnectedState ||
            m_socket->state() == QAbstractSocket::ConnectingState) return;
        emit stateChanged(int(reconnecting ? CameraStreamManager::State::Reconnecting
                                           : CameraStreamManager::State::Connecting),
                          QStringLiteral("Connecting to %1:%2").arg(currentHost()).arg(m_port));
        m_socket->connectToHost(currentHost(), m_port);
    }

    void scheduleReconnect(const QString& detail) {
        if (!m_enabled || m_reconnectTimer->isActive()) return;
        static constexpr int delays[] = {1000, 2000, 5000, 10000};
        const int delay = delays[qMin(m_attempt, 3)];
        m_attempt = qMin(m_attempt + 1, 3);
        if (!m_candidates.isEmpty()) m_candidateIndex = (m_candidateIndex + 1) % m_candidates.size();
        emit stateChanged(int(CameraStreamManager::State::Reconnecting),
                          QStringLiteral("%1; retry in %2 s").arg(detail).arg(delay / 1000));
        m_reconnectTimer->start(delay);
    }

    QString currentHost() const {
        return m_candidates.isEmpty() ? m_host : m_candidates.value(m_candidateIndex, m_host);
    }

    void pollStatus() {
        if (!m_enabled || m_activeHost.isEmpty() || !m_network) return;
        QUrl url;
        url.setScheme(QStringLiteral("http"));
        url.setHost(m_activeHost);
        url.setPort(m_port);
        url.setPath(QStringLiteral("/status"));
        QNetworkReply* reply = m_network->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray payload = reply->readAll();
            reply->deleteLater();
            QJsonParseError error{};
            const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
            if (error.error == QJsonParseError::NoError && document.isObject()) {
                emit telemetryDecoded(document.object());
            }
        });
    }
};

CameraStreamManager& CameraStreamManager::instance() {
    static CameraStreamManager manager;
    return manager;
}

CameraStreamManager::CameraStreamManager(QObject* parent) : QObject(parent) {
    m_worker = new CameraStreamWorker;
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &CameraStreamManager::requestConfigure, m_worker, &CameraStreamWorker::configure, Qt::QueuedConnection);
    connect(this, &CameraStreamManager::requestStart, m_worker, &CameraStreamWorker::start, Qt::QueuedConnection);
    connect(this, &CameraStreamManager::requestStop, m_worker, &CameraStreamWorker::stop, Qt::QueuedConnection);
    connect(this, &CameraStreamManager::requestReconnect, m_worker, &CameraStreamWorker::reconnect, Qt::QueuedConnection);
    connect(m_worker, &CameraStreamWorker::frameDecoded, this, &CameraStreamManager::acceptFrame, Qt::QueuedConnection);
    connect(m_worker, &CameraStreamWorker::stateChanged, this, &CameraStreamManager::acceptState, Qt::QueuedConnection);
    connect(m_worker, &CameraStreamWorker::telemetryDecoded, this, &CameraStreamManager::acceptTelemetry, Qt::QueuedConnection);
    connect(m_worker, &CameraStreamWorker::endpointResolved, this, &CameraStreamManager::acceptEndpoint, Qt::QueuedConnection);
    m_workerThread.setObjectName(QStringLiteral("ESP32CameraStreamWorker"));
    m_workerThread.start();
}

CameraStreamManager::~CameraStreamManager() {
    emit requestStop();
    m_workerThread.quit();
    m_workerThread.wait(1500);
}

void CameraStreamManager::configure(const QString& host, quint16 port, const QString& path) {
    m_host = host.trimmed();
    m_port = port;
    m_path = path;
    emit requestConfigure(m_host, m_port, m_path);
}

void CameraStreamManager::start() { emit requestStart(); }
void CameraStreamManager::stop() { emit requestStop(); }
void CameraStreamManager::reconnect() { emit requestReconnect(); }
void CameraStreamManager::refresh() { emit requestReconnect(); }

void CameraStreamManager::acceptFrame(const QImage& frame, int decodeLatencyMs, int encodedBytes) {
    m_latestFrame = frame;
    m_latencyMs = decodeLatencyMs;
    ++m_frameCount;
    ++m_windowFrames;
    m_windowBytes += encodedBytes;
    m_lastFrameTime = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    if (!m_fpsWindow.isValid()) m_fpsWindow.start();
    if (m_fpsWindow.elapsed() >= 1000) {
        const double seconds = m_fpsWindow.elapsed() / 1000.0;
        m_fps = m_windowFrames / seconds;
        m_bitrateKbps = (m_windowBytes * 8.0 / 1000.0) / seconds;
        m_windowFrames = 0;
        m_windowBytes = 0;
        m_fpsWindow.restart();
        emit statisticsChanged();
    }
    emit frameReady(frame);
}

void CameraStreamManager::acceptState(int state, const QString& detail) {
    m_state = static_cast<State>(state);
    if (m_state == State::Connected) m_connectionTimer.restart();
    emit statusChanged(m_state, detail);
    emit statisticsChanged();
}

void CameraStreamManager::acceptTelemetry(const QJsonObject& telemetry) {
    m_firmwareVersion = telemetry.value(QStringLiteral("firmware")).toString(m_firmwareVersion);
    m_reportedResolution = telemetry.value(QStringLiteral("resolution")).toString(m_reportedResolution);
    m_reportedIp = telemetry.value(QStringLiteral("ip")).toString(m_reportedIp);
    m_wifiConnected = telemetry.value(QStringLiteral("wifi_connected")).toBool();
    m_cameraOnline = telemetry.value(QStringLiteral("camera_online")).toBool();
    m_wifiRssi = telemetry.value(QStringLiteral("wifi_rssi")).toInt();
    m_uptimeMs = static_cast<quint64>(telemetry.value(QStringLiteral("uptime_ms")).toDouble());
    m_deviceFps = telemetry.value(QStringLiteral("fps")).toDouble();
    emit telemetryChanged(telemetry);
    emit statisticsChanged();
}

void CameraStreamManager::acceptEndpoint(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    emit statisticsChanged();
}

QString CameraStreamManager::connectionTime() const {
    if (!m_connectionTimer.isValid() || !isConnected()) return QStringLiteral("00:00:00");
    const qint64 seconds = m_connectionTimer.elapsed() / 1000;
    return QStringLiteral("%1:%2:%3")
        .arg(seconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

QString CameraStreamManager::stateText(State state) {
    switch (state) {
        case State::Connecting: return QStringLiteral("Connecting...");
        case State::Connected: return QStringLiteral("Connected");
        case State::Reconnecting: return QStringLiteral("Reconnecting...");
        case State::Stopped: return QStringLiteral("Stopped");
        case State::Offline: return QStringLiteral("Camera Offline");
    }
    return QStringLiteral("Camera Offline");
}

#include "CameraStreamManager.moc"
