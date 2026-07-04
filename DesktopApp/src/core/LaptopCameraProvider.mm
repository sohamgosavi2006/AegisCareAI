#include "LaptopCameraProvider.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <QMetaObject>

@interface AegisCareCaptureDelegate : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, assign) LaptopCameraProvider* owner;
@end

@implementation AegisCareCaptureDelegate
- (void)captureOutput:(AVCaptureOutput*)output
 didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection*)connection {
    Q_UNUSED(output);
    Q_UNUSED(connection);
    CVImageBufferRef buffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!buffer || !self.owner) return;
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    const int width = int(CVPixelBufferGetWidth(buffer));
    const int height = int(CVPixelBufferGetHeight(buffer));
    const int bytesPerRow = int(CVPixelBufferGetBytesPerRow(buffer));
    const uchar* bytes = static_cast<const uchar*>(CVPixelBufferGetBaseAddress(buffer));
    QImage frame(bytes, width, height, bytesPerRow, QImage::Format_ARGB32);
    const QImage ownedFrame = frame.copy().mirrored(true, false);
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    self.owner->submitFrame(ownedFrame);
}
@end

class LaptopCameraProvider::Impl {
public:
    explicit Impl(LaptopCameraProvider* provider) : owner(provider) {
        captureQueue = dispatch_queue_create("care.aegis.laptop-camera", DISPATCH_QUEUE_SERIAL);
    }

    ~Impl() { stopAndRelease(); }

    void start() {
        if (running || starting) return;
        starting = true;
        const AVAuthorizationStatus authorization = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
        if (authorization == AVAuthorizationStatusDenied || authorization == AVAuthorizationStatusRestricted) {
            starting = false;
            owner->submitError(QStringLiteral("Laptop camera permission is disabled in macOS Privacy & Security settings."));
            return;
        }
        if (authorization == AVAuthorizationStatusNotDetermined) {
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                QMetaObject::invokeMethod(owner, [this, granted]() {
                    starting = false;
                    if (granted) start();
                    else owner->submitError(QStringLiteral("Laptop camera permission was not granted."));
                }, Qt::QueuedConnection);
            }];
            return;
        }
        configureAndStart();
    }

    void stop() {
        if (!session) {
            running = false;
            starting = false;
            return;
        }
        AVCaptureSession* sessionToStop = session;
        running = false;
        starting = false;
        dispatch_async(captureQueue, ^{
            if ([sessionToStop isRunning]) [sessionToStop stopRunning];
        });
    }

    void stopAndRelease() {
        if (session && [session isRunning]) [session stopRunning];
        [output setSampleBufferDelegate:nil queue:nullptr];
        delegate.owner = nullptr;
        session = nil;
        input = nil;
        output = nil;
        delegate = nil;
        running = false;
    }

    LaptopCameraProvider* owner = nullptr;
    AVCaptureSession* session = nil;
    AVCaptureDeviceInput* input = nil;
    AVCaptureVideoDataOutput* output = nil;
    AegisCareCaptureDelegate* delegate = nil;
    dispatch_queue_t captureQueue = nullptr;
    bool running = false;
    bool starting = false;

private:
    void configureAndStart() {
        AVCaptureDevice* device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        if (!device) {
            starting = false;
            owner->submitError(QStringLiteral("No laptop webcam was detected."));
            return;
        }
        NSError* error = nil;
        input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
        if (!input || error) {
            starting = false;
            owner->submitError(QString::fromNSString(error.localizedDescription));
            return;
        }
        session = [[AVCaptureSession alloc] init];
        [session beginConfiguration];
        if ([session canSetSessionPreset:AVCaptureSessionPreset640x480]) session.sessionPreset = AVCaptureSessionPreset640x480;
        if ([session canAddInput:input]) [session addInput:input];
        output = [[AVCaptureVideoDataOutput alloc] init];
        output.alwaysDiscardsLateVideoFrames = YES;
        output.videoSettings = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
        delegate = [[AegisCareCaptureDelegate alloc] init];
        delegate.owner = owner;
        [output setSampleBufferDelegate:delegate queue:captureQueue];
        if ([session canAddOutput:output]) [session addOutput:output];
        [session commitConfiguration];
        AVCaptureSession* sessionToStart = session;
        dispatch_async(captureQueue, ^{
            [sessionToStart startRunning];
            QMetaObject::invokeMethod(owner, [this]() {
                starting = false;
                running = session && [session isRunning];
                emit owner->availabilityChanged(running);
                if (!running) owner->submitError(QStringLiteral("Laptop camera could not start."));
            }, Qt::QueuedConnection);
        });
    }
};

LaptopCameraProvider::LaptopCameraProvider(QObject* parent)
    : CameraProvider(parent), m_impl(new Impl(this)) {}

LaptopCameraProvider::~LaptopCameraProvider() { delete m_impl; }
void LaptopCameraProvider::start() { m_impl->start(); }
void LaptopCameraProvider::stop() {
    m_impl->stop();
    emit availabilityChanged(false);
}
bool LaptopCameraProvider::isRunning() const { return m_impl->running; }
bool LaptopCameraProvider::isAvailable() const { return m_impl->running; }

void LaptopCameraProvider::submitFrame(const QImage& frame) {
    if (frame.isNull()) return;
    QMetaObject::invokeMethod(this, [this, frame]() {
        if (m_impl->running) emit frameReady(frame);
    }, Qt::QueuedConnection);
}

void LaptopCameraProvider::submitError(const QString& message) {
    QMetaObject::invokeMethod(this, [this, message]() {
        emit availabilityChanged(false);
        emit providerError(message);
    }, Qt::QueuedConnection);
}

