#include "StatusManager.h"
#include "CameraManager.h"
#include "Config.h"
#include "WiFiManager.h"

namespace AegisCare {
void StatusManager::frameSent(size_t bytes) {
    ++frameCount_;
    ++windowFrames_;
    bytesSent_ += bytes;
    const uint32_t now = millis();
    if (windowStartedMs_ == 0) windowStartedMs_ = now;
    if (now - windowStartedMs_ >= 1000) {
        fps_ = windowFrames_ * 1000.0f / (now - windowStartedMs_);
        windowFrames_ = 0;
        windowStartedMs_ = now;
    }
}

void StatusManager::setStreaming(bool streaming) { streaming_ = streaming; }

String StatusManager::json(const CameraManager& camera, const WiFiManager& wifi) {
    String result;
    result.reserve(384);
    result += "{\"device\":\"esp32_cam\",\"educational_demo\":true";
    result += ",\"camera_online\":" + String(camera.online() ? "true" : "false");
    result += ",\"wifi_connected\":" + String(wifi.connected() ? "true" : "false");
    result += ",\"wifi_state\":\"" + wifi.stateText() + "\"";
    result += ",\"wifi_rssi\":" + String(wifi.rssi());
    result += ",\"ip\":\"" + wifi.ip().toString() + "\"";
    result += ",\"fps\":" + String(fps_, 1);
    result += ",\"frame_count\":" + String(frameCount_);
    result += ",\"streaming\":" + String(streaming_ ? "true" : "false");
    result += ",\"resolution\":\"" + camera.resolution() + "\"";
    result += ",\"uptime_ms\":" + String(millis());
    result += ",\"firmware\":\"" + String(Config::FirmwareVersion) + "\"";
    result += ",\"free_heap\":" + String(ESP.getFreeHeap());
    result += ",\"bytes_sent\":" + String(static_cast<uint32_t>(bytesSent_)) + "}";
    return result;
}
}

