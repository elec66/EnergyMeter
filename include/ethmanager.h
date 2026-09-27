/***************************************************************************
  Copyright (c) 2026 Thorsten Heins

  This file a part of the "ESP32-SML-Reader" source code.
    
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
   
  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

***************************************************************************/
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <LittleFS.h>  
#include <atomic>
#include "esp_log.h"
#include "config.h"
#include "secrets.h"

struct WiFiConfig
{
    const char* ssid;
    const char* password;
    const char* hostname;

    uint32_t connectTimeoutMs = 15000;
    uint32_t reconnectIntervalMs = 5000;

    bool autoReconnect = true;
    bool persistent = false;
};

inline constexpr WiFiConfig wifiConfig{
    .ssid = WIFI_SSID,
    .password = PW,
    .hostname = HOSTNAME,

    .connectTimeoutMs = 15000,
    .reconnectIntervalMs = 5000,

    .autoReconnect = true,
    .persistent = false
};

class EthManager
{
public:

    enum class State : uint8_t
    {
        Idle = 0,
        Connecting,
        Connected,
        Disconnected,
        Error
    };


public:

    explicit EthManager(const WiFiConfig& config);

    ~EthManager();

    void init();
    bool wifiBegin();
    void loop();
    void stop();

    State state() const;

    bool isConnected();
    bool isConnecting() const;
    bool hasError() const;

    IPAddress localIP() const;

    IPAddress gatewayIP() const;

    IPAddress subnetMask() const;

    IPAddress dnsIP() const;

    int32_t rssi() const;

    String ssid() const;

    String macAddress() const;

    const char* hostname() const;

    uint8_t disconnectReason() const;

    const char* disconnectReasonText() const;

    void reconnect();

    void disconnect();


private:

    static void eventHandler(
        arduino_event_id_t event,
        arduino_event_info_t info
    );

    void handleEvent(
        arduino_event_id_t event,
        arduino_event_info_t info
    );


    void startConnection();

    void handleConnecting();

    void handleDisconnected();

    static const char* disconnectReasonToString(
        uint8_t reason
    );


private:

    WiFiConfig _config;
    static constexpr const char* TAG = "LAN";

    // -------------------------------------------------------------------------
    // State
    //
    // Events are executed from a different FreeRTOS task than loop().
    // Therefore state is atomic.
    // -------------------------------------------------------------------------

    std::atomic<State> _state {
        State::Idle
    };

    std::atomic<uint8_t> _disconnectReason {
        0
    };

    uint32_t _connectStartedAt = 0;
    uint32_t _lastReconnectAttempt = 0;

    WiFiEventId_t _eventId = 0;
    bool _eventRegistered = false;
    bool _started = false;
    bool _ethConnected = false;

    static EthManager* _instance;
};