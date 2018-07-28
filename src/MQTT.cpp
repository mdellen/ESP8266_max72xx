#include <MQTT.h>

#include "coredecls.h"  //tune_timeshift64
#include <time.h>
#include <sys/time.h>
#include <sys/reent.h>
#include "sntp.h"    

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
matrix Matrix;
char nodeID[22];

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
    mqttReconnectTimer.attach(60, mqttKeepAlive);
}

void onMqttConnect(bool sessionPresent)
{
    uint16_t packetIdSub1 = mqttClient.subscribe("display/matrix", 2);
    snprintf(nodeID, 22, "%s%02x%s", "display/matrix/", (long)ESP.getChipId(), "/");
    mqttClient.publish(nodeID, 0, true, "ONLINE");
    mqttClient.setWill(nodeID, 0, true, "OFFLINE");
}

void mqttKeepAlive()
{
    mqttClient.publish(nodeID, 0, true, "ONLINE");
    if (!mqttClient.connected())
    {
        Serial.print("Disconnected from MQTT.");
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.print("Disconnected from MQTT.");
    mqttReconnectTimer.once(2, connectToMqtt);
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

    if (strcmp(topic, "display/matrix") == 0)
    {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(payload);
        if (!root.success())
        {
            Serial.println("parseObject() failed");
            return;
        }

        if (root.containsKey("zone"))
            Matrix.zone = root["zone"];
        if (root.containsKey("message"))
        {
            strncpy(Matrix.message, root["message"], 200);
            Matrix.newMessage = true;
        }
        if (root.containsKey("align"))
        {
            if (root["align"] = "LEFT")
                Matrix.align = PA_LEFT;
            else if (root["align"] = "CENTER")
                Matrix.align = PA_CENTER;
            else if (root["align"] = "RIGHT")
                Matrix.align = PA_RIGHT;
        }
        if (root.containsKey("speed"))
            Matrix.speed = root["speed"];
        if (root.containsKey("pause"))
            Matrix.pause = root["pause"];
        if (root.containsKey("effectIn"))
            Matrix.effectIn = root["effectIn"];
        if (root.containsKey("effectOut"))
            Matrix.effectOut = root["effectOut"];
        if (root.containsKey("brightness"))
            Matrix.brightness = root["brightness"];
        if (root.containsKey("BigFont"))
            Matrix.BigFont = root["BigFont"];
        if (root.containsKey("UTC"))
        {
            Matrix.UTC = root["UTC"];
            tune_timeshift64(Matrix.UTC);
            timeshift64_is_set = true;
            Matrix.sync = true;
        }
    }
}

void onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}
