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

***************************************************************************/

//W5500 Ethernet module
#define W5500_CS_PIN 10   
#define W5500_RST_PIN -1   // Not wired
#define W5500_INT_PIN 4    
#define W5500_SCK_PIN 12   
#define W5500_MISO_PIN 13  
#define W5500_MOSI_PIN 11  

//One Wire
#define ONE_WIRE 21 

//Internal RGB LED
#define RGB_LED 48
#define CONTROL_PIN 38     


#define LED_PIN 8
#define CS_PIN 0

//GAS
#define GAS_INPUT_PIN 7

//WATER
#define WATER_INPUT_PIN_1 15
#define WATER_INPUT_PIN_2 16
#define MIN_THRESHOLD_PIN_1 400
#define MAX_THRESHOLD_PIN_1 1000
#define MIN_THRESHOLD_PIN_2 1500
#define MAX_THRESHOLD_PIN_2 2500

//SML
#define SML_RX_PIN 18
#define SML_TX_PIN 17
#define BAUDRATE 9600

//WIFI
#define HOSTNAME "ESP32-C3-ISKRA-MT681"

//MQTT
#define MQTT_PSK_ID "mqttpskid"
#define MQTT_PSK    "70736B70736B70736B"
#define MQTT_ID "ISKRA-MT681"
#define MQTT_TOPIC_SUBSCRIBE_1 "Energie/Wasser/Sollwert"
#define MQTT_TOPIC_SUBSCRIBE_2 "Energie/Gas/Sollwert"
#define MQTT_PUBLISH_INTERVALL 5000

#define DEVICE_NAME "ISKRA MT681"
#define WDT_TIMEOUT 10


// POSIX timezone-string for Germany (MEZ/MESZ)
// CET-1CEST,M3.5.0,M10.5.0/3 means:
// - CET (UTC+1), MESZ im Sommer (UTC+2)
// - Switch to MESZ: March (M3), 5. Week (5 = last Sonday), Sonday (0)
// - Switch to CET: October (M10), 5. Week, Sonday at 03:00 o'clock
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1 "ptbtime1.ptb.de"  // PTB Braunschweig (Primär)
#define NTP_SERVER_2 "pool.ntp.org"     // Allgemeiner NTP-Pool (Secondary)
#define NTP_SERVER_3 "time.nist.gov"    // NIST Server (Tertiary)