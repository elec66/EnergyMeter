#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <ETH.h>
#include <WiFi.h>
#include "config.h"
#include "secrets.h"

enum class NetworkModus {
  NONE,
  ETH,
  WIFI
};

struct ETHManagerConfig {
  const char* ssid;
  const char* password;
  const char* hostname;
  int csPin = W5500_CS_PIN;
  int rstPin = W5500_RST_PIN;
  int intPin = W5500_INT_PIN;
  int sclkPin = W5500_SCK_PIN;
  int misoPin = W5500_MISO_PIN;
  int mosiPin = W5500_MOSI_PIN;
  unsigned long ethTimeoutMs = 5000;
  unsigned long wifiTimeoutMs = 15000;
  unsigned long reconnectIntervalMs = 5000; // Intervall für Reconnect-Versuche in loop()
};

inline constexpr ETHManagerConfig wifiConfig{
    .ssid = WIFI_SSID,
    .password = WIFI_PW,
    .hostname = HOSTNAME,
    .ethTimeoutMs = 15000,
    .reconnectIntervalMs = 5000,
};



class ETHManager {
public:
  explicit ETHManager(const ETHManagerConfig& config);
  
  // Initialisiert Harware & versucht Erstverbindung
  bool init();

  // MUSS regelmäßig in der Hauptschleife (loop()) aufgerufen werden!
  void tick();
  
  // Statusabfragen
  bool isConnected() const;
  NetworkModus getActiveInterface() const;
  IPAddress getLocalIP() const;

private:
  ETHManagerConfig _config;
  NetworkModus _activeInterface = NetworkModus::NONE;
  static constexpr const char* TAG = "ETH";
  
  static bool _ethConnected;
  bool _ethHardwareInitialized = false;
  unsigned long _lastReconnectAttempt = 0;

  // Event-Callbacks
  static void onETHConnected(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onETHGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onETHDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onWIFIConnected(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onWIFIGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

  bool initEthernetHardware();
  bool connectEthernetBlocking();
  bool startWiFiConnection();
  void disconnectWiFi();
};

