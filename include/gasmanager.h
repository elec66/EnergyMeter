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
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <config.h>
#include "esp_log.h"
#include <mqttmanager.h>


class Gas {
public:

Gas();
  ~Gas() {}  

void init();
void loop();
float getGasCounter();
float getHourlyGasConsumption();
float getDailyGasConsumption();
void setGasCounter(uint32_t value);
void setHourlyGasConsumption(uint32_t value);
void setDailyGasConsumption(uint32_t value);

private:
Preferences _pref;

static constexpr const char* TAG = "GAS";
static constexpr unsigned long MIN_PULSE_DURATION = 100; 
static constexpr unsigned long DEBOUNCE_OFF_TIME  = 100; 

bool _currentPinState = LOW;
bool _lastReadState   = LOW;
unsigned long _highStartTime = 0;
unsigned long _lowStartTime  = 0;
bool _pulseRegistered = false; 
uint32_t _gas_counter = 0;
uint32_t _hourly_gas_consumption = 0;
uint32_t _daily_gas_consumption = 0;


protected:

};
