#include "CommunicationManager.h"
#include "CameraStreamManager.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QRandomGenerator>
#include <QSet>
#include <cmath>

CommunicationManager& CommunicationManager::instance() {
    static CommunicationManager inst;
    return inst;
}

CommunicationManager::CommunicationManager(QObject* parent) : QObject(parent) {
    m_serialPort = new QSerialPort(this);
    m_emulationTimer = new QTimer(this);
    m_serialCheckTimer = new QTimer(this);
    m_heartbeatTimer = new QTimer(this);
    m_serialCheckTimer->setSingleShot(true);

    QObject::connect(m_emulationTimer, &QTimer::timeout, this, &CommunicationManager::generateEmulatedFrame);
    QObject::connect(m_emulationTimer, &QTimer::timeout, this, &CommunicationManager::generateEmulatedSensors);
    QObject::connect(m_serialCheckTimer, &QTimer::timeout, this, &CommunicationManager::tryConnectArduino);
    QObject::connect(m_heartbeatTimer, &QTimer::timeout, this, &CommunicationManager::handleHeartbeatTimer);
}

CommunicationManager::~CommunicationManager() {
    stopAll();
}

void CommunicationManager::init() {
    if (m_initialized) {
        return;
    }
    setupConnections();
    m_initialized = true;
}

void CommunicationManager::startAll() {
    init();
    if (m_running) {
        return;
    }
    m_running = true;
    m_heartbeatTimer->start(HeartbeatIntervalMs);
    if (m_emulationActive) {
        m_emulationTimer->start(33);
    }
    tryConnectArduino();
    tryConnectEsp32();
}

void CommunicationManager::stopAll() {
    m_running = false;
    m_reconnectScheduled = false;
    m_serialCheckTimer->stop();
    m_emulationTimer->stop();
    m_heartbeatTimer->stop();
    disconnect();
}

bool CommunicationManager::connect() {
    if (m_serialPort->isOpen()) {
        return true;
    }
    tryConnectArduino();
    return m_serialPort->isOpen();
}

void CommunicationManager::disconnect() {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    m_serialProtocol.clear();
    m_lastHeartbeat.invalidate();
    m_connectionOpened.invalidate();
    m_serialPortName.clear();
    m_firmwareVersion = QStringLiteral("Unknown");
    setArduinoState(ArduinoConnectionState::Disconnected);
    emit serialMetadataChanged(m_serialPortName, m_firmwareVersion);
}

bool CommunicationManager::sendJSON(const QJsonObject& object) {
    if (!m_serialPort->isOpen()) {
        return false;
    }
    const QByteArray packet = JsonLineProtocol::encode(object);
    const qint64 accepted = m_serialPort->write(packet);
    if (accepted != packet.size()) {
        m_logger.logError(QStringLiteral("Serial write rejected"));
        return false;
    }
    m_logger.logTransmit(packet);
    return true;
}

void CommunicationManager::receiveJSON(const QJsonObject& object) {
    emit jsonReceived(object);
    const QString type = object.value(QStringLiteral("type")).toString().toLower();
    const QString status = object.value(QStringLiteral("status")).toString();
    const QString value = object.value(QStringLiteral("value")).toString().toUpper();

    if (type == QStringLiteral("pong") ||
        (type == QStringLiteral("heartbeat") && value == QStringLiteral("PONG")) ||
        status == QStringLiteral("HEARTBEAT")) {
        markHeartbeatReceived();
        return;
    }
    if (type == QStringLiteral("device") || status == QStringLiteral("DEVICE_READY")) {
        const QString version = object.value(QStringLiteral("firmware")).toString();
        if (!version.isEmpty()) {
            m_firmwareVersion = version;
        }
        markHeartbeatReceived();
        emit serialMetadataChanged(m_serialPortName, m_firmwareVersion);
    }
    dispatchOperatorEvent(object);
}

qint64 CommunicationManager::lastHeartbeatAgeMs() const {
    return m_lastHeartbeat.isValid() ? m_lastHeartbeat.elapsed() : -1;
}

void CommunicationManager::toggleEmulation(bool enable) {
    m_emulationActive = enable;
    if (enable) {
        m_emulationTimer->start(33);
        m_espConnected = true;
        emit espStatusChanged(true);
    } else {
        m_emulationTimer->stop();
        CameraStreamManager::instance().start();
        m_espConnected = CameraStreamManager::instance().isConnected();
        emit espStatusChanged(m_espConnected);
    }
}

void CommunicationManager::setupConnections() {
    QObject::connect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationManager::handleSerialReadyRead);
    QObject::connect(m_serialPort, &QSerialPort::errorOccurred, this, &CommunicationManager::handleSerialError);
    QObject::connect(&CameraStreamManager::instance(), &CameraStreamManager::frameReady, this,
                     [this](const QImage& frame) {
        if (!m_emulationActive) emit frameReceived(frame);
    });
    QObject::connect(&CameraStreamManager::instance(), &CameraStreamManager::statusChanged, this,
                     [this](CameraStreamManager::State state, const QString&) {
        const bool connected = state == CameraStreamManager::State::Connected;
        if (m_espConnected == connected) return;
        m_espConnected = connected;
        emit espStatusChanged(connected);
        emit deviceHeartbeat(QStringLiteral("ESP32-CAM"), connected);
    });
}

void CommunicationManager::tryConnectArduino() {
    m_reconnectScheduled = false;
    if (!m_running || m_serialPort->isOpen() ||
        m_arduinoState == ArduinoConnectionState::Opening) {
        return;
    }
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& portInfo : ports) {
        if (isArduinoCandidate(portInfo) && openSerialPort(portInfo)) {
            return;
        }
    }
    scheduleReconnect(QStringLiteral("No compatible Arduino serial port found"));
}

bool CommunicationManager::openSerialPort(const QSerialPortInfo& portInfo) {
    setArduinoState(ArduinoConnectionState::Opening);
    m_serialPort->setPort(portInfo);
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        m_logger.logError(QStringLiteral("Unable to open %1: %2")
                              .arg(portInfo.portName(), m_serialPort->errorString()));
        setArduinoState(ArduinoConnectionState::Disconnected);
        scheduleReconnect(QStringLiteral("Port open failed"));
        return false;
    }

    // QSerialPort defaults both modem-control lines to false. Reasserting this
    // after the one initial open prevents later application code from holding
    // DTR/RTS high and retriggering the Nano auto-reset circuit.
    m_serialPort->setDataTerminalReady(false);
    m_serialPort->setRequestToSend(false);
    m_serialPortName = portInfo.portName();
    m_serialCheckTimer->stop();
    m_reconnectScheduled = false;
    m_serialProtocol.clear();
    m_lastHeartbeat.invalidate();
    m_connectionOpened.start();
    setArduinoState(ArduinoConnectionState::AwaitingHeartbeat);
    m_logger.logConnection(QStringLiteral("Opened %1; awaiting heartbeat").arg(m_serialPortName));
    emit serialMetadataChanged(m_serialPortName, m_firmwareVersion);
    sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("heartbeat")},
                         {QStringLiteral("value"), QStringLiteral("PING")}});
    return true;
}

bool CommunicationManager::isArduinoCandidate(const QSerialPortInfo& portInfo) {
    static const QSet<quint16> supportedVendorIds = {0x2341, 0x2A03, 0x1A86, 0x0403, 0x10C4};
    const QString identity = QStringLiteral("%1 %2 %3")
                                 .arg(portInfo.portName(), portInfo.description(), portInfo.manufacturer())
                                 .toLower();
    return (portInfo.hasVendorIdentifier() && supportedVendorIds.contains(portInfo.vendorIdentifier())) ||
           identity.contains(QStringLiteral("arduino")) ||
           identity.contains(QStringLiteral("usbserial")) ||
           identity.contains(QStringLiteral("usbmodem")) ||
           identity.contains(QStringLiteral("ttyacm")) ||
           identity.contains(QStringLiteral("ttyusb")) ||
           identity.contains(QStringLiteral("wch"));
}

void CommunicationManager::tryConnectEsp32() {
    if (m_espConnected) return;
    if (m_emulationActive) {
        m_espConnected = true;
        emit espStatusChanged(true);
        return;
    }

    CameraStreamManager::instance().configure(m_espIp, m_espPort, QStringLiteral("/stream"));
    CameraStreamManager::instance().start();
}

void CommunicationManager::handleSerialReadyRead() {
    const QByteArray bytes = m_serialPort->readAll();
    const JsonLineProtocol::Result result = m_serialProtocol.append(bytes);
    for (const QString& error : result.errors) {
        m_logger.logError(error);
        emit protocolError(error);
    }
    for (const QJsonObject& object : result.objects) {
        const QByteArray packet = JsonLineProtocol::encode(object);
        m_logger.logReceive(packet);
        emit messageReceived(QStringLiteral("Arduino Nano"), QString::fromUtf8(packet).trimmed());
        receiveJSON(object);
    }
}

void CommunicationManager::handleSerialError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }
    const QString message = QStringLiteral("Serial error on %1: %2")
                                .arg(m_serialPortName, m_serialPort->errorString());
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError ||
        error == QSerialPort::PermissionError || error == QSerialPort::OpenError ||
        error == QSerialPort::NotOpenError) {
        m_logger.logError(message);
        closeSerialForReconnect(message);
        return;
    }
    // Framing, parity, break, and transient read/write errors are logged but do
    // not tear down an otherwise healthy USB link.
    m_logger.logWarning(message);
}

void CommunicationManager::setArduinoState(ArduinoConnectionState state) {
    if (m_arduinoState == state) {
        return;
    }
    m_arduinoState = state;
    const bool connected = state == ArduinoConnectionState::Connected;
    if (m_arduinoConnected != connected) {
        m_arduinoConnected = connected;
        emit arduinoStatusChanged(connected);
        emit deviceHeartbeat(QStringLiteral("Arduino Nano"), connected);
    }
    emit arduinoConnectionStateChanged(state);
}

void CommunicationManager::markHeartbeatReceived() {
    const bool wasConnected = m_arduinoState == ArduinoConnectionState::Connected;
    m_lastHeartbeat.restart();
    m_connectionOpened.invalidate();
    m_reconnectAttempt = 0;
    setArduinoState(ArduinoConnectionState::Connected);
    if (!wasConnected) {
        m_logger.logConnection(QStringLiteral("Heartbeat established on %1").arg(m_serialPortName));
    }
    emit heartbeatUpdated(0);
}

void CommunicationManager::handleHeartbeatTimer() {
    if (!m_serialPort->isOpen()) {
        return;
    }
    const qint64 ageMs = m_lastHeartbeat.isValid()
                             ? m_lastHeartbeat.elapsed()
                             : (m_connectionOpened.isValid() ? m_connectionOpened.elapsed() : 0);
    if (ageMs >= HeartbeatTimeoutMs) {
        const QString warning = QStringLiteral("Missing heartbeat for %1 ms on %2")
                                    .arg(ageMs)
                                    .arg(m_serialPortName);
        m_logger.logWarning(warning);
        closeSerialForReconnect(warning);
        return;
    }
    if (m_arduinoState == ArduinoConnectionState::AwaitingHeartbeat) {
        emit heartbeatUpdated(ageMs);
        return;
    }
    sendJSON(QJsonObject{{QStringLiteral("type"), QStringLiteral("heartbeat")},
                         {QStringLiteral("value"), QStringLiteral("PING")}});
    emit heartbeatUpdated(ageMs);
}

void CommunicationManager::scheduleReconnect(const QString& reason) {
    if (!m_running || m_reconnectScheduled || m_serialPort->isOpen()) {
        return;
    }
    const int delayMs = reconnectDelayMs(m_reconnectAttempt);
    m_reconnectAttempt = qMin(m_reconnectAttempt + 1, ReconnectDelayCount - 1);
    m_reconnectScheduled = true;
    m_logger.logReconnect(QStringLiteral("%1; retry in %2 ms").arg(reason).arg(delayMs));
    m_serialCheckTimer->start(delayMs);
}

void CommunicationManager::closeSerialForReconnect(const QString& reason) {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    m_serialProtocol.clear();
    m_lastHeartbeat.invalidate();
    m_connectionOpened.invalidate();
    setArduinoState(ArduinoConnectionState::Disconnected);
    scheduleReconnect(reason);
}

int CommunicationManager::reconnectDelayMs(int attempt) {
    static constexpr int delaysMs[ReconnectDelayCount] = {1000, 2000, 5000, 10000};
    return delaysMs[qBound(0, attempt, ReconnectDelayCount - 1)];
}

void CommunicationManager::dispatchOperatorEvent(const QJsonObject& object) {
    const QString type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("patient_changed")) {
        emit patientChanged(object.value(QStringLiteral("index")).toInt(-1));
        return;
    }
    if (type == QStringLiteral("patient_confirm")) {
        emit patientConfirmed();
        return;
    }
    if (type == QStringLiteral("joystick")) {
        emit joystickMoved(object.value(QStringLiteral("direction")).toString().toUpper());
        return;
    }
    if (type == QStringLiteral("scan_control")) {
        emit scanControlReceived(object.value(QStringLiteral("action")).toString().toUpper());
        return;
    }
    if (type == QStringLiteral("button")) {
        const QString id = object.value(QStringLiteral("id")).toString().toUpper();
        if (id == QStringLiteral("VOICE_RECORD_START") || id == QStringLiteral("VOICE_RECORD_STOP")) emit buttonPressed(2);
        else if (id == QStringLiteral("REPORT")) emit buttonPressed(3);
        else if (id == QStringLiteral("EMERGENCY")) emit buttonPressed(4);
        else emit buttonPressed(1);
        return;
    }

    const QString event = object.value(QStringLiteral("event")).toString().toUpper();
    if (event == QStringLiteral("UP") || event == QStringLiteral("DOWN") ||
        event == QStringLiteral("LEFT") || event == QStringLiteral("RIGHT") ||
        event == QStringLiteral("CLICK")) {
        emit joystickMoved(event);
    } else if (event == QStringLiteral("START_SCAN")) {
        emit buttonPressed(1);
    } else if (event == QStringLiteral("VOICE_NOTES")) {
        emit buttonPressed(2);
    } else if (event == QStringLiteral("REPORTS") || event == QStringLiteral("REPORT")) {
        emit buttonPressed(3);
    } else if (event == QStringLiteral("EMERGENCY")) {
        emit buttonPressed(4);
    } else if (event == QStringLiteral("ABOUT_DEVELOPER") || event == QStringLiteral("DOUBLE_CLICK")) {
        emit developerAboutRequested();
    }
}

void CommunicationManager::sendArduinoCommand(const QString& command) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(command.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && document.isObject()) {
        sendJSON(document.object());
        return;
    }
    sendJSON(QJsonObject{{QStringLiteral("command"), command}});
}

// Emulated Data Generators
void CommunicationManager::generateEmulatedFrame() {
    m_emulationAngle = (m_emulationAngle + 2) % 360;

    // Create high-tech digital medical image
    QImage frame(640, 480, QImage::Format_RGB32);
    frame.fill(QColor(10, 15, 25)); // Deep dark blue background

    QPainter painter(&frame);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw HUD Grid
    painter.setPen(QPen(QColor(255, 255, 255, 10), 1, Qt::SolidLine));
    int gridSize = 40;
    for (int x = 0; x < frame.width(); x += gridSize) {
        painter.drawLine(x, 0, x, frame.height());
    }
    for (int y = 0; y < frame.height(); y += gridSize) {
        painter.drawLine(0, y, frame.width(), y);
    }

    // Draw scanning radar arcs
    int cx = frame.width() / 2;
    int cy = frame.height() / 2;
    painter.setPen(QPen(QColor(0, 168, 255, 40), 1, Qt::SolidLine));
    painter.drawEllipse(QPoint(cx, cy), 150, 150);
    painter.drawEllipse(QPoint(cx, cy), 100, 100);
    painter.drawEllipse(QPoint(cx, cy), 50, 50);

    // Center Crosshairs
    painter.setPen(QPen(QColor(0, 168, 255, 100), 1, Qt::DashLine));
    painter.drawLine(cx - 180, cy, cx + 180, cy);
    painter.drawLine(cx, cy - 180, cx, cy + 180);

    // Animate a sonar sweep line
    double rad = (m_emulationAngle * M_PI) / 180.0;
    int sx = cx + static_cast<int>(150 * cos(rad));
    int sy = cy + static_cast<int>(150 * sin(rad));
    painter.setPen(QPen(QColor(0, 210, 255, 180), 2, Qt::SolidLine));
    painter.drawLine(cx, cy, sx, sy);

    // Draw simulated contracting organ (e.g. Heart / Lung profile)
    double pulse = 1.0 + 0.12 * sin(rad * 4.0); // pulsate frequency
    painter.setBrush(QBrush(QColor(0, 168, 255, 15)));
    painter.setPen(QPen(QColor(0, 168, 255, 160), 2, Qt::SolidLine));
    
    // Vector points drawing a stylized organ outline
    QPolygon organ;
    for (int i = 0; i < 360; i += 15) {
        double r = (i * M_PI) / 180.0;
        // Heart mathematical curve: cardioid
        double distance = (80 + 15 * sin(r * 2.0)) * pulse;
        int ox = cx + static_cast<int>(distance * cos(r) * 1.1);
        int oy = cy + static_cast<int>(distance * sin(r) * 0.9) - 10;
        organ << QPoint(ox, oy);
    }
    painter.drawPolygon(organ);

    // Overlay texts
    painter.setPen(QColor(0, 210, 255));
    painter.setFont(QFont("Monospace", 9, QFont::Bold));
    painter.drawText(20, 30, "AEGIS-AI ULTRASOUND SIMULATOR");
    painter.drawText(20, 45, QString("ANGLE: %1 DEG").arg(m_emulationAngle));
    painter.drawText(20, 60, QString("SCALE: %1X").arg(pulse, 0, 'f', 2));
    
    painter.drawText(480, 30, "MODE: LIVE_SCREEN");
    painter.drawText(480, 45, "QUALITY: 94.6% (EXCELLENT)");
    painter.drawText(480, 60, "FPS: 30.0");
    
    painter.setPen(QPen(QColor(231, 76, 60, 180), 1, Qt::SolidLine));
    painter.drawRect(cx - 160, cy - 160, 320, 320);
    painter.drawText(cx - 155, cy - 145, "SCREENING FOCUS AREA");

    painter.end();
    emit frameReceived(frame);
}

void CommunicationManager::generateEmulatedSensors() {
    // Only generate telemetry every 1.5 seconds to feel natural
    static int tickCount = 0;
    tickCount++;
    if (tickCount % 45 != 0) return; 

    // Generate random values with healthy baseline fluctuations
    int hr = QRandomGenerator::global()->bounded(67, 77);
    int spo2 = QRandomGenerator::global()->bounded(97, 100);
    double temp = 98.4 + QRandomGenerator::global()->bounded(-5, 5) / 10.0;
    int sys = QRandomGenerator::global()->bounded(112, 126);
    int dia = QRandomGenerator::global()->bounded(74, 82);
    
    emit sensorDataReceived(hr, spo2, temp, sys, dia);
    emit deviceHeartbeat("ESP32-CAM", true);
    emit deviceHeartbeat("Arduino Nano", true);
}
