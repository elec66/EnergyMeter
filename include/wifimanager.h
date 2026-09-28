#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <cstdint>
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

class WiFiManager
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

  explicit WiFiManager(const WiFiConfig& config);
  ~WiFiManager();

  bool begin();
  void loop();
  void stop();
  State state() const;
  bool isConnected() const;
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

  std::atomic<State> _state {
    State::Idle
  };

  std::atomic<uint8_t> _disconnectReason {
    0
  };

  uint32_t _connectStartedAt = 0;
  uint32_t _lastReconnectAttempt = 0;
  static constexpr const char* TAG = "WIFI";
  WiFiEventId_t _eventId = 0;

  bool _eventRegistered = false;
  bool _started = false;

  static WiFiManager* _instance;
};