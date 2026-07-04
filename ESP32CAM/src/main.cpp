#include <Arduino.h>

#include "CameraManager.h"
#include "Logger.h"
#include "StatusManager.h"
#include "StreamServer.h"
#include "WiFiManager.h"

namespace {
AegisCare::CameraManager camera;
AegisCare::WiFiManager wifi;
AegisCare::StatusManager status;
AegisCare::StreamServer server;
uint32_t cameraRetryAtMs = 0;
}

void setup() {
    AegisCare::Logger::begin();
    const bool cameraReady = camera.begin();
    const bool networkReady = wifi.begin();
    if (cameraReady && networkReady) server.begin(camera, wifi, status);
    if (!cameraReady) cameraRetryAtMs = millis() + 10000;
}

void loop() {
    wifi.update();
    if (!camera.online() && static_cast<int32_t>(millis() - cameraRetryAtMs) >= 0) {
        // Retry at a bounded rate; never enter a reboot loop.
        if (!camera.begin()) cameraRetryAtMs = millis() + 10000;
    }
    if (camera.online() && wifi.connected() && !server.running()) server.begin(camera, wifi, status);
    delay(20);
}

