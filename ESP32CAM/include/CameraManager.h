#pragma once

#include <Arduino.h>
#include <esp_camera.h>

namespace AegisCare {
class CameraManager final {
public:
    bool begin();
    bool online() const { return online_; }
    camera_fb_t* acquireFrame();
    void releaseFrame(camera_fb_t* frame);
    String resolution() const;
    bool setFrameSize(framesize_t size);

private:
    bool online_ = false;
    framesize_t frameSize_ = FRAMESIZE_QVGA;
};
}

