#if defined(ESP8266) 
#include <Ticker.h>
#elif defined(ESP32) 
#include <ESP32Ticker.h>
#endif

#include <ArduinoJson.h>
#include <MD_Parola.h>

#include <AsyncMqttClient.h>
#define MQTT_HOST IPAddress(143,178,172,128) // www.mennovandellen.nl (2021.08.12)
#define MQTT_PORT 1883
#define MQTT_USER "matrix"
#define MQTT_PW "ledje"

extern AsyncMqttClient mqttClient;
extern Ticker mqttReconnectTimer;

struct matrix
{
    int     zone;
    char    message[200];
    textPosition_t   align;
    int     speed;
    int     pause;
    int     effectIn;
    int     effectOut;
    int     brightness;
    unsigned long  UTC;
    bool    BigFont;
    bool    mirror;
    bool    flip;
    bool    sync;
    unsigned long offset;
    bool    newMessage;
    bool    reset;
    bool    permanent;
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
