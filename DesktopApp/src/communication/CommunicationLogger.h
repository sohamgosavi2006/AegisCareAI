#ifndef COMMUNICATIONLOGGER_H
#define COMMUNICATIONLOGGER_H

#include <QFile>
#include <QString>

/** Writes timestamped serial traffic to Logs/communication.log. */
class CommunicationLogger {
public:
    CommunicationLogger();

    void logTransmit(const QByteArray& packet);
    void logReceive(const QByteArray& packet);
    void logConnection(const QString& message);
    void logReconnect(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);
    QString filePath() const;

private:
    void write(const QString& direction, const QString& message);
    static QString resolveLogPath();

    QFile m_file;
};

#endif // COMMUNICATIONLOGGER_H
