#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace AegisCare {
class WiFiManager final {
public:
    enum class State { Disconnected, Connecting, StationConnected, AccessPoint };

    bool begin();
    void update();
    bool connected() const;
    bool stationMode() const { return state_ == State::StationConnected; }
    State state() const { return state_; }
    IPAddress ip() const;
    int32_t rssi() const;
    String stateText() const;

private:
    State state_ = State::Disconnected;
    uint32_t lastHealthCheckMs_ = 0;
    uint32_t reconnectAtMs_ = 0;
    uint32_t reconnectDelayMs_ = 1000;

    bool connectStation();
    bool startAccessPoint();
    void scheduleReconnect();
};
}

