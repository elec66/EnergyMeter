/***************************************************************************
  Copyright (c) 2026 Thorsten Heins

  This file is a part of the "ESP32-SML-Reader" source code.

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

#include "timemanager.h"

extern EthManager eth; 
extern Gas gas; 
extern Water water; 
extern MqttManager mqtt; 
extern DS18B20Manager temperature;
extern Sml sml;

bool TimeManager::s_ntpInitialized = false;

TimeManager::TimeManager() {
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

TimeManager::~TimeManager() {}


// =============================================================================
// Initialization
// =============================================================================
void TimeManager::init() {

  // Synchronize time via NTP (only once across instances)
  if (eth.isConnected()) {
    _pref.begin("values", false);
    if (!s_ntpInitialized) {
      configTzTime(_timeZone, _ntpServer1, _ntpServer2, _ntpServer3);

      struct tm timeinfo;
      while (!getLocalTime(&timeinfo)) {
        ESP_LOGD(TAG, "Waiting for NTP synchronization...");  
        delay(1000);
      }
      ESP_LOGD(TAG, "Time synchronized successfully!");  
      s_ntpInitialized = true;
    }

  // Align the first timer to the next interval boundary
  //this->scheduleNextInterval();
  }
}


// =============================================================================
// Calculate remaining milliseconds until next hour
// =============================================================================
unsigned long TimeManager::getMillisUntilNextHour() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 0; // Fehler beim Abrufen der Zeit
    }    
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;
    unsigned long remainingSeconds = (59 - minutes) * 60 + (60 - seconds);
    ESP_LOGD(TAG, "verbleibende Sekunden bis zur neuen Stunde: %u", remainingSeconds);  
    return remainingSeconds * 1000;
}


// =============================================================================
// Calculate remaining milliseconds until next day
// =============================================================================
unsigned long TimeManager::getMillisUntilNextDay() {
  struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
          return 0; // Fehler beim Abrufen der Zeit
      }      
      int hours = timeinfo.tm_hour;
      int minutes = timeinfo.tm_min;
      int seconds = timeinfo.tm_sec;
      unsigned long remainingSeconds = (23 - hours) * 3600 + (59 - minutes) * 60 + (60 - seconds);  
      ESP_LOGD(TAG, "verbleibende Sekunden bis zum neuen Tag: %u", remainingSeconds);   
      return remainingSeconds * 1000;
}


// =============================================================================
// Calculate hourly gas and water consumptions
// =============================================================================
void TimeManager::calculateHourlyConsumptions() {
  float _elec_counter = sml.readObis1_8_0();
  float _elec_hourly_consumption = _elec_counter - _pref.getLong("hourlyelec", 0);
  _pref.putLong("hourlyelec", _elec_counter);  
  sml.setHourlyElecConsumption(_elec_hourly_consumption);
  ESP_LOGD(TAG, "Stündlicher Stromverbrauch: %.3f m³", _elec_hourly_consumption);  

  float _gas_counter = gas.getGasCounter();
  float _gas_hourly_consumption = _gas_counter - _pref.getLong("hourlygas", 0);;
  _pref.putLong("hourlygas",  _gas_counter); 
  gas.setHourlyGasConsumption(_gas_hourly_consumption);
  ESP_LOGD(TAG, "Stündlicher Gasverbrauch: %.3f m³", _gas_hourly_consumption);  

  float _water_counter = water.getWaterCounter();
  float _water_hourly_consumption = _water_counter - _pref.getLong("hourlywater", 0);;
  _pref.putLong("hourlywater", _water_counter); 
  ESP_LOGD(TAG, "Stündlicher Wasserverbrauch: %.3f m³", _water_hourly_consumption);  
 
  char buffer[10];
  JsonDocument doc;
  snprintf(buffer, sizeof(buffer), "%.1f", _gas_hourly_consumption);
  doc["hourly_gas"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.3f", _water_hourly_consumption/1000);
  doc["hourly_water"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.0f", _elec_hourly_consumption/1000);
  doc["hourly_elec"] = buffer;
  String response;
  serializeJson(doc, response);
  mqtt.publish("ESP/Energie/Stundenwerte", response); 
}


// =============================================================================
// Calculate daily gas and water consumptions
// =============================================================================
void TimeManager::calculateDailyConsumptions() {
  float _elec_counter = sml.readObis1_8_0();
  float _elec_daily_consumption = _elec_counter - _pref.getLong("dailyelec", 0);;
  _pref.putLong("dailyelec", _elec_counter); 
  sml.setDailyElecConsumption(_elec_daily_consumption);
  ESP_LOGD(TAG, "Täglicher Stromverbrauch: %.0f m³", _elec_daily_consumption);  
  
  float _gas_counter = gas.getGasCounter();
  float _gas_daily_consumption = _gas_counter - _pref.getLong("dailygas", 0);;
  _pref.putLong("dailygas",  _gas_counter); 
  gas.setDailyGasConsumption(_gas_daily_consumption);
  ESP_LOGD(TAG, "Täglicher Gasverbrauch: %.3f m³", _gas_daily_consumption);  
 

  float _water_counter = water.getWaterCounter();
  float _water_daily_consumption = _water_counter - _pref.getLong("dailywater", 0);;
  _pref.putLong("dailywater", _water_counter); 
  ESP_LOGD(TAG, "Täglicher Wasserverbrauch: %.3f m³", _water_daily_consumption);  

  char buffer[10];
  JsonDocument doc;
  snprintf(buffer, sizeof(buffer), "%.1f", _gas_daily_consumption);
  doc["daily_gas"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.3f", _water_daily_consumption/1000);
  doc["daily_water"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.0f", _elec_daily_consumption/1000);
  doc["daily_elec"] = buffer;
  String response;
  serializeJson(doc, response);
  mqtt.publish("ESP/Energie/Tageswerte", response);  

}


// =============================================================================
// Send mqtt messages
// =============================================================================
void TimeManager::sendMqttMessages() {
  char buffer[10];
  JsonDocument doc;
  snprintf(buffer, sizeof(buffer), "%.1f", gas.getGasCounter()/1000);
  doc["gas"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.3f", water.getWaterCounter()/1000);
  doc["water"] = buffer;
  snprintf(buffer, sizeof(buffer), "%.0f", sml.readObis1_8_0()/10000);
  doc["elec"] = buffer;
  doc["power"] = sml.readObis16_7_0();
  String response;
  serializeJson(doc, response);
  mqtt.publish("ESP/Energie/Verbrauch", response);  
  //response = temperature.getTemperatures();
  //mqtt.publish("ESP/Energie/Temperaturen", response);  

}