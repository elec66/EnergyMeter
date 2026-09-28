#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ETH.h>
#include <LittleFS.h>  
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "esp_log.h"
#include "config.h"
#include "gasmanager.h"
#include "smlmanager.h"
#include "mqttmanager.h"
#include "ethmanager.h"
#include "DS18B20manager.h"


class WebManager {
public:

WebManager();
  ~WebManager() {}  

void init();
AsyncWebServer& getServer();


private:
AsyncWebServer _server;
AsyncWebSocket _ws;

void setupRoutes();
void setupWS();
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

uint32_t floatToScaledInt(double input);
unsigned long ota_progress_millis = 0;

static constexpr const char* TAG = "WEB";
static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);

};
