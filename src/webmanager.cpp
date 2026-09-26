/***************************************************************************
  Copyright (c) 2026 Thorsten Heins

  This file a part of the "EnergyMeter" source code.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  index.html:
  1) platformio.ini: board_build.filesystem = littlefs
  2) Platziere die index.html im Ordner data/ des PlatformIO-Projekts (/data/index.html).
  3) Wähle Platform -> Build Filesystem Image.
  4) Wähle anschließend Platform -> Upload Filesystem Image.

***************************************************************************/
#include <ArduinoJson.h> // Wichtig: ArduinoJson Header muss eingebunden sein
#include "webmanager.h"
#include "watermanager.h"

extern Water water; 
extern Gas gas; 
extern Sml sml;
extern MqttManager mqtt;
extern EthManager eth;
extern DS18B20Manager temperature;


WebManager::WebManager() : _server(80), _ws("/ws") {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

// =============================================================================
// Set dafault values
// =============================================================================
void WebManager::init() {
  setupWS();
  setupRoutes();
  _server.begin();
  ESP_LOGD(TAG, "HTTP server started");

  ElegantOTA.begin(&_server);  
  // ElegantOTA callbacks
  ElegantOTA.onStart([this]() {this->onOTAStart();});
  ElegantOTA.onProgress([this](size_t current, size_t final) {this->onOTAProgress(current, final);});
  ElegantOTA.onEnd([this](bool success) {this->onOTAEnd(success);});     
}


void WebManager::setupWS() {
    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);
}

// When a web client successfully connects via the WebSocket address /ws.
void WebManager::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        ESP_LOGD(TAG, "Client #%u verbunden", client->id());        
    } else if (type == WS_EVT_DISCONNECT) {
        ESP_LOGD(TAG, "Client #%u getrennt", client->id());
    }
}

// =============================================================================
// Set GET and POST routes
// =============================================================================
void WebManager::setupRoutes() {

    _server.serveStatic("/", LittleFS, "/");

    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(LittleFS, "/index.html", "text/html");
      }
    );
  
    _server.on("/values", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", sml.readObis1_8_0()/10000);
        doc["elec"]  = buffer;
        doc["power"] = sml.readObis16_7_0();
        snprintf(buffer, sizeof(buffer), "%.1f", gas.getGasCounter()/1000);
        doc["gas"]   = buffer;
        snprintf(buffer, sizeof(buffer), "%.3f", water.getWaterCounter()/1000);
        doc["water"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Warmwasserbehaelter"));
        doc["temp_ww"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Heizung_Vorlauf"));
        doc["temp_vl"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Heizung_Ruecklauf"));
        doc["temp_rl"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Wasser_Vorlauf"));
        doc["temp_wv"] = buffer;
        // snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Bibliothek_Vorlauf"));
        // doc["temp_bib_vl"] = buffer;
        // snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Bibliothek_Ruecklauf"));
        // doc["temp_bib_rl"] = buffer;
        // snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Bad_oben_Vorlauf"));
        // doc["temp_bad_vl"] = buffer;
        // snprintf(buffer, sizeof(buffer), "%.1f", temperature.getTemperature("Bad_oben_Ruecklauf"));
        // doc["temp_bad_rl"] = buffer;
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
      }
    );

    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        
        doc["wifi"]     = false; 
        doc["mqtt"]     = mqtt.isConnected(); 
        doc["eth"]      = eth.isConnected();
        doc["sml"]      = sml.isConnected();  
        doc["waterAlarm"] = false; 
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
      }
    );

    _server.on("/setpoint", HTTP_POST,
      [](AsyncWebServerRequest* req) {},
      nullptr,
      [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {

        static String jsonBuffer;

        // Clear buffers when a new packet begins.
        if (index == 0) {
          jsonBuffer = "";
        }

        // Collect received chunks in the buffer
        jsonBuffer += String((char*)data, len);

        // Check whether the entire package has been received.
        if (index + len >= total) {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, jsonBuffer);

          if (error) {
            ESP_LOGE(TAG, "JSON parsing failed: %s", error.c_str());
            req->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
            return;
          }

          JsonObject setpoint = doc["setpoint"];   
          double setpoint_gas   = setpoint["gas"] | 0.0f;
          double setpoint_water  = setpoint["water"] | 0.0f;  

          uint32_t int_setpoint_gas = floatToScaledInt(setpoint_gas);
          uint32_t int_setpoint_water = floatToScaledInt(setpoint_water);
          ESP_LOGD(TAG,"Gas Sollwert:   %u\n", int_setpoint_gas);
          ESP_LOGD(TAG,"Wasser Sollwert: %u\n", int_setpoint_water);
          gas.setGasCounter(int_setpoint_gas);
          water.setWaterConsumption(int_setpoint_water);
          req->send(200, "application/json", "{\"ok\":true}");
        }
      }
    );
}

uint32_t WebManager::floatToScaledInt(double input) {
  return static_cast<int32_t>(std::round(input * 1000.0));
}


AsyncWebServer& WebManager::getServer() {
  return _server;
}


void WebManager::onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void WebManager::onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void WebManager::onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}
  
