#ifndef JSONLINEPROTOCOL_H
#define JSONLINEPROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

/**
 * Incrementally frames newline-delimited JSON received from a serial stream.
 * Oversized and malformed packets are rejected without affecting later lines.
 */
class JsonLineProtocol {
public:
    static constexpr qsizetype MaxPacketBytes = 512;

    struct Result {
        QList<QJsonObject> objects;
        QStringList errors;
    };

    Result append(const QByteArray& bytes);
    void clear();

    static QByteArray encode(const QJsonObject& object);

private:
    QByteArray m_buffer;
    bool m_discardUntilNewline = false;
};

#endif // JSONLINEPROTOCOL_H
