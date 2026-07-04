#include "CommunicationLogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

CommunicationLogger::CommunicationLogger() : m_file(resolveLogPath()) {
    const QFileInfo info(m_file);
    QDir().mkpath(info.absolutePath());
    m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void CommunicationLogger::logTransmit(const QByteArray& packet) {
    write(QStringLiteral("TX"), QString::fromUtf8(packet).trimmed());
}

void CommunicationLogger::logReceive(const QByteArray& packet) {
    write(QStringLiteral("RX"), QString::fromUtf8(packet).trimmed());
}

void CommunicationLogger::logConnection(const QString& message) {
    write(QStringLiteral("CONNECT"), message);
}

void CommunicationLogger::logReconnect(const QString& message) {
    write(QStringLiteral("RECONNECT"), message);
}

void CommunicationLogger::logWarning(const QString& message) {
    write(QStringLiteral("WARNING"), message);
}

void CommunicationLogger::logError(const QString& message) {
    write(QStringLiteral("ERROR"), message);
}

QString CommunicationLogger::filePath() const {
    return m_file.fileName();
}

void CommunicationLogger::write(const QString& direction, const QString& message) {
    if (!m_file.isOpen()) {
        return;
    }
    QTextStream stream(&m_file);
    stream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
           << ' ' << direction << ' ' << message << '\n';
    stream.flush();
}

QString CommunicationLogger::resolveLogPath() {
    QDir candidate(QDir::current());
    if (candidate.exists(QStringLiteral("CMakeLists.txt"))) {
        return candidate.filePath(QStringLiteral("Logs/communication.log"));
    }

    candidate = QDir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; ++depth) {
        if (candidate.exists(QStringLiteral("CMakeLists.txt"))) {
            return candidate.filePath(QStringLiteral("Logs/communication.log"));
        }
        if (!candidate.cdUp()) {
            break;
        }
    }

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(dataDir).filePath(QStringLiteral("Logs/communication.log"));
}
