#ifndef LAPTOPCAMERAPROVIDER_H
#define LAPTOPCAMERAPROVIDER_H

#include "CameraProvider.h"

class LaptopCameraProvider final : public CameraProvider {
    Q_OBJECT
public:
    explicit LaptopCameraProvider(QObject* parent = nullptr);
    ~LaptopCameraProvider() override;

    void start() override;
    void stop() override;
    bool isRunning() const override;
    bool isAvailable() const override;
    Source source() const override { return Source::Laptop; }
    QString displayName() const override { return QStringLiteral("Laptop Camera"); }

    // Called by the native capture delegate; queued back to the Qt thread.
    void submitFrame(const QImage& frame);
    void submitError(const QString& message);

private:
    class Impl;
    Impl* m_impl = nullptr;
};

#endif
