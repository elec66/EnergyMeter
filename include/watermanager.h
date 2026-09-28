#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "mqttmanager.h"


// Zustände der State Machine
enum SequenzZustand {
  STATUS_0, // Idle
  STATUS_1,         
  STATUS_2,          
  STATUS_3         
};

class Water {
public:

// Callback signature for alarm triggers
using AlarmCallback = std::function<void()>;

Water();
  ~Water() {}  


void init();
void loop();
float getWaterCounter();
float getHourlyWaterConsumption();
float getDailyWaterConsumption();
void setWaterConsumption(uint32_t value);
void setHourlyWaterConsumption(uint32_t value);
void setDailyWaterConsumption(uint32_t value);

// Sets the callback function to execute when the timer triggers
void onAlarmTrigger(AlarmCallback callback) {
 _alarmCallback = callback;
}

private:

AlarmCallback _alarmCallback = nullptr;
static constexpr const char* TAG = "WATER";
uint32_t now = millis();

float calculateStdDev();
bool readPIN1();
bool readPIN2();
void resetSequence(const char* grund);
static void IRAM_ATTR handleImpulse();
void checkWaterLeakage();
void triggerAlarm();

uint32_t _hourly_water_consumption = 0;
uint32_t _daily_water_consumption = 0;

static constexpr unsigned long MIN_PULSE_DURATION = 100; // Mindestdauer in Millisekunden (0,5 s)
static constexpr unsigned long DEBOUNCE_OFF_TIME  = 100;  // Entprellzeit für das Ausschalten (50 ms)

bool currentPinState_1 = LOW;
bool currentPinState_2 = LOW;
bool lastReadState_1  = LOW;
bool lastReadState_2  = LOW;
unsigned long highStartTime_1 = 0;
unsigned long highStartTime_2 = 0;
unsigned long lowStartTime_1  = 0;
unsigned long lowStartTime_2  = 0;
bool pulseRegistered_1 = false; // Verhindert Mehrfachaussendung bei langem Signal
bool pulseRegistered_2 = false; 
uint64_t consumption = 0;
static float lastWasserVal;
uint32_t lastIncrementTime = 0;
static constexpr uint32_t ALARM_INTERVAL_MS = 600000; 

static constexpr unsigned long UNCONTROLLED_WATER_FLOW_INTERVAL = 300000; // 5 Minuten = 300.000 ms
static constexpr unsigned long TIMEOUT_MS = 100000;                        // Timeout bei Ausbleiben (10 Sek.)
static constexpr size_t BUFFER_SIZE = 10;                                 // Anzahl der Impulse zur Gleichmäßigkeitsprüfung
static constexpr float MAX_STD_DEV_MS = 310.0;                             // Max. zulässige Abweichung in ms


static volatile bool newImpulse;

unsigned long intervals[BUFFER_SIZE] = {0};
size_t intervalIndex = 0;
size_t intervalCount = 0;

unsigned long regularStartTime = 0;
bool trackingActive = false;
bool alarmTriggered = false;

// Schwellenwerte
int min_analog_pin_1 = MIN_THRESHOLD_PIN_1;
int max_analog_pin_1 = MAX_THRESHOLD_PIN_1;
int min_analog_pin_2 = MIN_THRESHOLD_PIN_2;
int max_analog_pin_2 = MAX_THRESHOLD_PIN_2;
bool currentState1 = false;
bool currentState2 = false;

SequenzZustand current_status = STATUS_0;
uint32_t _water_consumption = 0;
Preferences _pref;

protected:

};

/* 
static: Die Variable/Funktion gehört fest zu dieser Datei (oder Funktionsaufruf) und behält ihren Zustand über die gesamte Laufzeit des Programms bei.
        Bei Variablen: Wenn der Wert über Aufrufe hinweg nicht gelöscht werden soll, oder um die Variable auf die aktuelle Datei zu begrenzen 
        (keine Konflikte mit gleichen Namen in anderen Dateien).
        Bei Funktionen: Um die Funktion im sogenannten File Scope zu kapseln (sie ist außerhalb der .cpp / Datei nicht sichtbar).

constexpr: Der Wert steht fest und wird direkt zur Entwicklungszeit (Beim Kompilieren) berechnet. 
              Er verbraucht zur Laufzeit im Normalfall keinen Arbeitsspeicher (RAM).
              Für alle festen Schwellenwerte, Zeiten, Konfigurationen und Größen. 
              Es ist die moderne C++-Alternative zu #define (typsicher und fehlerresistenter) sowie besser 
              als const (da const unter Umständen erst zur Laufzeit initialisiert wird).

volatile (Flüchtig): Sagt dem Compiler: "Optimiere den Zugriff auf diese Variable niemals weg und lies sie jedes Mal 
                     direkt aus dem echten RAM-Speicher neu aus!"
                    Pflicht bei Interrupt-Service-Routinen (ISR / Interrupts): Da der Hauptcode und der Interrupt asynchron laufen, 
                    merkt der Compiler ohne volatile oft nicht, dass sich der Wert im Hintergrund geändert hat, und liest veraltete Cache-Werte.

*/
