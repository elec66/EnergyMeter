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

#include <TaskScheduler.h>
#include "webmanager.h"
#include "ethmanager.h"
#include "mqttmanager.h"
#include "timemanager.h"
#include "smlmanager.h"
#include "gasmanager.h"
#include "watermanager.h"
#include "DS18B20manager.h"
#include "config.h"


EthManager eth(wifiConfig);
WebManager web;
MqttManager mqtt;
TimeManager timer;
Sml sml;
Gas gas;
Water water;
DS18B20Manager temperature;
Scheduler task;

Task mqtt_connect(5 * TASK_SECOND, TASK_FOREVER, []() {mqtt.loop(); });
Task sml_meter(100 * TASK_MILLISECOND, TASK_FOREVER, []() {sml.loop(); });
Task water_meter(100 * TASK_MILLISECOND, TASK_FOREVER, []() {gas.loop(); });
Task gas_meter(100 * TASK_MILLISECOND, TASK_FOREVER, []() {water.loop(); });
Task tHourly(TASK_HOUR, TASK_FOREVER, []() {timer.calculateHourlyConsumptions(); });
Task tDaily(24 * TASK_HOUR, TASK_FOREVER, []() {timer.calculateDailyConsumptions(); });
Task t1min(1 * TASK_MINUTE, TASK_FOREVER, []() {timer.sendMqttMessages(); });


void setup() {
  Serial.begin(115200);
  eth.init();
  delay(3000);
  timer.init();
  mqtt.init(); 
  web.init(); 
  delay(1000);
  sml.init();
  gas.init();
  water.init(); 
  temperature.init(); 

  task.init(); 
  task.addTask(mqtt_connect);  
  task.addTask(sml_meter);  
  task.addTask(water_meter);  
  task.addTask(gas_meter); 
  task.addTask(tHourly);
  task.addTask(tDaily);
  task.addTask(t1min);

  mqtt_connect.enable();
  sml_meter.enable();
  water_meter.enable();
  gas_meter.enable();
  tHourly.enableDelayed(timer.getMillisUntilNextHour());
  tDaily.enableDelayed(timer.getMillisUntilNextDay());
  t1min.enable();

  // Watchdog-Konfiguration anpassen (statt init)
  // esp_task_wdt_config_t twdt_config = {
  //   .timeout_ms = WDT_TIMEOUT * 1000,
  //   .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Bezieht beide Kerne ein
  //   .trigger_panic = true
  // };
  // esp_task_wdt_reconfigure(&twdt_config);
  // esp_task_wdt_add(NULL); 
}



// ─────────────────────────────────────────────────────────────────
void loop() {
  task.execute();
  //Reset watchdog
  //esp_task_wdt_reset();
}