#include <MQTT.h>
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

Ticker wifiReconnectTimer;

String mqttMessage = "No MQTT message...";
//String weatherIcon    = "K";
RTC_DATA_ATTR char weatherIcon[] = "K";
String weatherSummary = "";
char icyTemperature[] = "----";

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
    // Serial.println("Connected to MQTT.");                 **********TODO
    // Serial.print("Session present: ");
    // Serial.println(sessionPresent);
    uint16_t packetIdSub1 = mqttClient.subscribe("forecast/icon", 2);
    uint16_t packetIdSub2 = mqttClient.subscribe("forecast/now", 2);
    mqttClient.subscribe("icy/temp", 2);
    //Serial.print("Subscribing at QoS 2, packetId: ");
    //Serial.println(packetIdSub1);
    //Serial.println(packetIdSub2);
    mqttClient.publish("test/lol", 0, true, "test123");
    //Serial.println("Publishing at QoS 0");
    //uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
    //Serial.print("Publishing at QoS 1, packetId: ");
    //Serial.println(packetIdPub1);
    //uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
    //Serial.print("Publishing at QoS 2, packetId: ");
    //Serial.println(packetIdPub2);
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
    char mqttMessage[len+1]; // size for temporary array with an extra spot for a NULL (if needed)
    strlcpy(mqttMessage, payload, len+1); // copy the array, up to len limit max + 1
    mqttMessage[len] = '\0'; // null terminate final char of the new array, strlcpy does this, but you should get a good habbit of doing it as well, especially if your new at char arrays
    Serial.println("");

    //char* mqttTopic = 
    
    if (strcmp(topic,"forecast/icon") == 0 )
    {
        strcpy(weatherIcon, mqttMessage);
        Serial.print("Weather icon: ");
        Serial.println(weatherIcon);
    }
    //char* mqttTopic2 =  "icy/temp";
    if (strcmp(topic,"icy/temp") == 0 )
    {
        strcpy(icyTemperature, mqttMessage);
        Serial.print("Temperature: ");
        Serial.println(icyTemperature);
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