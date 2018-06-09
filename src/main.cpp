#include <Arduino.h>
// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK
//

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
//#include <MD_MAXPanel.h>
#include <MD_Parola.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <time.h>

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define MAX_DEVICES 4
#define CLK_PIN D5  // or SCK
#define DATA_PIN D7 // or MOSI
#define CS_PIN D8   // or SS

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Create the graphics library object, passing through the Parola MD_MAX72XX graphic object
//MD_MAXPanel MP = MD_MAXPanel(P.getGraphicObject(), MAX_DEVICES, 1);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n Starting");
  WiFiManager wifiManager;
  P.begin();
  //MP.begin();
  //P.displayText("WIFI...", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.print("> WIFI");
  wifiManager.setConnectTimeout(15);
  wifiManager.autoConnect("AutoConnectAP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Port defaults to 8266
  //ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("ESP8266_MATRIX");
  // No authentication by default
  //ArduinoOTA.setPassword((const char *)"xxxxx");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
    Serial.println("Rebooting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));

    //P.print("-OTA-");
    P.setTextAlignment(PA_LEFT);
    P.print("> " + String(progress / (total / 100)) + "%");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  static uint32_t lastTime = 0; // millis() memory
  static bool flasher = false;  // seconds passing flasher
  static char tijd[7];

  time_t now;
  struct tm *timeinfo;
  time(&now);

  timeinfo = gmtime(&now);

  ArduinoOTA.handle();
  P.displayAnimate();

  if (millis() - lastTime >= 1000)
  {
    int h, m;
    h = timeinfo->tm_hour;
    m = timeinfo->tm_min;

    sprintf(tijd, "%02d%c%02d", h, (flasher ? ':' : '|'), m);
    lastTime = millis();
    flasher = !flasher;

    //P.print(tijd);
    P.displayReset();
    P.displayText(tijd, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
   // MP.update(false);
   // MP.drawLine(6,0,6,16,1);
   // MP.update(true);
    Serial.println(tijd);

    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //only once a minute?
  }
  // delay(500);
}
