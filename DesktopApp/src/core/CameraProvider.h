#ifndef CAMERAPROVIDER_H
#define CAMERAPROVIDER_H

#include <QImage>
#include <QObject>
#include <QString>

class CameraProvider : public QObject {
    Q_OBJECT
public:
    enum class Source { None, Esp32, Laptop };
    Q_ENUM(Source)

    explicit CameraProvider(QObject* parent = nullptr) : QObject(parent) {}
    ~CameraProvider() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual bool isAvailable() const = 0;
    virtual Source source() const = 0;
    virtual QString displayName() const = 0;

signals:
    void frameReady(const QImage& frame);
    void availabilityChanged(bool available);
    void providerError(const QString& message);
};

#endif
