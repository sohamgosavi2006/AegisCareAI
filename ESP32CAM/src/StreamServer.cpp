#include "StreamServer.h"
#include "CameraManager.h"
#include "Config.h"
#include "Logger.h"
#include "StatusManager.h"
#include "WiFiManager.h"

namespace AegisCare {
namespace {
constexpr char Boundary[] = "aegiscareframe";
constexpr char ContentType[] = "multipart/x-mixed-replace;boundary=aegiscareframe";
constexpr char BoundaryLine[] = "\r\n--aegiscareframe\r\n";
constexpr char PartHeader[] = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
}

CameraManager* StreamServer::camera_ = nullptr;
WiFiManager* StreamServer::wifi_ = nullptr;
StatusManager* StreamServer::status_ = nullptr;

bool StreamServer::begin(CameraManager& camera, WiFiManager& wifi, StatusManager& status) {
    if (server_) return true;
    camera_ = &camera;
    wifi_ = &wifi;
    status_ = &status;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = Config::HttpPort;
    config.ctrl_port = 32769;
    config.max_open_sockets = 5;
    config.recv_wait_timeout = 8;
    config.send_wait_timeout = 8;
    config.stack_size = 8192;
    if (httpd_start(&server_, &config) != ESP_OK) {
        Logger::error("STREAM server start failed");
        server_ = nullptr;
        return false;
    }

    const httpd_uri_t routes[] = {
        {.uri = "/stream", .method = HTTP_GET, .handler = streamHandler, .user_ctx = nullptr},
        {.uri = "/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = nullptr},
        {.uri = "/heartbeat", .method = HTTP_GET, .handler = heartbeatHandler, .user_ctx = nullptr},
        {.uri = "/stream/start", .method = HTTP_GET, .handler = startHandler, .user_ctx = nullptr},
        {.uri = "/stream/stop", .method = HTTP_GET, .handler = stopHandler, .user_ctx = nullptr},
        {.uri = "/resolution", .method = HTTP_GET, .handler = resolutionHandler, .user_ctx = nullptr},
    };
    for (const auto& route : routes) httpd_register_uri_handler(server_, &route);
    status_->setStreaming(true);
    Logger::info(String("STREAM Server ready http://") + wifi.ip().toString() + ":" + Config::HttpPort + "/stream");
    return true;
}

void StreamServer::stop() {
    if (!server_) return;
    httpd_stop(server_);
    server_ = nullptr;
    if (status_) status_->setStreaming(false);
    Logger::info("STREAM Server stopped");
}

esp_err_t StreamServer::streamHandler(httpd_req_t* request) {
    if (!camera_ || !camera_->online() || !status_ || !status_->streaming()) {
        return sendJson(request, "{\"error\":\"camera_offline_or_stream_stopped\"}", 503);
    }
    Logger::info(String("CLIENT Connected ") + httpd_req_to_sockfd(request));
    httpd_resp_set_type(request, ContentType);
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    esp_err_t result = ESP_OK;
    uint32_t lastFrameMs = 0;
    while (status_->streaming()) {
        const uint32_t now = millis();
        if (now - lastFrameMs < Config::MinimumFrameIntervalMs) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        lastFrameMs = now;
        camera_fb_t* frame = camera_->acquireFrame();
        if (!frame) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }
        char header[96];
        const int headerLength = snprintf(header, sizeof(header), PartHeader, frame->len);
        result = httpd_resp_send_chunk(request, BoundaryLine, sizeof(BoundaryLine) - 1);
        if (result == ESP_OK) result = httpd_resp_send_chunk(request, header, headerLength);
        if (result == ESP_OK) result = httpd_resp_send_chunk(request, reinterpret_cast<const char*>(frame->buf), frame->len);
        const size_t bytes = frame->len;
        camera_->releaseFrame(frame);
        if (result != ESP_OK) break;
        status_->frameSent(bytes);
    }
    Logger::info("STREAM Client disconnected");
    return result;
}

esp_err_t StreamServer::statusHandler(httpd_req_t* request) {
    return sendJson(request, status_->json(*camera_, *wifi_));
}

esp_err_t StreamServer::heartbeatHandler(httpd_req_t* request) {
    return sendJson(request, "{\"type\":\"heartbeat\",\"status\":\"ok\"}");
}

esp_err_t StreamServer::startHandler(httpd_req_t* request) {
    status_->setStreaming(true);
    Logger::info("STREAM Enabled");
    return sendJson(request, "{\"streaming\":true}");
}

esp_err_t StreamServer::stopHandler(httpd_req_t* request) {
    status_->setStreaming(false);
    Logger::info("STREAM Disabled");
    return sendJson(request, "{\"streaming\":false}");
}

esp_err_t StreamServer::resolutionHandler(httpd_req_t* request) {
    char query[32]{};
    char value[12]{};
    if (httpd_req_get_url_query_str(request, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "value", value, sizeof(value)) != ESP_OK) {
        return sendJson(request, "{\"error\":\"use value=qvga or value=vga\"}", 400);
    }
    const framesize_t size = strcmp(value, "vga") == 0 ? FRAMESIZE_VGA : FRAMESIZE_QVGA;
    if (!camera_->setFrameSize(size)) return sendJson(request, "{\"error\":\"resolution_change_failed\"}", 500);
    return sendJson(request, String("{\"resolution\":\"") + camera_->resolution() + "\"}");
}

esp_err_t StreamServer::sendJson(httpd_req_t* request, const String& body, int statusCode) {
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    if (statusCode == 400) httpd_resp_set_status(request, "400 Bad Request");
    else if (statusCode == 500) httpd_resp_set_status(request, "500 Internal Server Error");
    else if (statusCode == 503) httpd_resp_set_status(request, "503 Service Unavailable");
    return httpd_resp_send(request, body.c_str(), body.length());
}
}

