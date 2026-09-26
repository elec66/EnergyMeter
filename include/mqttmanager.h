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

#pragma once

#include <Arduino.h>
#include <espMqttClient.h>
#include <WiFiClient.h>
#include "esp_log.h"
#include "config.h"
#include "ethmanager.h"


enum class MqttTopic {
    Unknown = 0,
    SollwertGas,
    SollwertWasser
};


class MqttManager {
public:

// Callback signature for alarm triggers
using MqttCallback = std::function<void()>;

// constructors and destructor  
MqttManager();
~MqttManager();  

void init();
void loop();
bool isConnected();
void publish(const char* topic, const String& payload);
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode* codes, size_t len);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);



// void onRequest(MqttCallback callback) {
//  _requestCallback = callback;


private:
WiFiClient espClient;
espMqttClient mqttClient;
// MqttCallback _requestCallback = nullptr;

// static void mqtt_callback(char* topic, byte* payload, unsigned int length);
// static MqttTopic parseTopic(const char* topic);
// static bool parsePayloadToUint32(const byte* payload, unsigned int length, uint32_t& result);

 static constexpr const char* TAG = "MQTT";
// uint32_t lastMqttReconnectAttempt = 0;
bool _reconnectMqtt = true;
bool _eth_connected = false;

protected:

};
