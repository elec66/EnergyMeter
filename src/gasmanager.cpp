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

#include "gasmanager.h"

extern MqttManager mqtt ; 


Gas::Gas() {
    pinMode(GAS_INPUT_PIN, INPUT_PULLUP); 

}

void Gas::init() {
  _pref.begin("gas", false);
  _gas_counter = getGasCounter();

}
// =============================================================================
// Regular loop
// =============================================================================
void Gas::loop() {

  bool readGascounterPin = digitalRead(GAS_INPUT_PIN);
  unsigned long now = millis();

  if (readGascounterPin == LOW) {
    // If the pin has just gone LOW, record the start time
    if (_lastReadState == HIGH) {
      _lowStartTime = now;
    }

    // Check if the signal is LOW continuously for long enough
    if (!_pulseRegistered && (now - _lowStartTime >= MIN_PULSE_DURATION)) {
      _pulseRegistered = true; // Sperre setzen  
    }
  } else { // rawReading == HIGH
    // If the pin has just gone HIGH, record the start time
    if (_lastReadState == LOW) {
      _highStartTime = now;
    }

  // Pulse will only reset status once the signal has been stably LOW for, say, 50 ms. 
  // This prevents accidental re-triggering caused by brief fluctuations at the end of a long signal.
    if (_pulseRegistered && (now - _highStartTime >= DEBOUNCE_OFF_TIME)) {
      _pulseRegistered = false; 
      _gas_counter = _gas_counter + 100;
      _pref.putLong("Gascounter", _gas_counter);      
      ESP_LOGI(TAG, "new gas tick: %u l", _gas_counter);
   
    }
  }
  _lastReadState = readGascounterPin;
}
    
// =============================================================================
// get gas counter()
// =============================================================================
float Gas::getGasCounter() {
  return _pref.getLong("Gascounter", 0);
}

// =============================================================================
// get hourly gas consumption()
// =============================================================================
float Gas::getHourlyGasConsumption() {
  return _hourly_gas_consumption;
}

// =============================================================================
// set hourly gas consumption()
// =============================================================================
void Gas::setHourlyGasConsumption(uint32_t value) {
  _hourly_gas_consumption = value;  
}

// =============================================================================
// get daily gas consumption()
// =============================================================================
float Gas::getDailyGasConsumption() {
  return _daily_gas_consumption;
}

// =============================================================================
// set daily gas consumption()
// =============================================================================
void Gas::setDailyGasConsumption(uint32_t value) {
  _daily_gas_consumption = value;  
}

// =============================================================================
// set gas counter()
// =============================================================================
void Gas::setGasCounter(uint32_t value) {
  _gas_counter = value;
  _pref.putLong("Gascounter", value);
}