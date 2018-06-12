#if defined(ESP8266) 
#include <Ticker.h>
#elif defined(ESP32) 
#include <ESP32Ticker.h>
#endif

#include <ArduinoJson.h>

#include <AsyncMqttClient.h>
#define MQTT_HOST IPAddress(77,174,82,246) // www.mennovandellen.nl
#define MQTT_PORT 1883
#define MQTT_USER "mdel"
#define MQTT_PW "Yamaha01v"

//#include <WiFiManager.h>
//extern WiFiManager wifiManager;
extern AsyncMqttClient mqttClient;
extern Ticker mqttReconnectTimer;
//extern char* mqttMessage;
extern char* mqttTemp[];
//extern String weatherIcon;
extern char temperature[];
extern char weatherSummary[];

struct matrix
{
    char mqttMessage[200];
    int brightness;
};

void scroll();


void mqttSetup();
void mqttKeepAlive();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);

void connectToWifi();
