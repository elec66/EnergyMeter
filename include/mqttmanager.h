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

MqttManager();
  ~MqttManager() {}

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
