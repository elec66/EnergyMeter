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
#include <time.h>
#include <functional>
#include <Preferences.h>
#include "config.h"
#include "ethmanager.h"
#include "gasmanager.h"
#include "watermanager.h"
#include "mqttmanager.h"
#include "DS18B20manager.h"
#include "smlmanager.h"

class TimeManager {
public:
    TimeManager();
    ~TimeManager();  

    void init();
    unsigned long getMillisUntilNextHour();
    unsigned long getMillisUntilNextDay();
    void calculateHourlyConsumptions();
    void calculateDailyConsumptions();
    void sendMqttMessages();

    private:  
    Preferences _pref;

    static constexpr const char* TAG = "TIME";
    // NTP Server Settings (Europe/Berlin CEST/CET)
    const char* _ntpServer1 = NTP_SERVER_1; // PTB Braunschweig (Primär)
    const char* _ntpServer2 = NTP_SERVER_2;    // Allgemeiner NTP-Pool (Secondary)
    const char* _ntpServer3 = NTP_SERVER_3;   // NIST Server (Tertiary)

    const char* _timeZone = TIMEZONE;

    static bool s_ntpInitialized;
    float last_hourly_gas_counter = 0.0f;
    float last_hourly_water_counter= 0.0f;
    float last_hourly_elec_counter = 0.0f;
    float last_daily_gas_counter = 0.0f;
    float last_daily_water_counter= 0.0f;
    float last_daily_elec_counter= 0.0f;
};

