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

#include "ethmanager.h"
#include <cstring>

// =============================================================================
// Static instance
// =============================================================================

EthManager* EthManager::_instance = nullptr;


// =============================================================================
// Constructor
// =============================================================================
EthManager::EthManager(const WiFiConfig& config):_config(config){
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  _instance = this;
}

// =============================================================================
// Destructor
// =============================================================================
EthManager::~EthManager(){
  stop();
  if (_instance == this){
    _instance = nullptr;
  }
}

// =============================================================================
// Set dafault values
// =============================================================================
void EthManager::init() {
  if (!LittleFS.begin(true)) {
    ESP_LOGE(TAG, "LittleFS not mounted");
    return;
  } else {
    ESP_LOGD(TAG, "LittleFS mounted");
  }
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.disconnect(true, true);
  if (!_eventRegistered){
      _eventId = WiFi.onEvent(EthManager::eventHandler);
      _eventRegistered = true;
  }
  if (!ETH.begin(ETH_PHY_W5500,
                 1,
                 W5500_CS_PIN,
                 W5500_INT_PIN,
                 W5500_RST_PIN,
                 SPI2_HOST,
                 W5500_SCK_PIN,
                 W5500_MISO_PIN,
                 W5500_MOSI_PIN)) {
    ESP_LOGE(TAG, "W5500 chip connection failed");
  } else {
    ESP_LOGD(TAG, "W5500 chip connected successfullly");
  }
}

// =============================================================================
// begin()
// =============================================================================

bool EthManager::wifiBegin()
{
    if (_started){
      return true;
    }

    // Validate configuration
    if (_config.ssid == nullptr || strlen(_config.ssid) == 0){
        ESP_LOGE(TAG, "SSID is empty");
        _state.store(State::Error, std::memory_order_release);
        return false;
    }

    if (_config.hostname == nullptr || strlen(_config.hostname) == 0){
      ESP_LOGE(TAG, "hostname is empty");
      _state.store(State::Error, std::memory_order_release);
      return false;
    }

    ESP_LOGD(TAG,"SSID: %s", _config.ssid );
    ESP_LOGD(TAG, "Hostname : %s", _config.hostname);

    // IMPORTANT:
    // Espressif requires setHostname() before WiFi is started.
    // Therefore do this BEFORE WiFi.mode().
    WiFi.setHostname(_config.hostname);

    // Persistent configuration    //
    // false prevents unnecessary writes to NVS.
    WiFi.persistent(_config.persistent);

    // Station mode
    WiFi.mode(WIFI_STA);

    // Automatic reconnect
    // We deliberately manage reconnects ourselves so that the connection
    // state machine has a single owner.
    WiFi.setAutoReconnect(false);


    // -------------------------------------------------------------------------
    // Register event callback
    // -------------------------------------------------------------------------

    if (!_eventRegistered){
      _eventId = WiFi.onEvent(EthManager::eventHandler);
      _eventRegistered = true;
    }

    _started = true;
    startConnection();
    return true;
}


// =============================================================================
// stop()
// =============================================================================
void EthManager::stop(){
    if (!_started){
      return;
    }
    ESP_LOGD(TAG, "Wifi stopping");

    // Remove event callback
    // This must NOT be called from inside the event callback.
    // We are executing from the normal application context here.
    if (_eventRegistered){
      WiFi.removeEvent(_eventId);
      _eventRegistered = false;
      _eventId = 0;
    }
    WiFi.disconnect(true, false);
    _state.store(State::Idle, std::memory_order_release);
    _started = false;
}


// =============================================================================
// loop()
// =============================================================================

void EthManager::loop()
{
    if (!_started)
    {
        return;
    }


    const State currentState =
        _state.load(
            std::memory_order_acquire
        );


    switch (currentState)
    {
        // ---------------------------------------------------------------------
        case State::Idle:
        {
            startConnection();

            break;
        }


        // ---------------------------------------------------------------------
        case State::Connecting:
        {
            handleConnecting();

            break;
        }


        // ---------------------------------------------------------------------
        case State::Connected:
        {
            // Normally this is handled by WiFi events.
            //
            // This additional check provides a safety net.

            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println(
                    "[WiFi] Connection lost "
                    "(status check)"
                );

                _state.store(
                    State::Disconnected,
                    std::memory_order_release
                );

                _lastReconnectAttempt = millis();
            }

            break;
        }


        // ---------------------------------------------------------------------
        case State::Disconnected:
        {
            handleDisconnected();

            break;
        }


        // ---------------------------------------------------------------------
        case State::Error:
        {
            // Permanent error state.
            //
            // A manual reconnect() can leave this state.

            break;
        }
    }
}


// =============================================================================
// startConnection()
// =============================================================================
void EthManager::startConnection()
{
    ESP_LOGD(TAG, "Connecting to: %s", _config.ssid);
    _state.store(State::Connecting, std::memory_order_release);
    _connectStartedAt = millis();

    // Clear previous disconnect reason
    _disconnectReason.store(0, std::memory_order_release);

    // Start WiFi connection
    const wl_status_t result = WiFi.begin(_config.ssid, _config.password);

    if (result == WL_NO_SSID_AVAIL){
        ESP_LOGE(TAG, "SSID not available");
    }
}


// =============================================================================
// handleConnecting()
// =============================================================================

void EthManager::handleConnecting()
{
    const uint32_t now =
        millis();


    // -------------------------------------------------------------------------
    // Already connected?
    //
    // Normally ARDUINO_EVENT_WIFI_STA_GOT_IP will already have changed
    // the state, but this is an additional safety check.
    // -------------------------------------------------------------------------

    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }


    // -------------------------------------------------------------------------
    // Connection timeout
    // -------------------------------------------------------------------------

    if ((now - _connectStartedAt) >=
        _config.connectTimeoutMs)
    {
        Serial.println(
            "[WiFi] Connection timeout"
        );


        WiFi.disconnect(
            false,
            false
        );


        _state.store(
            State::Disconnected,
            std::memory_order_release
        );


        _lastReconnectAttempt = now;
    }
}


// =============================================================================
// handleDisconnected()
// =============================================================================

void EthManager::handleDisconnected()
{
    if (!_config.autoReconnect)
    {
        return;
    }


    const uint32_t now =
        millis();


    if ((now - _lastReconnectAttempt) <
        _config.reconnectIntervalMs)
    {
        return;
    }


    _lastReconnectAttempt = now;


    Serial.println(
        "[WiFi] Attempting reconnect..."
    );


    startConnection();
}


// =============================================================================
// eventHandler()
// =============================================================================
//
// IMPORTANT:
//
// This callback is executed from the WiFi/FreeRTOS event task.
//
// Do not perform complicated application logic here.
//
// The callback only forwards the event to the class.
// =============================================================================

void EthManager::eventHandler(
    arduino_event_id_t event,
    arduino_event_info_t info)
{
    if (_instance == nullptr)
    {
        return;
    }


    _instance->handleEvent(
        event,
        info
    );
}


// =============================================================================
// handleEvent()
// =============================================================================

void EthManager::handleEvent(arduino_event_id_t event, arduino_event_info_t info){
  switch (event)
    {
    case ARDUINO_EVENT_ETH_START:
      {
        ESP_LOGI(TAG, "ETH start");
        break;
      }
    case ARDUINO_EVENT_ETH_STOP:
      {  
        ESP_LOGI(TAG, "ETH stop"); 
        break;
      }
    case ARDUINO_EVENT_ETH_CONNECTED:
      { 
        ESP_LOGI(TAG, "ETH connected");
        _ethConnected = true;
        break;
      }  
    case ARDUINO_EVENT_ETH_GOT_IP:
      {
        ESP_LOGI(TAG, "ETH got IP: %s", ETH.localIP().toString().c_str());
        break;
      }  
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      {
        ESP_LOGI(TAG, "ETH disconnected");
        _ethConnected = false;
        break;
      }
    case ARDUINO_EVENT_WIFI_READY:
      {
        ESP_LOGI(TAG, "[WiFi] Interface ready");
        break;    
      }
    case ARDUINO_EVENT_WIFI_STA_START:
      {
        ESP_LOGI(TAG,"[WiFi] STA started");
        break;
      }
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      {
        ESP_LOGI(TAG,"[WiFi] Connected to access point");
        break;
      }
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      {   
        ESP_LOGW(TAG, "WiFi disconnected: %" PRIu8 "", info.wifi_sta_disconnected.reason);
        ESP_LOGI(TAG, "Try reconnecting");
        _lastReconnectAttempt = millis();
        _state.store(State::Disconnected, std::memory_order_release);
        _disconnectReason.store(info.wifi_sta_disconnected.reason, std::memory_order_release);

        WiFi.disconnect(true, false);
        WiFi.begin();
      }
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      {
        //ESP_LOGI(TAG, "WiFi got ip: %s", WiFi.localIP().toString().c_str());


            // -----------------------------------------------------------------
            // IMPORTANT:
            // We do not call WiFi.localIP() here.
            // The callback runs in another FreeRTOS task.
              // The application can query the IP later from loop().
            // -----------------------------------------------------------------

            _state.store(
                State::Connected,
                std::memory_order_release
            );
            _disconnectReason.store(
                0,
                std::memory_order_release
            );
            break;
        }

         // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        {
            Serial.println(
                "[WiFi] Lost IP address"
            );


            _state.store(
                State::Disconnected,
                std::memory_order_release
            );


            _lastReconnectAttempt = millis();


            break;
        }


        // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_STA_STOP:
        {
            Serial.println(
                "[WiFi] STA stopped"
            );


            _state.store(
                State::Idle,
                std::memory_order_release
            );


            break;
        }


        // ---------------------------------------------------------------------
        default:
        {
            break;
        }
    }
}


// =============================================================================
// state()
// =============================================================================

EthManager::State EthManager::state() const
{
    return _state.load(
        std::memory_order_acquire
    );
}


// =============================================================================
// isConnected()
// =============================================================================

bool EthManager::isConnected(){
    //return state() == State::Connected;
    return _ethConnected;
}


// =============================================================================
// isConnecting()
// =============================================================================

bool EthManager::isConnecting() const
{
    return state() == State::Connecting;
}


// =============================================================================
// hasError()
// =============================================================================

bool EthManager::hasError() const
{
    return state() == State::Error;
}


// =============================================================================
// localIP()
// =============================================================================

IPAddress EthManager::localIP() const
{
    return WiFi.localIP();
}


// =============================================================================
// gatewayIP()
// =============================================================================

IPAddress EthManager::gatewayIP() const
{
    return WiFi.gatewayIP();
}


// =============================================================================
// subnetMask()
// =============================================================================

IPAddress EthManager::subnetMask() const
{
    return WiFi.subnetMask();
}


// =============================================================================
// dnsIP()
// =============================================================================

IPAddress EthManager::dnsIP() const
{
    return WiFi.dnsIP();
}


// =============================================================================
// rssi()
// =============================================================================

// int32_t EthManager::rssi() const
// {
//     if (!isConnected())
//     {
//         return 0;
//     }


//     return WiFi.RSSI();
// }


// =============================================================================
// ssid()
// =============================================================================

String EthManager::ssid() const
{
    return WiFi.SSID();
}


// =============================================================================
// macAddress()
// =============================================================================

String EthManager::macAddress() const
{
    return WiFi.macAddress();
}


// =============================================================================
// hostname()
// =============================================================================

const char* EthManager::hostname() const
{
    return WiFi.getHostname();
}


// =============================================================================
// disconnectReason()
// =============================================================================

uint8_t EthManager::disconnectReason() const
{
    return _disconnectReason.load(
        std::memory_order_acquire
    );
}


// =============================================================================
// disconnectReasonText()
// =============================================================================

const char* EthManager::disconnectReasonText() const
{
    return disconnectReasonToString(
        disconnectReason()
    );
}


// =============================================================================
// disconnectReasonToString()
// =============================================================================

const char* EthManager::disconnectReasonToString(
    uint8_t reason)
{
    // -------------------------------------------------------------------------
    // ESP-IDF WiFi disconnect reason codes.
    //
    // We intentionally return "Unknown" for codes not handled here so that
    // newer ESP-IDF versions remain compatible.
    // -------------------------------------------------------------------------

    switch (reason)
    {
        case 1:
            return "UNSPECIFIED";


        case 2:
            return "AUTH_EXPIRE";


        case 3:
            return "AUTH_LEAVE";


        case 4:
            return "ASSOC_EXPIRE";


        case 5:
            return "ASSOC_TOOMANY";


        case 6:
            return "NOT_AUTHED";


        case 7:
            return "NOT_ASSOCED";


        case 8:
            return "ASSOC_LEAVE";


        case 9:
            return "ASSOC_NOT_AUTHED";


        case 10:
            return "DISASSOC_PWRCAP_BAD";


        case 11:
            return "DISASSOC_SUPCHAN_BAD";


        case 15:
            return "4WAY_HANDSHAKE_TIMEOUT";


        case 16:
            return "GROUP_KEY_UPDATE_TIMEOUT";


        case 17:
            return "IE_IN_4WAY_DIFFERS";


        case 18:
            return "GROUP_CIPHER_INVALID";


        case 19:
            return "PAIRWISE_CIPHER_INVALID";


        case 20:
            return "AKMP_INVALID";


        case 21:
            return "UNSUPP_RSN_IE_VERSION";


        case 22:
            return "INVALID_RSN_IE_CAP";


        case 23:
            return "802_1X_AUTH_FAILED";


        case 24:
            return "CIPHER_SUITE_REJECTED";


        case 200:
            return "BEACON_TIMEOUT";


        case 201:
            return "NO_AP_FOUND";


        case 202:
            return "AUTH_FAIL";


        case 203:
            return "ASSOC_FAIL";


        case 204:
            return "HANDSHAKE_TIMEOUT";


        default:
            return "UNKNOWN";
    }
}


// =============================================================================
// reconnect()
// =============================================================================

void EthManager::reconnect()
{
    Serial.println(
        "[WiFi] Manual reconnect requested"
    );


    WiFi.disconnect(
        false,
        false
    );


    _disconnectReason.store(
        0,
        std::memory_order_release
    );


    _lastReconnectAttempt = millis();


    startConnection();
}


// =============================================================================
// disconnect()
// =============================================================================

void EthManager::disconnect()
{
    Serial.println(
        "[WiFi] Manual disconnect"
    );


    WiFi.disconnect(
        false,
        false
    );


    _state.store(
        State::Disconnected,
        std::memory_order_release
    );
}