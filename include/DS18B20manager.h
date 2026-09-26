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
#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h> 
#include "mqttmanager.h"
#include "config.h"

struct TempSensor
{
  const uint8_t address[8]; // 8-Byte OneWire-address
  const char* name;         // description
};

// Array with all temperature elements
const TempSensor elements[] =
{
  { {0x28, 0xAA, 0xE6, 0x79, 0x51, 0x14, 0x01, 0xA9}, "Warmwasserbehaelter" },
  { {0x28, 0xAA, 0xFE, 0x78, 0x4B, 0x14, 0x01, 0x44}, "Heizung_Ruecklauf" },
  { {0x28, 0xAA, 0x07, 0x2F, 0x51, 0x14, 0x01, 0x6F}, "Wasser_Vorlauf" },
  { {0x28, 0xAA, 0x3F, 0x5E, 0x51, 0x14, 0x01, 0xF2}, "Heizung_Vorlauf" },
  { {0x28, 0x64, 0x93, 0x87, 0x00, 0x00, 0x00, 0xA7}, "Bibliothek_Vorlauf" },
  { {0x28, 0x29, 0x44, 0x87, 0x00, 0x00, 0x00, 0x97}, "Bibliothek_Ruecklauf" },
  { {0x28, 0x0D, 0xF7, 0x86, 0x00, 0x00, 0x00, 0x73}, "Bad_oben_Vorlauf" },
  { {0x28, 0xF7, 0x10, 0x07, 0xD6, 0x01, 0x3C, 0x02}, "Bad_oben_Ruecklauf" },

};

class DS18B20Manager {

    public:
    DS18B20Manager();
    void init();
    String getTemperatures();
    void getAllDeviceAddresses();
    void requestTemperatures();
    uint8_t getDeviceCount(); 
    float getTemperature(char* name); 
    bool getDeviceAddress(DeviceAddress deviceAddress, uint8_t index);  
    String getAddressString(uint8_t index);
    float getTempCByAddress(const uint8_t* deviceAddress);
    bool setResolutionByAddress(const uint8_t* deviceAddress, uint8_t resolution);
 
    private:
    void printAddress(DeviceAddress deviceAddress);
    static constexpr const char* TAG = "TEMP";
    uint8_t _pin;
    OneWire _oneWire;
    DallasTemperature _sensors;
    uint8_t _deviceCount;
    const size_t numSensors = sizeof(elements) / sizeof(elements[0]);
};

