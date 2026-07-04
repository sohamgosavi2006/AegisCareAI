#include "CameraManager.h"
#include "Config.h"
#include "Logger.h"

namespace AegisCare {
bool CameraManager::begin() {
    pinMode(Config::PinFlashLed, OUTPUT);
    digitalWrite(Config::PinFlashLed, LOW);

    camera_config_t config{};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Config::PinY2;
    config.pin_d1 = Config::PinY3;
    config.pin_d2 = Config::PinY4;
    config.pin_d3 = Config::PinY5;
    config.pin_d4 = Config::PinY6;
    config.pin_d5 = Config::PinY7;
    config.pin_d6 = Config::PinY8;
    config.pin_d7 = Config::PinY9;
    config.pin_xclk = Config::PinXclk;
    config.pin_pclk = Config::PinPclk;
    config.pin_vsync = Config::PinVsync;
    config.pin_href = Config::PinHref;
    config.pin_sccb_sda = Config::PinSiod;
    config.pin_sccb_scl = Config::PinSioc;
    config.pin_pwdn = Config::PinPwdn;
    config.pin_reset = Config::PinReset;
    config.xclk_freq_hz = Config::XclkFrequencyHz;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = Config::DefaultFrameSize;
    config.jpeg_quality = Config::JpegQuality;
    config.fb_count = psramFound() ? 2 : 1;
    config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    const esp_err_t result = esp_camera_init(&config);
    if (result != ESP_OK) {
        Logger::error(String("CAMERA init failed code=0x") + String(result, HEX));
        online_ = false;
        return false;
    }
    sensor_t* sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_framesize(sensor, Config::DefaultFrameSize);
        sensor->set_quality(sensor, Config::JpegQuality);
    }
    frameSize_ = Config::DefaultFrameSize;
    online_ = true;
    Logger::info(String("CAMERA Started PSRAM=") + (psramFound() ? "yes" : "no"));
    return true;
}

camera_fb_t* CameraManager::acquireFrame() {
    if (!online_) return nullptr;
    camera_fb_t* frame = esp_camera_fb_get();
    if (!frame) Logger::error("CAMERA frame allocation/capture failed");
    return frame;
}

void CameraManager::releaseFrame(camera_fb_t* frame) {
    if (frame) esp_camera_fb_return(frame);
}

String CameraManager::resolution() const {
    switch (frameSize_) {
        case FRAMESIZE_VGA: return "640x480";
        case FRAMESIZE_QVGA: return "320x240";
        default: return "custom";
    }
}

bool CameraManager::setFrameSize(framesize_t size) {
    if (size != FRAMESIZE_QVGA && size != FRAMESIZE_VGA) return false;
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor || sensor->set_framesize(sensor, size) != 0) return false;
    frameSize_ = size;
    return true;
}
}

