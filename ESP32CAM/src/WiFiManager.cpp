#include "WiFiManager.h"
#include "Config.h"
#include "Logger.h"

#include <ESPmDNS.h>
#include <esp_wifi.h>

namespace AegisCare {
bool WiFiManager::begin() {
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_11dBm);
    if (strlen(Config::StationSsid) > 0 && connectStation()) return true;
    return startAccessPoint();
}

bool WiFiManager::connectStation() {
    state_ = State::Connecting;
    Logger::info(String("WIFI Connecting to ") + Config::StationSsid);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(Config::Hostname);
    WiFi.begin(Config::StationSsid, Config::StationPassword);
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < Config::WifiConnectTimeoutMs) {
        delay(100);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning("WIFI station timeout; enabling access-point fallback");
        WiFi.disconnect(true, false);
        return false;
    }
    state_ = State::StationConnected;
    reconnectDelayMs_ = 1000;
    MDNS.begin(Config::Hostname);
    MDNS.addService("http", "tcp", Config::HttpPort);
    Logger::info(String("WIFI Connected IP=") + WiFi.localIP().toString());
    return true;
}

bool WiFiManager::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(Config::AccessPointSsid, Config::AccessPointPassword)) {
        state_ = State::Disconnected;
        Logger::error("WIFI access point failed");
        scheduleReconnect();
        return false;
    }
    state_ = State::AccessPoint;
    MDNS.begin(Config::Hostname);
    MDNS.addService("http", "tcp", Config::HttpPort);
    Logger::info(String("WIFI AP Ready IP=") + WiFi.softAPIP().toString());
    return true;
}

void WiFiManager::update() {
    const uint32_t now = millis();
    if (now - lastHealthCheckMs_ < Config::WifiHealthIntervalMs) return;
    lastHealthCheckMs_ = now;
    if (state_ == State::StationConnected && WiFi.status() != WL_CONNECTED) {
        state_ = State::Disconnected;
        Logger::warning("WIFI disconnected");
        scheduleReconnect();
    }
    if (state_ == State::Disconnected && static_cast<int32_t>(now - reconnectAtMs_) >= 0) {
        Logger::info("WIFI reconnect attempt");
        if (!connectStation()) startAccessPoint();
    }
}

void WiFiManager::scheduleReconnect() {
    reconnectAtMs_ = millis() + reconnectDelayMs_;
    reconnectDelayMs_ = min(reconnectDelayMs_ * 2, Config::WifiReconnectMaximumMs);
}

bool WiFiManager::connected() const {
    return state_ == State::StationConnected || state_ == State::AccessPoint;
}

IPAddress WiFiManager::ip() const {
    return state_ == State::AccessPoint ? WiFi.softAPIP() : WiFi.localIP();
}

int32_t WiFiManager::rssi() const {
    return state_ == State::StationConnected ? WiFi.RSSI() : 0;
}

String WiFiManager::stateText() const {
    switch (state_) {
        case State::Connecting: return "connecting";
        case State::StationConnected: return "connected";
        case State::AccessPoint: return "access_point";
        case State::Disconnected: return "disconnected";
    }
    return "disconnected";
}
}

