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
#include <MD_Parola.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <time.h>
#include <Ticker.h>
#include <mqtt.h>

Ticker scrollText;
Ticker flashDot;

extern struct matrix Matrix;

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

//#define MAX_ZONES 2
#define ZONE_SIZE 4
//#define MAX_DEVICES (MAX_ZONES * ZONE_SIZE)
#define MAX_DEVICES 8
//#define MAX_DEVICES 8
#define CLK_PIN D5  // or SCK
#define DATA_PIN D7 // or MOSI
#define CS_PIN D8   // or SS

#define ZONE_LOWER 0
#define ZONE_UPPER 1
#define ALIGN_LOWER PA_CENTER
#define ALIGN_UPPER ALIGN_LOWER

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Create the graphics library object, passing through the Parola MD_MAX72XX graphic object
//MD_MAXPanel MP = MD_MAXPanel(P.getGraphicObject(), MAX_DEVICES, 1);
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 0;
static char tijd[7];
static bool flasher = false;

void scroll()
{
  if (P.getZoneStatus(ZONE_LOWER))
  {
    P.setIntensity(Matrix.zone, Matrix.brightness);
    P.displayZoneText(ZONE_LOWER, Matrix.message, PA_LEFT, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    //P.displayZoneText(Matrix.zone, Matrix.message, PA_LEFT, Matrix.speed, Matrix.pause, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    P.setTextBuffer(Matrix.zone, Matrix.message);
   // P.setTextEffect(ZONE_UPPER, PA_SCROLL_DOWN, PA_SCROLL_UP);
   // P.setTextEffect(ZONE_LOWER, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
   // P.setSpeed(Matrix.speed);
   // P.setPause(Matrix.pause);

    //P.setTextEffect(Matrix.zone, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  
  //  P.setTextEffect(ZONE_LOWER, Matrix.effectIn, PA_SCROLL_LEFT);
  
   // P.setTextAlignment(ZONE_LOWER, PA_CENTER);
    
  }
}

void flashing()
{
  time_t now;
  struct tm *timeinfo;
  time(&now);

  timeinfo = gmtime(&now);

  int h, m;
  h = timeinfo->tm_hour;
  m = timeinfo->tm_min;

  sprintf(tijd, "%02d%c%02d", h, (flasher ? ':' : '|'), m);

  if (P.getZoneStatus(ZONE_UPPER))
  { //wait untill animation is done
    P.setIntensity(ZONE_UPPER, 0);
    P.displayZoneText(ZONE_UPPER, tijd, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    //P.setTextBuffer(Matrix.zone, Matrix.message);
  }

  flasher = !flasher;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n Starting");
  WiFiManager wifiManager;

  P.begin(MAX_ZONES);
  // Set up zones for 2 halves of the display
  P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
  P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);


  wifiManager.setConnectTimeout(15);
  wifiManager.autoConnect("AutoConnectAP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  mqttSetup();
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
    // P.setTextAlignment(PA_LEFT);
    // P.print("> " + String(progress / (total / 100)) + "%");
    static char ota[7];
    sprintf(ota, "%s%3d%s", "> ", (progress / (total / 100)), "%");
    P.displayZoneText(ZONE_UPPER, "OTA", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayZoneText(ZONE_LOWER, ota, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);

    P.displayAnimate();
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

  scrollText.attach(10, scroll);
  flashDot.attach(1, flashing);
}

void loop()
{
  ArduinoOTA.handle();
  P.displayAnimate();
}
