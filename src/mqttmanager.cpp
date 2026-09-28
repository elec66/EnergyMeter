/***************************************************************************
  MQTT Manager:

  The MQTT Manager organizes all MQTT functions and connections.

***************************************************************************/

#include "mqttmanager.h"

extern EthManager eth; 

// =============================================================================
// Constructor
// =============================================================================
MqttManager::MqttManager() {
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
}

// =============================================================================
// Set dafault values
// =============================================================================
void MqttManager::init() {
  mqttClient.onConnect([this](bool sessionPresent) {
    this->onMqttConnect(sessionPresent);
  });
  mqttClient.onDisconnect([this](espMqttClientTypes::DisconnectReason reason) {
    this->onMqttDisconnect(reason);
  });
  mqttClient.onSubscribe([this](uint16_t packetId, const espMqttClientTypes::SubscribeReturncode* codes, size_t len) {
    this->onMqttSubscribe(packetId, codes, len);
  });
  mqttClient.onUnsubscribe([this](uint16_t packetId) {
    this->onMqttUnsubscribe(packetId);
  });
  mqttClient.onMessage([this](const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total) {
    this->onMqttMessage(properties, topic, payload, len, index, total);
  });
  mqttClient.onPublish([this](uint16_t packetId) {
    this->onMqttPublish(packetId);
  });
  mqttClient.setCredentials(MQTT_USER, MQTT_PW);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);   
}


// =============================================================================
// loop()
// =============================================================================
void MqttManager::loop() {  
  if (eth.isConnected()) {
    if (!isConnected()) {
      ESP_LOGD(TAG, "Connecting to MQTT...");
      if (!mqttClient.connect()) {
        ESP_LOGW(TAG, "MQTT-Connection failed.");
      } else {
        ESP_LOGD(TAG, "MQTT-Connection activated.");
      }
    }
  } else {
    ESP_LOGW(TAG, "ETH or Wifi for MQTT not available.");
  } 
}


// =============================================================================
// mqtt connected ?
// =============================================================================
bool MqttManager::isConnected() {
    return mqttClient.connected();
}  

// =============================================================================
// mqtt publish
// =============================================================================
void MqttManager::publish(const char* topic, const String& payload){
   
    String value = payload;
    value.trim();
    //ESP_LOGD(TAG, "Publishing MQTT Topic/Payload");
    mqttClient.publish(topic, 0, true, payload.c_str());
}


// =============================================================================
// callback mqttconnected
// =============================================================================
void MqttManager::onMqttConnect(bool sessionPresent) {
  ESP_LOGD(TAG, "Connected to MQTT.");
  uint16_t packetIdSub1 = mqttClient.subscribe("Energie/Sollwert/Wasser", 2);
  uint16_t packetIdSub2 = mqttClient.subscribe("Energie/Sollwert/Gas", 2);

}


// =============================================================================
// callback mqtt disconnected
// =============================================================================
void MqttManager::onMqttDisconnect(espMqttClientTypes::DisconnectReason reason) {
  ESP_LOGW(TAG, "Disconnected from MQTT: %u", static_cast<uint8_t>(reason));
}


// =============================================================================
// callback mqtt subscribe
// =============================================================================
void MqttManager::onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode* codes, size_t len) {
  ESP_LOGD(TAG, "Subscribe acknowledged.");
  ESP_LOGD(TAG, "  packetId: %u", packetId);
  for (size_t i = 0; i < len; ++i) {
    ESP_LOGD(TAG, "  qos: %u", static_cast<uint8_t>(codes[i]));
  }
}


// =============================================================================
// callback mqtt unsubscribe
// =============================================================================
void MqttManager::onMqttUnsubscribe(uint16_t packetId) {
  ESP_LOGD(TAG, "Unsubscribe acknowledged.");
  ESP_LOGD(TAG, "  packetId: %u", packetId);
}


// =============================================================================
// callback mqtt message
// =============================================================================
void MqttManager::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total) {
  (void) payload;
  ESP_LOGD(TAG, "Publish received.");
  ESP_LOGD(TAG, "  topic: %s", topic);
  String pl = String((const char*)payload).substring(0, len);
  ESP_LOGD(TAG, "  payload: %s", pl);
  ESP_LOGD(TAG, "  qos: %u", properties.qos);
  ESP_LOGD(TAG, "  dup: %s", properties.dup? "true" : "false");
  ESP_LOGD(TAG, "  retain: %s", properties.retain ? "true" : "false");
  ESP_LOGD(TAG, "  len: %d", len);
  ESP_LOGD(TAG, "  index: %d", index);
  ESP_LOGD(TAG, "  total: %d", total);
}


// =============================================================================
// callback mqtt publish
// =============================================================================
void MqttManager::onMqttPublish(uint16_t packetId) {
  ESP_LOGD(TAG, "Publish acknowledged.");
  ESP_LOGD(TAG, "  packetId: %u", packetId);
}