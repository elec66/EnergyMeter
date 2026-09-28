#pragma once

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include "esp_log.h"

struct Manufacturer
{
    const char* code;
    const char* name;
};


// https://www.dlms.com/flag-id-directory/
const Manufacturer manufacturers[] =
{
  { "APA", "Apator" },
  { "DZG", "DZG (Deutsche Zählergesellschaft)" },
  { "EAS", "EasyMeter" },
  { "EFR", "EFR GmbH" },
  { "ELS", "Elster (Honeywell)" },
  { "EMH", "EMH Metering" },
  { "HOL", "Holley Technology / Holley Europe" },
  { "ISK", "Iskraemeco" },
  { "ITR", "Itron" },
  { "KAI", "Kaifa Technology" },
  { "LOG", "Landis+Gyr" },
  { "MST", "Metcom" },
  { "PFA", "Pfaffinger" },
  { "SAG", "Sagemcom" },
  { "STV", "Sensus (Xylem)" },
  { "SEN", "Sensus (Xylem)" },
  { "SGM", "Hager (SGM-Plattform)" },
  { "TCH", "Techem" },
  { "ZPA", "ZPA Smart Energy" }
};

class Sml {
public:

Sml();
  ~Sml() {}  

void init();
void loop();
float readObis1_8_0();
uint64_t readObis1_8_1();
uint64_t readObis1_8_2();
int64_t readObis16_7_0();
String readManufacturerName();
String readManufacturer();
uint64_t readSerial_no();
void setHourlyElecConsumption(uint32_t value);
void setDailyElecConsumption(uint32_t value);
float getHourlyElecConsumption();
float getDailyElecConsumption();

bool isConnected();

private:
Task _loopTask;
void processBuffer(const uint8_t* buffer, size_t length);
uint64_t readSMLType55(const uint8_t* data, size_t pos);
uint64_t readSMLType56(const uint8_t* data, size_t pos);
uint64_t readSMLType57(const uint8_t* data, size_t pos);
uint64_t readSMLType58(const uint8_t* data, size_t pos);
uint64_t readSMLType59(const uint8_t* data, size_t pos);
String manufacturerToASCII(const uint8_t* data, size_t pos);
String getManufacturerName(const String& code);

static constexpr const char* TAG = "SML";
uint64_t _consumption = 0;
uint64_t _consumptionT1 = 0;
uint64_t _consumptionT2 = 0;
uint32_t _hourly_elec_consumption = 0;
uint32_t _daily_elec_consumption = 0;
int64_t _power = 0;
uint64_t serial_no = 0;
String manufacturer = "";

static constexpr size_t manufacturerCount = sizeof(manufacturers) / sizeof(manufacturers[0]);
static constexpr size_t MAX_BUFFER_SIZE = 2048;
uint8_t rxBuffer[MAX_BUFFER_SIZE];
static const unsigned long TIMEOUT_MS = 500; 
size_t bufferIndex = 0;
unsigned long lastByteTime = 0;
bool _is_connected = false;

//https://www.promotic.eu/en/pmdoc/Subsystems/Comm/PmDrivers/PmIEC62056/IEC62056_OBIS.htm
// OBIS1.7.0: Positive active instantaneous power (A+) [kW]
static constexpr uint8_t obis170[]  = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};   
// OBIS1.8.0: Positive active energy (A+) total [kWh]
static constexpr uint8_t obis180[]  = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};   
// OBIS1.8.1: Positive active energy (A+) in tariff T1 [kWh]
static constexpr uint8_t obis181[]  = {0x01, 0x00, 0x01, 0x08, 0x01, 0xff};   
// OBIS1.8.2: Active energy (A+) in tariff T2 [kWh]
static constexpr uint8_t obis182[]  = {0x01, 0x00, 0x01, 0x08, 0x02, 0xff};   
// OBIS2.8.0: Negative active energy (A+) total [kWh]
static constexpr uint8_t obis280[]  = {0x01, 0x00, 0x02, 0x08, 0x00, 0xff};  
// OBIS16.7.0: Sum active instantaneous power (A+ - A-) [kW]
static constexpr uint8_t obis1670[] = {0x01, 0x00, 0x10, 0x07, 0x00, 0xff};  
//
static constexpr uint8_t startsequenz[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};  
//
static constexpr uint8_t endsequenz[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1A};  
// Device Number
static constexpr uint8_t device_no[] = {0x01, 0x00, 0x00, 0x00, 0x09, 0xff};

protected:

};
