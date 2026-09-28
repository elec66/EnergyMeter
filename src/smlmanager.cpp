/***************************************************************************
  sml Manager:

 The sml Manager handles the sml meter reading (via IR diode) and the calculation of current consumption.
 
***************************************************************************/

#include "smlmanager.h"
#include "webmanager.h"

HardwareSerial SMLSerial(1);
extern WebManager web ; 


Sml::Sml()
{
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

// =============================================================================
// Initialization
// =============================================================================
void Sml::init() {
  SMLSerial.setRxBufferSize(MAX_BUFFER_SIZE);
  SMLSerial.begin(BAUDRATE, SERIAL_8N1, SML_RX_PIN, SML_TX_PIN);
  lastByteTime = millis();
  AsyncWebServer& server = web.getServer();
}

// =============================================================================
// Manage UART serial connection
// =============================================================================
void Sml::loop() {
   // read SML-data from RX serial buffer to RAM buffer
    while (SMLSerial.available() > 0) {
      uint8_t byteIn = SMLSerial.read();

      // control buffer overflow
      if (bufferIndex < MAX_BUFFER_SIZE) {
        rxBuffer[bufferIndex++] = byteIn;
      } else {
        ESP_LOGW(TAG,"warning: buffer overflow!");
      }
      lastByteTime = millis();
    }

    // check timeout: data in buffer AND 0.5 seconds no data anymore
    if ((bufferIndex > 0) && (millis() - lastByteTime >= TIMEOUT_MS)) {
      this->processBuffer(rxBuffer, bufferIndex);
      ESP_LOGD(TAG,"receive SML-data successfully");
      _is_connected = true;
      bufferIndex = 0;    
    }

    // check timeout: no data in buffer AND 30 seconds no data anymore
    if ((bufferIndex == 0) && (millis() - lastByteTime >= 30000)) {
        ESP_LOGW(TAG,"no SML-data received");
        _is_connected = false;
        lastByteTime = millis();
    }
}


float Sml::readObis1_8_0(){
  return _consumption;
};

uint64_t Sml::readObis1_8_1(){
  return _consumptionT1;
};

uint64_t Sml::readObis1_8_2(){
  return _consumptionT2;
};

int64_t Sml::readObis16_7_0(){
  return _power;
};

String Sml::readManufacturerName(){
 return this->getManufacturerName(manufacturer).c_str();
};


String Sml::readManufacturer(){
    return manufacturer;
}


uint64_t Sml::readSerial_no(){
  return serial_no;
};

bool Sml::isConnected(){
  return _is_connected;
};

// =============================================================================
// get hourly gas consumption()
// =============================================================================
float Sml::getHourlyElecConsumption() {
  return _hourly_elec_consumption;
}

// =============================================================================
// set hourly gas consumption()
// =============================================================================
void Sml::setHourlyElecConsumption(uint32_t value) {
  _hourly_elec_consumption = value;  
}

// =============================================================================
// get daily gas consumption()
// =============================================================================
float Sml::getDailyElecConsumption() {
  return _daily_elec_consumption;
}

// =============================================================================
// set daily gas consumption()
// =============================================================================
void Sml::setDailyElecConsumption(uint32_t value) {
  _daily_elec_consumption = value;  
}

// =============================================================================
// Process buffer
// =============================================================================
void Sml::processBuffer(const uint8_t* buffer, size_t length) {

  // OBIS sequences
  uint8_t* match_obis170 = (uint8_t*) memmem(buffer, length, obis170, sizeof(obis170));
  uint8_t* match_obis180 = (uint8_t*) memmem(buffer, length, obis180, sizeof(obis180));
  uint8_t* match_obis181 = (uint8_t*) memmem(buffer, length, obis181, sizeof(obis181));
  uint8_t* match_obis182 = (uint8_t*) memmem(buffer, length, obis182, sizeof(obis182));
  uint8_t* match_obis280 = (uint8_t*) memmem(buffer, length, obis280, sizeof(obis280));
  uint8_t* match_obis1670 = (uint8_t*) memmem(buffer, length, obis1670, sizeof(obis1670));
  uint8_t* match_startsequenz = (uint8_t*) memmem(buffer, length, startsequenz, sizeof(startsequenz));
  uint8_t* match_endsequenz = (uint8_t*) memmem(buffer, length, endsequenz, sizeof(endsequenz));
  uint8_t* match_device_no = (uint8_t*) memmem(buffer, length, device_no, sizeof(device_no));

  // Check if valid SML protocol received
  if ((match_startsequenz != nullptr) and (match_endsequenz != nullptr)){
    
    // ---------------------------------------------------------
    // 1.8.0 - Positive active energy
    // ---------------------------------------------------------
    if (match_obis180 != nullptr) {
      size_t index = match_obis180 - buffer; 
      _consumption = this->readSMLType59(buffer, index+17);
    }  

    // ---------------------------------------------------------
    // 1.8.1 - Positive active energy tariff 1
    // ---------------------------------------------------------
    if (match_obis181 != nullptr) {
      size_t index = match_obis181 - buffer; 
      _consumptionT1 = this->readSMLType59(buffer, index+13);
    }  

    // ---------------------------------------------------------
    // 1.8.2 - Positive active energy tariff 2
    // ---------------------------------------------------------
    if (match_obis182 != nullptr) {
      size_t index = match_obis182 - buffer; 
      _consumptionT2 = this->readSMLType59(buffer, index+13);
    }  

    // ---------------------------------------------------------
    // 2.8.0 - Negative active energy
    // ---------------------------------------------------------   
    if (match_obis280 != nullptr) {
      size_t index = match_obis280 - buffer; 
    } 

    // ---------------------------------------------------------
    // 16.7.0 - Sum active instantaneous power
    // ---------------------------------------------------------
    if (match_obis1670 != nullptr) {
      size_t index = match_obis1670 - buffer; 
      if (buffer[index+13] == 255){
        _power = this->readSMLType55(buffer, index+13);
        ESP_LOGD(TAG,"Wertnegativ, vor Abzug: %d", _power);
        _power = _power - 4294967296;
        ESP_LOGD(TAG,"Wert nach Abzug: %d", _power);
      } else{
        ESP_LOGD(TAG,"Wert ist negativ");
        _power = this->readSMLType55(buffer, index+13);
      }
    } 

    // ---------------------------------------------------------
    // 0.0.9 - id-no of counter
    // ---------------------------------------------------------
    if (match_device_no != nullptr) {
      size_t index = match_device_no - buffer; 
      manufacturer = this->manufacturerToASCII(buffer, index+13);
      serial_no = this->readSMLType56(buffer, index+16);
    } 
  }
}

// =============================================================================
// Helper functions
// =============================================================================

// calculate value of SML-typ 55 (integer 32 bit) 
uint64_t Sml::readSMLType55(const uint8_t* data, size_t pos)
{
  return ((uint64_t)data[pos] << 24) |
         ((uint64_t)data[pos + 1] << 16) |
         ((uint64_t)data[pos + 2] << 8)  |
         ((uint64_t)data[pos + 3]);
}


// calculate value of SML-typ 56 (integer 40 bit) 
uint64_t Sml::readSMLType56(const uint8_t* data, size_t pos)
{
  return ((uint64_t)data[pos] << 32) |
         ((uint64_t)data[pos + 1] << 24) |
         ((uint64_t)data[pos + 2] << 16) |
         ((uint64_t)data[pos + 3] << 8)  |
         ((uint64_t)data[pos + 4]);
}


// calculate value of SML-typ 57 (integer 48 bit) 
uint64_t Sml::readSMLType57(const uint8_t* data, size_t pos)
{
  return ((uint64_t)data[pos] << 40) |
         ((uint64_t)data[pos + 1] << 32) |
         ((uint64_t)data[pos + 2] << 24) |
         ((uint64_t)data[pos + 3] << 16) |
         ((uint64_t)data[pos + 4] << 8)  |
         ((uint64_t)data[pos + 5]);
}


// calculate value of SML-typ 58 (integer 56 bit) 
uint64_t Sml::readSMLType58(const uint8_t* data, size_t pos)
{
  return ((uint64_t)data[pos] << 48) |
         ((uint64_t)data[pos + 1] << 40) |
         ((uint64_t)data[pos + 2] << 32) |
         ((uint64_t)data[pos + 3] << 24) |
         ((uint64_t)data[pos + 4] << 16) |
         ((uint64_t)data[pos + 5] << 8)  |
         ((uint64_t)data[pos + 6]);
}


// calculate value of SML-typ 59 (integer 64 bit) 
uint64_t Sml::readSMLType59(const uint8_t* data, size_t pos)
{
  return ((uint64_t)data[pos]     << 56) |
         ((uint64_t)data[pos + 1] << 48) |
         ((uint64_t)data[pos + 2] << 40) |
         ((uint64_t)data[pos + 3] << 32) |
         ((uint64_t)data[pos + 4] << 24) |
         ((uint64_t)data[pos + 5] << 16) |
         ((uint64_t)data[pos + 6] << 8)  |
         ((uint64_t)data[pos + 7]);
}


String Sml::manufacturerToASCII(const uint8_t* data, size_t pos)
{
  return String((char)data[pos]) +
         String((char)data[pos + 1]) +
         String((char)data[pos + 2]);
}



String Sml::getManufacturerName(const String& code)
{
  for (size_t i = 0; i < manufacturerCount; i++)
    {
      if (code == manufacturers[i].code){
          return String(manufacturers[i].name);
      }
    }
  return "Unknown";
}

