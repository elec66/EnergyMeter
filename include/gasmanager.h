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
