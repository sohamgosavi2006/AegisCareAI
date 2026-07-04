#pragma once

#include <Arduino.h>

namespace AegisCare {
class CameraManager;
class WiFiManager;

class StatusManager final {
public:
    void frameSent(size_t bytes);
    void setStreaming(bool streaming);
    bool streaming() const { return streaming_; }
    uint32_t frameCount() const { return frameCount_; }
    float fps() const { return fps_; }
    String json(const CameraManager& camera, const WiFiManager& wifi);

private:
    volatile bool streaming_ = false;
    uint32_t frameCount_ = 0;
    uint32_t windowFrames_ = 0;
    uint32_t windowStartedMs_ = 0;
    uint64_t bytesSent_ = 0;
    float fps_ = 0.0f;
};
}

