#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <QElapsedTimer>
#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

#include "../communication/CommunicationLogger.h"
#include "../protocol/JsonLineProtocol.h"

/**
 * Owns the non-blocking Arduino serial transport and bridges the shared
 * CameraStreamManager for backward-compatible ESP32 frame/status signals.
 * The serial side provides discovery, reconnect, JSON framing, heartbeat
 * supervision, metadata, logging, and backward-compatible Phase 1 events.
 */
class CommunicationManager : public QObject {
    Q_OBJECT
public:
    enum class ArduinoConnectionState {
        Disconnected,
        Opening,
        AwaitingHeartbeat,
        Connected
    };
    Q_ENUM(ArduinoConnectionState)

    static CommunicationManager& instance();

    void init();
    void startAll();
    void stopAll();

    bool connect();
    void disconnect();
    bool sendJSON(const QJsonObject& object);
    void receiveJSON(const QJsonObject& object);

    bool isEspConnected() const { return m_espConnected; }
    bool isArduinoConnected() const { return m_arduinoConnected; }
    bool isEmulationMode() const { return m_emulationActive; }
    QString currentSerialPort() const { return m_serialPortName; }
    QString firmwareVersion() const { return m_firmwareVersion; }
    ArduinoConnectionState arduinoConnectionState() const { return m_arduinoState; }
    qint64 lastHeartbeatAgeMs() const;
    QString communicationLogPath() const { return m_logger.filePath(); }

    void sendArduinoCommand(const QString& command);
    void toggleEmulation(bool enable);

signals:
    void espStatusChanged(bool connected);
    void arduinoStatusChanged(bool connected);
    void arduinoConnectionStateChanged(CommunicationManager::ArduinoConnectionState state);
    void serialMetadataChanged(const QString& portName, const QString& firmwareVersion);
    void heartbeatUpdated(qint64 ageMs);
    void jsonReceived(const QJsonObject& object);
    void protocolError(const QString& error);
    void frameReceived(const QImage& frame);
    void messageReceived(const QString& sender, const QString& message);
    void buttonPressed(int buttonNum);
    void joystickMoved(const QString& direction);
    void scanControlReceived(const QString& action);
    void developerAboutRequested();
    void patientChanged(int index);
    void patientConfirmed();
    void sensorDataReceived(int heartRate, int spo2, double temperature, int sysBP, int diaBP);
    void deviceHeartbeat(const QString& device, bool healthy);

private:
    explicit CommunicationManager(QObject* parent = nullptr);
    ~CommunicationManager() override;
    CommunicationManager(const CommunicationManager&) = delete;
    CommunicationManager& operator=(const CommunicationManager&) = delete;

    static constexpr int HeartbeatIntervalMs = 1000;
    static constexpr int HeartbeatTimeoutMs = 5000;
    static constexpr int ReconnectDelayCount = 4;

    QString m_espIp = QStringLiteral("192.168.4.1");
    quint16 m_espPort = 80;
    bool m_espConnected = false;

    QSerialPort* m_serialPort = nullptr;
    QString m_serialPortName;
    QString m_firmwareVersion = QStringLiteral("Unknown");
    bool m_arduinoConnected = false;
    ArduinoConnectionState m_arduinoState = ArduinoConnectionState::Disconnected;
    JsonLineProtocol m_serialProtocol;
    CommunicationLogger m_logger;
    QElapsedTimer m_lastHeartbeat;
    QElapsedTimer m_connectionOpened;

    bool m_emulationActive = true;
    QTimer* m_emulationTimer = nullptr;
    QTimer* m_serialCheckTimer = nullptr;
    QTimer* m_heartbeatTimer = nullptr;
    int m_emulationAngle = 0;
    bool m_initialized = false;
    bool m_running = false;
    bool m_reconnectScheduled = false;
    int m_reconnectAttempt = 0;

    void setupConnections();
    void setArduinoState(ArduinoConnectionState state);
    void markHeartbeatReceived();
    void handleHeartbeatTimer();
    void handleSerialReadyRead();
    void handleSerialError(QSerialPort::SerialPortError error);
    void tryConnectArduino();
    void scheduleReconnect(const QString& reason);
    void closeSerialForReconnect(const QString& reason);
    static int reconnectDelayMs(int attempt);
    void tryConnectEsp32();
    bool openSerialPort(const QSerialPortInfo& portInfo);
    static bool isArduinoCandidate(const QSerialPortInfo& portInfo);
    void dispatchOperatorEvent(const QJsonObject& object);

    void generateEmulatedFrame();
    void generateEmulatedSensors();
    void generateEmulatedControls();
};

#endif // COMMUNICATIONMANAGER_H
