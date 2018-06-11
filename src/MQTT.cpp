#include <MQTT.h>
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

Ticker wifiReconnectTimer;

//String mqttMessage = "No MQTT message...";

char temperature[6];
char weatherSummary[128];
char mqttMessage[128]; 

void mqttSetup()
{
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCredentials(MQTT_USER, MQTT_PW);
    connectToMqtt();
}

void connectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
    uint16_t packetIdSub1 = mqttClient.subscribe("display/matrix", 2);
    //uint16_t packetIdSub2 = mqttClient.subscribe("forecast/today", 2);

    mqttClient.publish("display/matrix/connected", 0, true, "test123");

}

void mqttKeepAlive()
{
    if (!mqttClient.connected())
    {
        Serial.print("Disconnected from MQTT.");
        //if
        // (WiFi.isConnected())
        // {
        mqttReconnectTimer.once(2, connectToMqtt);
        // }
    }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.print("Disconnected from MQTT.");
    //if
    // (WiFi.isConnected())
    //{
    mqttReconnectTimer.once(2, connectToMqtt);
    //}
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    //Serial.print("  payload: ");
    //Serial.println(payload);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);

    //The paylod has to be NULL terminated to be ased as a String to display
    //char mqttMessage[len + 1];              // size for temporary array with an extra spot for a NULL (if needed)
    //strlcpy(mqttMessage, payload, len + 1); // copy the array, up to len limit max + 1
    strncpy(mqttMessage, payload, 128); // copy the array, up to len limit max + 1
   // mqttMessage[len] = '\0';                // null terminate final char of the new array, strlcpy does this, but you should get a good habbit of doing it as well, especially if your new at char arrays
   // Serial.println("");
   scroll;
   /* if (strcmp(topic, "forecast/now/temperature") == 0)
    {
        strncpy(temperature, mqttMessage, 6);
        Serial.print("Temperature: ");
        Serial.println(temperature);
    }
    //char* mqttTopic2 =  "icy/temp";*/
    if (strcmp(topic, "display/matrix") == 0)
    {

        strncpy(weatherSummary, mqttMessage, 128);

        Serial.print("Message: ");
        Serial.println(weatherSummary);
    }
}

void onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void connectToWifi()
{
    Serial.println("Connecting to Wi-Fi...");
    //WiFi.begin();
}

/*
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}
*/