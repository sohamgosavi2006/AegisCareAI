#include "JsonLineProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

JsonLineProtocol::Result JsonLineProtocol::append(const QByteArray& bytes) {
    Result result;

    for (const char byte : bytes) {
        if (m_discardUntilNewline) {
            if (byte == '\n') {
                m_discardUntilNewline = false;
            }
            continue;
        }

        if (byte != '\n') {
            if (byte != '\r') {
                m_buffer.append(byte);
            }
            if (m_buffer.size() > MaxPacketBytes) {
                m_buffer.clear();
                m_discardUntilNewline = true;
                result.errors.append(QStringLiteral("Packet exceeded %1 bytes").arg(MaxPacketBytes));
            }
            continue;
        }

        const QByteArray line = m_buffer.trimmed();
        m_buffer.clear();
        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            result.errors.append(QStringLiteral("Invalid JSON: %1").arg(parseError.errorString()));
            continue;
        }
        result.objects.append(document.object());
    }

    return result;
}

void JsonLineProtocol::clear() {
    m_buffer.clear();
    m_discardUntilNewline = false;
}

QByteArray JsonLineProtocol::encode(const QJsonObject& object) {
    if (!object.contains(QStringLiteral("type"))) {
        return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
    }

    // Keep the discriminator first so constrained streaming receivers can
    // choose a parser before a large payload (such as patient_list) arrives.
    QJsonArray typeWrapper;
    typeWrapper.append(object.value(QStringLiteral("type")));
    QByteArray typeValue = QJsonDocument(typeWrapper).toJson(QJsonDocument::Compact);
    typeValue = typeValue.mid(1, typeValue.size() - 2);

    QJsonObject remainder = object;
    remainder.remove(QStringLiteral("type"));
    QByteArray trailing = QJsonDocument(remainder).toJson(QJsonDocument::Compact);
    QByteArray packet = QByteArrayLiteral("{\"type\":") + typeValue;
    if (trailing.size() > 2) {
        packet += ',';
        packet += trailing.mid(1, trailing.size() - 2);
    }
    packet += QByteArrayLiteral("}\n");
    return packet;
}
