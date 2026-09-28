/***************************************************************************
  DS18B20 Manager:

  The DS18B20 Manager organizes all DS18B20 temperature devices.

***************************************************************************/
#include "DS18B20manager.h"

extern MqttManager mqtt ; 

DS18B20Manager::DS18B20Manager() 
    : _pin(ONE_WIRE), _oneWire(ONE_WIRE), _sensors(&_oneWire), _deviceCount(0) {
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
    }

// =============================================================================
// Initialization
// =============================================================================  
void DS18B20Manager::init() {
    _sensors.begin();
    _deviceCount = _sensors.getDS18Count();
    ESP_LOGD(TAG, "%u Temperaturelemente DS18B20 angeschlossen", _deviceCount);
    //this->getAllDeviceAddresses();
}


// =============================================================================
// Regular loop
// =============================================================================
String DS18B20Manager::getTemperatures() {
    JsonDocument doc;
    char buffer[10];
    _sensors.requestTemperatures();

    for (size_t i = 0; i < _deviceCount; i++) {
        float temp = this->getTempCByAddress(elements[i].address);
        ESP_LOGD(TAG, "%s: %.2f °C", elements[i].name, temp); 
        snprintf(buffer, sizeof(buffer), "%.1f", temp); 
        doc[elements[i].name]  = buffer;
    }
    String response;
    serializeJson(doc, response);
    return response;
    
}


// =============================================================================
// get temperatures
// =============================================================================
float DS18B20Manager::getTemperature(char* name) {
  _sensors.requestTemperatures();
  for (int i = 0; i < _deviceCount; i++) {
    if (strcmp(name, elements[i].name) == 0) {
      return this->getTempCByAddress(elements[i].address);
    }  
  }
  ESP_LOGW(TAG, "Element %s nicht gefunden!", name);
  return 0.0f;
}

// =============================================================================
// get hex addresses of all DS18B20
// =============================================================================
void DS18B20Manager::getAllDeviceAddresses() {

  DeviceAddress tempDeviceAddress;
  for (int i = 0; i < _deviceCount; i++) {
    if (_sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.print(" Hex-Adresse: ");
      this->printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Fehler beim Lesen der Adresse von Sensor ");
      Serial.println(i + 1);
    }
  }
}


// =============================================================================
// helper function to print 8-byte hex address
// =============================================================================
void DS18B20Manager::printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(":");
  }
}


// =============================================================================
// request temperatures
// =============================================================================
void DS18B20Manager::requestTemperatures() {
    _sensors.requestTemperatures();
}


// =============================================================================
// get number of devices
// =============================================================================
uint8_t DS18B20Manager::getDeviceCount() {
    return _deviceCount;
}

bool DS18B20Manager::getDeviceAddress(DeviceAddress deviceAddress, uint8_t index) {
    return _sensors.getAddress(deviceAddress, index);
}

String DS18B20Manager::getAddressString(uint8_t index) {
    DeviceAddress address;
    if (!_sensors.getAddress(address, index)) {
        return "N/A";
    }

    String addressStr = "";
    for (uint8_t i = 0; i < 8; i++) {
        if (address[i] < 16) addressStr += "0";
        addressStr += String(address[i], HEX);
        if (i < 7) addressStr += ":";
    }
    addressStr.toUpperCase();
    return addressStr;
}

// NEU: Liest Temperatur direkt über die 8-Byte-Adresse
float DS18B20Manager::getTempCByAddress(const uint8_t* deviceAddress) {
    if (!deviceAddress) return DEVICE_DISCONNECTED_C;
    return _sensors.getTempC(deviceAddress);
}

// NEU: Bietet die Möglichkeit, die Auflösung zielgerichtet zu ändern (9=0.5°C, 12=0.0625°C)
bool DS18B20Manager::setResolutionByAddress(const uint8_t* deviceAddress, uint8_t resolution) {
    if (!deviceAddress) return false;
    return _sensors.setResolution(deviceAddress, resolution);
}


