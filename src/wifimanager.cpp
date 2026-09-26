#include "wifimanager.h"
#include <cstring>

// =============================================================================
// Static instance
// =============================================================================

WiFiManager* WiFiManager::_instance = nullptr;


// =============================================================================
// Constructor
// =============================================================================

WiFiManager::WiFiManager(const WiFiConfig& config)
    : _config(config)
{
    _instance = this;
}

// =============================================================================
// Destructor
// =============================================================================

WiFiManager::~WiFiManager()
{
    stop();

    if (_instance == this)
    {
        _instance = nullptr;
    }
}



// =============================================================================
// begin()
// =============================================================================

bool WiFiManager::begin()
{
    if (_started)
    {
        return true;
    }


    Serial.println();
    Serial.println("----------------------------------------");
    Serial.println("WiFiManager");
    Serial.println("----------------------------------------");


    // -------------------------------------------------------------------------
    // Validate configuration
    // -------------------------------------------------------------------------

    if (_config.ssid == nullptr ||
        strlen(_config.ssid) == 0)
    {
        Serial.println(
            "[WiFi] ERROR: SSID is empty"
        );

        _state.store(
            State::Error,
            std::memory_order_release
        );

        return false;
    }


    if (_config.hostname == nullptr ||
        strlen(_config.hostname) == 0)
    {
        Serial.println(
            "[WiFi] ERROR: hostname is empty"
        );

        _state.store(
            State::Error,
            std::memory_order_release
        );

        return false;
    }


    Serial.printf(
        "[WiFi] SSID     : %s\n",
        _config.ssid
    );

    Serial.printf(
        "[WiFi] Hostname : %s\n",
        _config.hostname
    );


    // -------------------------------------------------------------------------
    // IMPORTANT:
    //
    // Espressif requires setHostname() before WiFi is started.
    //
    // Therefore do this BEFORE WiFi.mode().
    // -------------------------------------------------------------------------

    WiFi.setHostname(
        _config.hostname
    );


    // -------------------------------------------------------------------------
    // Persistent configuration
    //
    // false prevents unnecessary writes to NVS.
    // -------------------------------------------------------------------------

    WiFi.persistent(
        _config.persistent
    );


    // -------------------------------------------------------------------------
    // Station mode
    // -------------------------------------------------------------------------

    WiFi.mode(
        WIFI_STA
    );


    // -------------------------------------------------------------------------
    // Automatic reconnect
    //
    // We deliberately manage reconnects ourselves so that the connection
    // state machine has a single owner.
    // -------------------------------------------------------------------------

    WiFi.setAutoReconnect(
        false
    );


    // -------------------------------------------------------------------------
    // Register event callback
    // -------------------------------------------------------------------------

    if (!_eventRegistered)
    {
        _eventId = WiFi.onEvent(
            WiFiManager::eventHandler
        );

        _eventRegistered = true;
    }


    // -------------------------------------------------------------------------
    // Start connection
    // -------------------------------------------------------------------------

    _started = true;

    startConnection();

    return true;
}


// =============================================================================
// stop()
// =============================================================================

void WiFiManager::stop()
{
    if (!_started)
    {
        return;
    }


    Serial.println(
        "[WiFi] Stopping"
    );


    // -------------------------------------------------------------------------
    // Remove event callback
    //
    // This must NOT be called from inside the event callback.
    // We are executing from the normal application context here.
    // -------------------------------------------------------------------------

    if (_eventRegistered)
    {
        WiFi.removeEvent(
            _eventId
        );

        _eventRegistered = false;
        _eventId = 0;
    }


    WiFi.disconnect(
        true,
        false
    );


    _state.store(
        State::Idle,
        std::memory_order_release
    );


    _started = false;
}


// =============================================================================
// loop()
// =============================================================================

void WiFiManager::loop()
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

void WiFiManager::startConnection()
{
    Serial.printf(
        "[WiFi] Connecting to \"%s\"...\n",
        _config.ssid
    );


    _state.store(
        State::Connecting,
        std::memory_order_release
    );


    _connectStartedAt = millis();


    // -------------------------------------------------------------------------
    // Clear previous disconnect reason
    // -------------------------------------------------------------------------

    _disconnectReason.store(
        0,
        std::memory_order_release
    );


    // -------------------------------------------------------------------------
    // Start WiFi connection
    // -------------------------------------------------------------------------

    const wl_status_t result =
        WiFi.begin(
            _config.ssid,
            _config.password
        );


    if (result == WL_NO_SSID_AVAIL)
    {
        Serial.println(
            "[WiFi] SSID not available"
        );
    }
}


// =============================================================================
// handleConnecting()
// =============================================================================

void WiFiManager::handleConnecting()
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

void WiFiManager::handleDisconnected()
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

void WiFiManager::eventHandler(
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

void WiFiManager::handleEvent(
    arduino_event_id_t event,
    arduino_event_info_t info)
{
    switch (event)
    {
        // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_READY:
        {
            Serial.println(
                "[WiFi] Interface ready"
            );

            break;
        }


        // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_STA_START:
        {
            Serial.println(
                "[WiFi] STA started"
            );

            break;
        }


        // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        {
            Serial.println(
                "[WiFi] Connected to access point"
            );

            break;
        }


        // ---------------------------------------------------------------------
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        {
            Serial.println();
            Serial.println(
                "[WiFi] Got IP address"
            );


            // -----------------------------------------------------------------
            // IMPORTANT:
            //
            // We do not call WiFi.localIP() here.
            //
            // The callback runs in another FreeRTOS task.
            //
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
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        {
            // -----------------------------------------------------------------
            // THIS is where "info" is valid.
            //
            // info is passed into this function by the WiFi event system.
            // -----------------------------------------------------------------

            const uint8_t reason =
                info.wifi_sta_disconnected.reason;


            _disconnectReason.store(
                reason,
                std::memory_order_release
            );


            Serial.printf(
                "[WiFi] Disconnected "
                "reason=%u (%s)\n",
                reason,
                disconnectReasonToString(reason)
            );


            _state.store(
                State::Disconnected,
                std::memory_order_release
            );


            _lastReconnectAttempt = millis();


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

WiFiManager::State WiFiManager::state() const
{
    return _state.load(
        std::memory_order_acquire
    );
}


// =============================================================================
// isConnected()
// =============================================================================

bool WiFiManager::isConnected() const
{
    return state() == State::Connected;
}


// =============================================================================
// isConnecting()
// =============================================================================

bool WiFiManager::isConnecting() const
{
    return state() == State::Connecting;
}


// =============================================================================
// hasError()
// =============================================================================

bool WiFiManager::hasError() const
{
    return state() == State::Error;
}


// =============================================================================
// localIP()
// =============================================================================

IPAddress WiFiManager::localIP() const
{
    return WiFi.localIP();
}


// =============================================================================
// gatewayIP()
// =============================================================================

IPAddress WiFiManager::gatewayIP() const
{
    return WiFi.gatewayIP();
}


// =============================================================================
// subnetMask()
// =============================================================================

IPAddress WiFiManager::subnetMask() const
{
    return WiFi.subnetMask();
}


// =============================================================================
// dnsIP()
// =============================================================================

IPAddress WiFiManager::dnsIP() const
{
    return WiFi.dnsIP();
}


// =============================================================================
// rssi()
// =============================================================================

int32_t WiFiManager::rssi() const
{
    if (!isConnected())
    {
        return 0;
    }


    return WiFi.RSSI();
}


// =============================================================================
// ssid()
// =============================================================================

String WiFiManager::ssid() const
{
    return WiFi.SSID();
}


// =============================================================================
// macAddress()
// =============================================================================

String WiFiManager::macAddress() const
{
    return WiFi.macAddress();
}


// =============================================================================
// hostname()
// =============================================================================

const char* WiFiManager::hostname() const
{
    return WiFi.getHostname();
}


// =============================================================================
// disconnectReason()
// =============================================================================

uint8_t WiFiManager::disconnectReason() const
{
    return _disconnectReason.load(
        std::memory_order_acquire
    );
}


// =============================================================================
// disconnectReasonText()
// =============================================================================

const char* WiFiManager::disconnectReasonText() const
{
    return disconnectReasonToString(
        disconnectReason()
    );
}


// =============================================================================
// disconnectReasonToString()
// =============================================================================

const char* WiFiManager::disconnectReasonToString(
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

void WiFiManager::reconnect()
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

void WiFiManager::disconnect()
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