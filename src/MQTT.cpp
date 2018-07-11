#include <MQTT.h>
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

Ticker wifiReconnectTimer;

char temperature[6];
char weatherSummary[128];



matrix Matrix;


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

    //strncpy(mqttMessage, payload, 128); // copy the array, up to len limit max + 1

    if (strcmp(topic, "display/matrix") == 0)
    {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(payload);
        if (!root.success())
        {
            Serial.println("parseObject() failed");
            return;
        }

    
        if (root.containsKey("zone"))       Matrix.zone = root["zone"];      
        if (root.containsKey("message"))    strncpy(Matrix.message, root["message"], 200);   
        if (root.containsKey("align"))      {
            if (root["align"] =      "LEFT")      Matrix.align = PA_LEFT; 
            else if (root["align"] = "CENTER")    Matrix.align = PA_CENTER; 
            else if (root["align"] = "RIGHT")     Matrix.align = PA_RIGHT; 
        }
        if (root.containsKey("speed"))      Matrix.speed = root["speed"]; 
        if (root.containsKey("pause"))      Matrix.pause = root["pause"]; 
        if (root.containsKey("effectIn"))   Matrix.effectIn = root["effectIn"];   
        if (root.containsKey("effectOut"))  Matrix.effectOut = root["effectOut"]; 
        if (root.containsKey("brightness")) Matrix.brightness = root["brightness"];  
        if (root.containsKey("UTC"))        Matrix.UTC = root["UTC"];  
        if (root.containsKey("UTC"))        globalTime = root["UTC"];
        globalTime = globalTime - millis();
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