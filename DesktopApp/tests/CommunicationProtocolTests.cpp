#include "../src/protocol/JsonLineProtocol.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>

namespace {

bool require(bool condition, const char* message) {
    if (!condition) qCritical() << message;
    return condition;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    bool passed = true;

    JsonLineProtocol protocol;
    auto partial = protocol.append("{\"type\":\"heart");
    passed &= require(partial.objects.isEmpty(), "Partial JSON must not be parsed");
    partial = protocol.append("beat\",\"value\":\"PONG\"}\n");
    passed &= require(partial.objects.size() == 1, "Completed partial packet must parse once");

    auto malformed = protocol.append("{broken}\n{\"type\":\"pong\"}\n");
    passed &= require(malformed.errors.size() == 1, "Malformed packet must be reported");
    passed &= require(malformed.objects.size() == 1, "Packet after malformed JSON must survive");

    QByteArray oversized(JsonLineProtocol::MaxPacketBytes + 20, 'x');
    auto overflow = protocol.append(oversized + "\n{\"type\":\"pong\"}\n");
    passed &= require(overflow.errors.size() == 1, "Oversized packet must be rejected");
    passed &= require(overflow.objects.size() == 1, "Parser must recover after oversized packet");

    const QByteArray encoded = JsonLineProtocol::encode(
        QJsonObject{{QStringLiteral("patients"), QJsonArray{}},
                    {QStringLiteral("type"), QStringLiteral("patient_list")},
                    {QStringLiteral("total"), 0}});
    passed &= require(encoded.startsWith("{\"type\":\"patient_list\""),
                      "Type discriminator must be encoded first");
    passed &= require(encoded.endsWith('\n'), "Encoded packets must end in newline");

    return passed ? 0 : 1;
}
