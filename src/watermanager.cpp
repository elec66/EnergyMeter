/***************************************************************************
  Water Manager:

 The Water Manager handles the water meter reading (via IR diodes) and the calculation of water consumption.

***************************************************************************/

#include "watermanager.h"
#include "webmanager.h"

extern MqttManager mqtt ; 
extern WebManager web ; 

float Water::lastWasserVal = 0.0f; // (or uint32_t / float depending on how it's declared in watercounter.h)

Water::Water() {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
   
}

// =============================================================================
// Initialization
// =============================================================================
void Water::init() {
  _pref.begin("water", false);
  _water_consumption = getWaterCounter();
  AsyncWebServer& server = web.getServer();
  analogReadResolution(12);  
  // Optional: Configure attenuator for the full voltage range (0 – 3.3 V)
  analogSetAttenuation(ADC_11db);
}


// =============================================================================
// Regular loop
// =============================================================================
void Water::loop() {

  bool watercounterpin_1 = readPIN1();
  bool watercounterpin_2 = readPIN2();

  switch (current_status) {
    
    // T0: Start – Wait for I1 = 0 (I2 = 1)
    case STATUS_0:
      if (watercounterpin_1 == LOW && watercounterpin_2 == HIGH) {
        current_status = STATUS_1;
      }
      break;

    // T1: Waiting for I2 to also switch from 1 to 0.
    case STATUS_1:
      if (watercounterpin_1 == LOW && watercounterpin_2 == LOW) {
        current_status = STATUS_2;
      } else if (watercounterpin_1 == HIGH) {
         ESP_LOGW(TAG, "Fehler bei T1: Eingang 1 ist %d, Eingang 2 ist %d\n",watercounterpin_1, watercounterpin_2);
        current_status = STATUS_0;
      }
      break;

    // T2: Waiting for I1 to switch from 0 back to 1.
    case STATUS_2:
      if (watercounterpin_1 == HIGH && watercounterpin_2 == LOW) {
        current_status = STATUS_3;
      } else if (watercounterpin_2 == HIGH) {
         ESP_LOGW(TAG, "Fehler bei T2: Eingang 1 ist %d, Eingang 2 ist %d\n",watercounterpin_1, watercounterpin_2);
        current_status = STATUS_0;
      }
      break;

    // T3: Waiting for I2 to switch from 0 back to 1 as well.
    case STATUS_3:
      if (watercounterpin_1 == HIGH && watercounterpin_2 == HIGH) {
        _water_consumption = _water_consumption + 1;
        ESP_LOGI(TAG, "new water tick: %u liter", _water_consumption);
        _pref.putLong("Watercounter", _water_consumption);                
        current_status = STATUS_0;
      } else if (watercounterpin_1 == LOW) {
        ESP_LOGW(TAG, "Fehler bei T3: Eingang 1 ist %d, Eingang 2 ist %d\n",watercounterpin_1, watercounterpin_2);
        current_status = STATUS_0;
      }
      break;
  }
  this->checkWaterLeakage();
}

// =============================================================================
// check water leckage
// =============================================================================
void Water::checkWaterLeakage() {
  uint32_t now = millis();

  // 1. Detect whether the variable "wasser" has been incremented.
  if (_water_consumption > lastWasserVal) {
    // Falls mehr als +1 auf einmal verarbeitet wird, den Stand anpassen
    uint32_t delta = _water_consumption - lastWasserVal; 
    lastWasserVal = _water_consumption;


    if (lastIncrementTime > 0) {
      // Calculate the time elapsed since the last increase
      uint32_t interval = (now - lastIncrementTime) / delta;
       ESP_LOGD(TAG, "Wasserleckage Interval:  %u", interval);

      // Enter in ring buffer
      intervals[intervalIndex] = interval;
      intervalIndex = (intervalIndex + 1) % BUFFER_SIZE;
      if (intervalCount < BUFFER_SIZE) intervalCount++;
 
      // Check for uniformity
      float stdDev = this->calculateStdDev();
      ESP_LOGD(TAG, "Wasserleckage StdDev:  %.3f", stdDev);
      bool isRegular = (stdDev <= MAX_STD_DEV_MS);

      if (isRegular) {
        if (!trackingActive) {
          // Steady flow begins
          trackingActive = true;
          regularStartTime = now;
          Serial.println("[INFO] Gleichmäßiger Wasserfluss erkannt. 10-Minuten-Timer gestartet.");
        }
      } else {
        if (trackingActive) {
          // Irregularity (e.g., normal consumption) -> Reset monitoring
          trackingActive = false;
          alarmTriggered = false;
          Serial.println("[INFO] Schwankung im Durchfluss erkannt. Timer zurückgesetzt.");
        }
      }
    }
    lastIncrementTime = now;
  }

  // 2. Timeout check: If the variable is not incremented for a prolonged period (flow stopped)
  if (trackingActive && (lastIncrementTime > 0) && (now - lastIncrementTime > TIMEOUT_MS)) {
    trackingActive = false;
    alarmTriggered = false;
    ESP_LOGI(TAG, "No water flow within timeout period. Monitoring reset.");
    mqtt.publish("ESP/Energie/Wasser/Alarm", "OFF");
  }

  // 3. Alarm Check: After 10 minutes of continuous, regular flow
  if (trackingActive && !alarmTriggered) {
    if (now - regularStartTime >= ALARM_INTERVAL_MS) {
      alarmTriggered = true;
      this->triggerAlarm();
    }
  }
}  
  

// =============================================================================
// trigger water leckage alarm
// =============================================================================
void Water::triggerAlarm() {
  ESP_LOGI(TAG, "ALARM: Leak suspected! Constant water flow.");
  mqtt.publish("ESP/Energie/Wasser/Alarm", "ON");
}  


// =============================================================================
// get water consumption()
// =============================================================================
float Water::getWaterCounter() {
    return _pref.getLong("Watercounter", 0);
}

// =============================================================================
// get hourly water consumption()
// =============================================================================
float Water::getHourlyWaterConsumption() {
  return _hourly_water_consumption;
}

// =============================================================================
// set hourly water consumption()
// =============================================================================
void Water::setHourlyWaterConsumption(uint32_t value) {
  _hourly_water_consumption = value;
}

// =============================================================================
// get daily water consumption()
// =============================================================================
float Water::getDailyWaterConsumption() {
  return _daily_water_consumption;
}

// =============================================================================
// set daily water consumption()
// =============================================================================
void Water::setDailyWaterConsumption(uint32_t value) {
  _daily_water_consumption = value;
}


// =============================================================================
// set water consumption()
// =============================================================================
void Water::setWaterConsumption(uint32_t value) {
    _water_consumption = value;
    _pref.putLong("Watercounter", value);
}


// =============================================================================
// Calculate standard deviation of intervals
// =============================================================================
float Water::calculateStdDev() {
  if (intervalCount < 2) return 999999.0f; // Zu wenig Daten

    float sum = 0.0f;
    for (size_t i = 0; i < intervalCount; i++) {
      sum += intervals[i];
    }
    float mean = sum / intervalCount;

    float varianceSum = 0.0f;
    for (size_t i = 0; i < intervalCount; i++) {
      float diff = intervals[i] - mean;
      varianceSum += diff * diff;
    }
    return sqrt(varianceSum / intervalCount);
  }


// =============================================================================
// Read analog pin
// =============================================================================
bool Water::readPIN1() {
  int rawValue = analogRead(WATER_INPUT_PIN_1);
  //Serial.printf("Analogwert von Pin 1, %d", rawValue);
  if (rawValue > max_analog_pin_1) {
    if (currentState1 != true) {
      currentState1 = true;
    }
  } else if (rawValue < min_analog_pin_1) {
    if (currentState1 != false) {
      currentState1 = false;
    }
  }
  return currentState1;
}

bool Water::readPIN2() {
  int rawValue = analogRead(WATER_INPUT_PIN_2);
  if (rawValue > max_analog_pin_2) {
    if (currentState2 != true) {
      currentState2 = true;
    }
  } else if (rawValue < min_analog_pin_2) {
    if (currentState2 != false) {
      currentState2 = false;
    }
  }
  return currentState2;
}

