#include "LaptopCameraProvider.h"

#ifndef Q_OS_MACOS
class LaptopCameraProvider::Impl { public: bool running = false; };
LaptopCameraProvider::LaptopCameraProvider(QObject* parent) : CameraProvider(parent), m_impl(new Impl) {}
LaptopCameraProvider::~LaptopCameraProvider() { delete m_impl; }
void LaptopCameraProvider::start() { emit providerError(QStringLiteral("Laptop camera provider is unavailable on this platform build.")); }
void LaptopCameraProvider::stop() { m_impl->running = false; }
bool LaptopCameraProvider::isRunning() const { return false; }
bool LaptopCameraProvider::isAvailable() const { return false; }
void LaptopCameraProvider::submitFrame(const QImage&) {}
void LaptopCameraProvider::submitError(const QString& message) { emit providerError(message); }
#endif
