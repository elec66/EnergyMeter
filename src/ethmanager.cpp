/***************************************************************************
  ETH Manager:

  The ETH Manager organizes all ETH and WIFI functions and connections.

***************************************************************************/
#include "ethmanager.h"

bool ETHManager::_ethConnected = false;

ETHManager::ETHManager(const ETHManagerConfig& config): _config(config) {
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

void ETHManager::onETHConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGD(TAG, "ETH cable connected / link up");
}

void ETHManager::onWIFIConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGD(TAG, "WIFI connected / link up");
}

void ETHManager::onETHGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGD(TAG, "IP-address: %s", ETH.localIP().toString().c_str());
  _ethConnected = true;
}

void ETHManager::onWIFIGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGD(TAG, "IP-address: %s", WiFi.localIP().toString().c_str());
}

void ETHManager::onETHDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGE(TAG, "ETH deconnected / link down");
  _ethConnected = false;
}

bool ETHManager::init() {
  // Register callbacks
  WiFi.onEvent(onETHConnected, WiFiEvent_t::ARDUINO_EVENT_ETH_CONNECTED);
  WiFi.onEvent(onWIFIConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onETHGotIP, WiFiEvent_t::ARDUINO_EVENT_ETH_GOT_IP);
  WiFi.onEvent(onETHDisconnected, WiFiEvent_t::ARDUINO_EVENT_ETH_DISCONNECTED);
  WiFi.onEvent(onWIFIGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  // Start SPI + W5500 hardware
  _ethHardwareInitialized = initEthernetHardware();

  // Start first connection (first ETH, then WIFI)
  if (_ethHardwareInitialized && connectEthernetBlocking()) {
    _activeInterface = NetworkModus::ETH;
    ESP_LOGD(TAG, "ETH connected");
    return true;
  }

  if (startWiFiConnection()) {
    _activeInterface = NetworkModus::WIFI;
    return true;
  }

  _activeInterface = NetworkModus::NONE;
  return false;
}


bool ETHManager::initEthernetHardware() {
  SPI.begin(_config.sclkPin, _config.misoPin, _config.mosiPin, _config.csPin);

  bool ethInit = ETH.begin(ETH_PHY_W5500, 1, _config.csPin, _config.intPin, _config.rstPin, SPI);
  if (!ethInit) {
    ESP_LOGD(TAG, "W5500 Initialization failed");
  } else {
    ESP_LOGD(TAG, "W5500 Initialization passed");
  }
  return ethInit;
}


bool ETHManager::connectEthernetBlocking() {
  ESP_LOGD(TAG, "Warte auf Ethernet DHCP...");
  unsigned long start = millis();

  while (!_ethConnected && (millis() - start < _config.ethTimeoutMs)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  return _ethConnected;
}


bool ETHManager::startWiFiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(_config.ssid, _config.password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < _config.wifiTimeoutMs)) {
    delay(500);
    Serial.print(".");
  }
  ESP_LOGD(TAG, "WIFI connected");

  return WiFi.status() == WL_CONNECTED;
}


void ETHManager::disconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() != WIFI_OFF) {
    ESP_LOGD(TAG, "WIFI disconnected (ETH has priority)...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}


void ETHManager::tick() {
  // Scenario 1: Ethernet is active / connected (hot-plug or continuous operation)
  if (_ethConnected) {
    if (_activeInterface != NetworkModus::ETH) {
      ESP_LOGI(TAG, "Ethernet aktiv! Wechsel zu ETHERNET.");
      disconnectWiFi();
      _activeInterface = NetworkModus::ETH;
    }
    return; 
  }

  // Scenario 2: Connected via Ethernet, but cable was unplugged / Link DOWN
  if (_activeInterface == NetworkModus::ETH && !_ethConnected) {
    ESP_LOGW(TAG, "Lost ethernet! Switch to WIFI...");
    _activeInterface = NetworkModus::NONE;
  }

  // Scenario 3: No Ethernet available, but Wi-Fi is (or was) active.
  if (_activeInterface == NetworkModus::WIFI) {
    if (WiFi.status() == WL_CONNECTED) {
      return; // WLAN läuft stabil
    } else {
      Serial.println("[ETHManager] WLAN-Verbindung verloren!");
      _activeInterface = NetworkModus::NONE;
    }
  }

  // Scenario 4: No interface active -> Periodic reconnection attempt
  unsigned long now = millis();
  if (_activeInterface == NetworkModus::NONE && (now - _lastReconnectAttempt >= _config.reconnectIntervalMs)) {
    _lastReconnectAttempt = now;
    ESP_LOGD(TAG, "Attempting to re-establish connection...");

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(_config.ssid, _config.password);
    }

    if (WiFi.status() == WL_CONNECTED) {
      ESP_LOGD(TAG, "WIFI reconnection passed successfully!");
      _activeInterface = NetworkModus::WIFI;
    }
  }
}

bool ETHManager::isConnected() const {
  return _activeInterface != NetworkModus::NONE;
}

NetworkModus ETHManager::getActiveInterface() const {
  return _activeInterface;
}

IPAddress ETHManager::getLocalIP() const {
  if (_activeInterface == NetworkModus::ETH) {
    return ETH.localIP();
  } else if (_activeInterface == NetworkModus::WIFI) {
    return WiFi.localIP();
  }
  return IPAddress(0, 0, 0, 0);
}