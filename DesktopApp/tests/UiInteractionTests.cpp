#include "../src/ui/components/DigitalTwinWidget.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>

namespace {

void waitForEvents(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    DigitalTwinWidget widget;
    widget.resize(480, 640);
    widget.show();

    // The former DeleteWhenStopped bug left a dangling zoom pointer after
    // 600 ms. Waiting past completion before reset reproduces that path.
    widget.zoomToRegion(QStringLiteral("Heart"));
    waitForEvents(720);
    widget.resetZoom();
    waitForEvents(620);

    // Repeated calls must retarget the same valid animation objects.
    for (int iteration = 0; iteration < 40; ++iteration) {
        widget.zoomToRegion(iteration % 2 == 0 ? QStringLiteral("Brain") : QStringLiteral("Lungs"));
        widget.resetZoom();
        widget.triggerRotation();
        app.processEvents();
    }

    // The old rotation object self-deleted before this second invocation.
    waitForEvents(1900);
    widget.triggerRotation();
    waitForEvents(100);
    return 0;
}
