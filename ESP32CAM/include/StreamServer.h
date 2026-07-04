#pragma once

#include <Arduino.h>
#include <esp_http_server.h>

namespace AegisCare {
class CameraManager;
class StatusManager;
class WiFiManager;

class StreamServer final {
public:
    bool begin(CameraManager& camera, WiFiManager& wifi, StatusManager& status);
    void stop();
    bool running() const { return server_ != nullptr; }

private:
    httpd_handle_t server_ = nullptr;
    static CameraManager* camera_;
    static WiFiManager* wifi_;
    static StatusManager* status_;

    static esp_err_t streamHandler(httpd_req_t* request);
    static esp_err_t statusHandler(httpd_req_t* request);
    static esp_err_t heartbeatHandler(httpd_req_t* request);
    static esp_err_t startHandler(httpd_req_t* request);
    static esp_err_t stopHandler(httpd_req_t* request);
    static esp_err_t resolutionHandler(httpd_req_t* request);
    static esp_err_t sendJson(httpd_req_t* request, const String& body, int statusCode = 200);
};
}

