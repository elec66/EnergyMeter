#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <config.h>
#include <atomic>
#include <cstdint>

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

class WiFiManager
{
public:

    // -------------------------------------------------------------------------
    // Connection state
    // -------------------------------------------------------------------------

    enum class State : uint8_t
    {
        Idle = 0,
        Connecting,
        Connected,
        Disconnected,
        Error
    };


public:

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    explicit WiFiManager(const WiFiConfig& config);

    ~WiFiManager();


    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    bool begin();

    void loop();

    void stop();


    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    State state() const;

    bool isConnected() const;

    bool isConnecting() const;

    bool hasError() const;


    // -------------------------------------------------------------------------
    // Network information
    // -------------------------------------------------------------------------

    IPAddress localIP() const;

    IPAddress gatewayIP() const;

    IPAddress subnetMask() const;

    IPAddress dnsIP() const;

    int32_t rssi() const;

    String ssid() const;

    String macAddress() const;

    const char* hostname() const;


    // -------------------------------------------------------------------------
    // Diagnostic information
    // -------------------------------------------------------------------------

    uint8_t disconnectReason() const;

    const char* disconnectReasonText() const;


    // -------------------------------------------------------------------------
    // Manual control
    // -------------------------------------------------------------------------

    void reconnect();

    void disconnect();


private:

    // -------------------------------------------------------------------------
    // WiFi event handling
    // -------------------------------------------------------------------------

    static void eventHandler(
        arduino_event_id_t event,
        arduino_event_info_t info
    );

    void handleEvent(
        arduino_event_id_t event,
        arduino_event_info_t info
    );


    // -------------------------------------------------------------------------
    // Connection handling
    // -------------------------------------------------------------------------

    void startConnection();

    void handleConnecting();

    void handleDisconnected();


    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    static const char* disconnectReasonToString(
        uint8_t reason
    );


private:

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    WiFiConfig _config;


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


    // -------------------------------------------------------------------------
    // Timing
    // -------------------------------------------------------------------------

    uint32_t _connectStartedAt = 0;

    uint32_t _lastReconnectAttempt = 0;


    // -------------------------------------------------------------------------
    // Event registration
    // -------------------------------------------------------------------------

    WiFiEventId_t _eventId = 0;

    bool _eventRegistered = false;

    bool _started = false;


    // -------------------------------------------------------------------------
    // Singleton instance
    //
    // WiFi.onEvent() requires a callback that does not contain a user object.
    // The static instance forwards the event to the active WiFiManager.
    // -------------------------------------------------------------------------

    static WiFiManager* _instance;
};