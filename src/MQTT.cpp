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
    uint16_t packetIdSub2 = mqttClient.subscribe(nodeID, 0);
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

    if ((strcmp(topic, "display/matrix") == 0) || (strcmp(topic, nodeID) == 0))
    {
        StaticJsonDocument<200> jsonBuffer;
        //JsonObject &jsonBuffer = jsonBuffer.parseObject(payload);  //json 5

        auto error = deserializeJson(jsonBuffer, payload); //json 6
        if (error) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            return;
        }

        if (jsonBuffer.containsKey("zone"))
            Matrix.zone = jsonBuffer["zone"];
        if (jsonBuffer.containsKey("message"))
        {
            strncpy(Matrix.message, jsonBuffer["message"], 200);
            Matrix.newMessage = true;
        }
        if (jsonBuffer.containsKey("align"))
        {
            if (jsonBuffer["align"] = "LEFT")
                Matrix.align = PA_LEFT;
            else if (jsonBuffer["align"] = "CENTER")
                Matrix.align = PA_CENTER;
            else if (jsonBuffer["align"] = "RIGHT")
                Matrix.align = PA_RIGHT;
        }
        if (jsonBuffer.containsKey("speed"))
            Matrix.speed = jsonBuffer["speed"];
        if (jsonBuffer.containsKey("pause"))
            Matrix.pause = jsonBuffer["pause"];
        if (jsonBuffer.containsKey("effectIn"))
            Matrix.effectIn = jsonBuffer["effectIn"];
        if (jsonBuffer.containsKey("effectOut"))
            Matrix.effectOut = jsonBuffer["effectOut"];
        if (jsonBuffer.containsKey("brightness"))
            Matrix.brightness = jsonBuffer["brightness"];
        if (jsonBuffer.containsKey("BigFont"))
            Matrix.BigFont = jsonBuffer["BigFont"];
        if (jsonBuffer.containsKey("mirror"))
            Matrix.mirror = jsonBuffer["mirror"];
        if (jsonBuffer.containsKey("flip"))
            Matrix.flip = jsonBuffer["flip"];
        if (jsonBuffer.containsKey("UTC"))
        {
            Matrix.UTC = jsonBuffer["UTC"];
            //tune_timeshift64(Matrix.UTC); TEST WITH 0
            tune_timeshift64(0);
            timeshift64_is_set = true;
            Matrix.sync = true;
        }
        if (jsonBuffer.containsKey("reset"))
        {
            Matrix.reset = jsonBuffer["reset"];
            if (!Matrix.reset) {
                   ESP.reset();
                   delay(5000);
            }
        }
        
    }
   /* if (strcmp(topic, nodeID) == 0)
    {
        StaticJsonDocument<200> jsonBuffer;
        JsonObject &jsonBuffer = jsonBuffer.parseObject(payload);
        if (!jsonBuffer.success())
        {
            Serial.println("parseObject() failed");
            return;
        }
        if (jsonBuffer.containsKey("reset"))
        {
            Matrix.reset = jsonBuffer["reset"];
            if (!Matrix.reset) {
                   ESP.reset();
                   delay(5000);
            }
        }
        if (jsonBuffer.containsKey("mirror"))
        Matrix.mirror = jsonBuffer["mirror"];
    }

*/
}

void onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}
