#include <Arduino.h>
// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK

//$ git remote add origin https://github.com/mdellen/ESP8266_max72xx.git
//$ git push -u origin master

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include "Font_Data.h"
#include "Font_Clock.h"

#include <WiFiManager.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h> // struct timeval
#include <Ticker.h>
#include <mqtt.h>

Ticker resetTimer;
WiFiManager wifiManager;

extern struct matrix Matrix;

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define ZONE_SIZE 4
#define MAX_DEVICES 8
#define CLK_PIN D5  // or SCK
#define DATA_PIN D7 // or MOSI
#define CS_PIN D8   // or SS

#define ZONE_LOWER 0
#define ZONE_UPPER 1
#define ALIGN_LOWER PA_CENTER
#define ALIGN_UPPER ALIGN_LOWER

#define SCROLL_LEFT 1
#if SCROLL_LEFT // invert and scroll left
#define SCROLL_UPPER PA_SCROLL_RIGHT
#define SCROLL_LOWER PA_SCROLL_LEFT
#else // invert and scroll right
#define SCROLL_UPPER PA_SCROLL_LEFT
#define SCROLL_LOWER PA_SCROLL_RIGHT
#endif
#define SPEED_TIME 75
#define PAUSE_TIME 0
#define SCROLL_SPEED 30

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Create the graphics library object, passing through the Parola MD_MAX72XX graphic object
//MD_MAXPanel MP = MD_MAXPanel(P.getGraphicObject(), MAX_DEVICES, 1);
const char *ntpServer = "nl.pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 0;
static char tijd[7];
static char tijdS[7];
static bool flasher = false;

timeval cbtime;
timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;

#define MAX_MESG 6
char szTimeL[MAX_MESG]; // mm:ss\0
char szTimeH[MAX_MESG];

uint8_t degC[] = {6, 3, 3, 56, 68, 68, 68}; // Deg C
// byte 0 is the number of column bytes (n) that form this character.
// byte 1..n – one byte for each column of the character.
uint8_t dot[] = {0x08, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

void createHString(char *pH, char *pL)
{
  for (; *pL != '\0'; pL++)
    *pH++ = *pL | 0x80; // offset character

  *pH = '\0'; // terminate the string
}

void resetNode()
{
   ESP.reset();
   delay(5000);
}

void scroll()
{
  //if (P.getZoneStatus(ZONE_LOWER) && P.getZoneStatus(ZONE_UPPER) && (Matrix.message[0] != '\0'))
  /*if (Matrix.speed) {
    #undef  SCROLL_SPEED
    #define SCROLL_SPEED 0
  }
  else {    
    #undef  SCROLL_SPEED
    #define SCROLL_SPEED 30
  }

  #undef SCROLL_UPPER
  #undef SCROLL_LOWER
  #if SCROLL_SPEED
  #define SCROLL_UPPER PA_PRINT
  #define SCROLL_LOWER PA_PRINT
  #else // invert and scroll right
  #define SCROLL_UPPER PA_SCROLL_LEFT
  #define SCROLL_LOWER PA_SCROLL_RIGHT
  #endif
*/
  if (Matrix.message[0] != '\0')
  {
    //if (ESP.getChipId() == 0xfcb9ef)
      //delay(SCROLL_SPEED * 32);
    
    P.setFont(NULL);

    if (Matrix.BigFont)
    {
      P.displayClear();
      //P.setIntensity(Matrix.brightness);
      P.setCharSpacing(5); // double height --> double spacing
      if (Matrix.flip){
        P.setFont(ZONE_LOWER, BigFontUpper);
        P.setFont(ZONE_UPPER, BigFontLower);
      }
      else {
        P.setFont(ZONE_UPPER, BigFontUpper);
        P.setFont(ZONE_LOWER, BigFontLower);
      }

      if(Matrix.mirror) {
        P.displayZoneText(ZONE_UPPER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
        P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_UPPER, SCROLL_UPPER);
      }
      else { 
        P.displayZoneText(ZONE_UPPER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_UPPER, SCROLL_UPPER);
        P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
      }
      P.synchZoneStart();
    }

    else
    {
      if (Matrix.flip) {
        P.setFont(ZONE_LOWER, numeric7Seg);
        P.setCharSpacing(ZONE_LOWER, 2);
        P.setCharSpacing(ZONE_UPPER, 1); // double height --> double spacing
        if(Matrix.mirror)   P.displayZoneText(ZONE_UPPER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
        else                P.displayZoneText(ZONE_UPPER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_UPPER, SCROLL_UPPER);
      }
      else{
        P.setFont(ZONE_UPPER, numeric7Seg);
        P.setCharSpacing(ZONE_UPPER, 2);
        P.setCharSpacing(ZONE_LOWER, 1); // double height --> double spacing
        if(Matrix.mirror)   P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_UPPER, SCROLL_UPPER);
        else                P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
      }
    }
  }
}

void flashing()
{
  uint64_t currentTime_us = micros64();
  tv.tv_sec = currentTime_us / 1000000ULL;
  tv.tv_usec = currentTime_us % 1000000ULL;

  time_t now;
  struct tm *timeinfo;
  time(&now);

  timeinfo = gmtime(&now);

  int h, m, s;
  h = timeinfo->tm_hour;
  m = timeinfo->tm_min;
  s = timeinfo->tm_sec;

  sprintf(tijd, "%02d%c%02d", h, (flasher ? ':' : ' '), m);

  if (P.getZoneStatus(ZONE_UPPER))
  { 
   if (Matrix.mirror) {
     P.setZoneEffect(ZONE_UPPER, false, PA_FLIP_LR);
     P.setZoneEffect(ZONE_LOWER, true,  PA_FLIP_LR);
   }
   else  {
     P.setZoneEffect(ZONE_UPPER, true,  PA_FLIP_LR);
     P.setZoneEffect(ZONE_LOWER, false, PA_FLIP_LR);
   }

   P.setIntensity(Matrix.brightness);

   if (Matrix.flip) {
      P.setZoneEffect(ZONE_UPPER, false, PA_FLIP_UD);
      P.setZoneEffect(ZONE_LOWER, true,  PA_FLIP_UD);
      P.setFont(ZONE_LOWER, numeric7Seg);
      P.setCharSpacing(ZONE_LOWER, 2);
      P.displayZoneText(ZONE_LOWER, tijd, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
   }
   else {
      P.setZoneEffect(ZONE_UPPER, true,  PA_FLIP_UD);
      P.setZoneEffect(ZONE_LOWER, false, PA_FLIP_UD);P.setFont(ZONE_UPPER, numeric7Seg);
      P.setCharSpacing(ZONE_UPPER, 2);
      P.displayZoneText(ZONE_UPPER, tijd, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
   }

    if (s % 2 == 0) flasher = true;
    else            flasher = false;
  }

 /* if (P.getZoneStatus(ZONE_LOWER)) 
  {
    if (s % 10 == 0)
    {
      P.addChar('$', dot);
      P.setIntensity(ZONE_LOWER, 0);
      P.setCharSpacing(0, 0);
      P.displayZoneText(ZONE_LOWER, "$$$$", PA_LEFT, 40, 0, PA_OPENING, PA_CLOSING);
    }
  }*/

}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n Starting");

  P.begin(MAX_ZONES);
  Matrix.sync = false;
  Matrix.BigFont = false;
  Matrix.mirror = false;
  Matrix.flip = false;
  // Set up zones for 2 halves of the display
  P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
  P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);

  P.displayZoneText(ZONE_UPPER, "Wi-Fi", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayZoneText(ZONE_LOWER, ". . .", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();

  
  wifiManager.setConnectTimeout(15);
  wifiManager.autoConnect("AutoConnectAP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  mqttSetup();

  //WELCOME ANIMATION
  P.displayClear();
  P.displayAnimate();
  P.displayZoneText(ZONE_UPPER, "|", PA_LEFT, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_RIGHT);
  P.displayZoneText(ZONE_LOWER, "|", PA_LEFT, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_RIGHT);

  //ArduinoOTA.setHostname("ESP8266_MATRIX");
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

    static char ota[7];
    sprintf(ota, "%s%3d%s", ">", (progress / (total / 100)), "%");

    P.setFont(NULL);
    P.setCharSpacing(2);
    P.setIntensity(3);
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
}

void loop()
{
  ArduinoOTA.handle();
  P.displayAnimate();
  gettimeofday(&tv, nullptr);

  static time_t lastv = 0;
  if (lastv != tv.tv_sec)
  {
    lastv = tv.tv_sec;
    flashing();
  }

  if (Matrix.newMessage)
  {
    scroll();
    Matrix.newMessage = false;
    Matrix.sync = false;
  }

  if (Matrix.reset) {
    resetTimer.attach(5, resetNode);
    strcpy(Matrix.message, "RESET");
    Matrix.newMessage = true;
    Matrix.BigFont = true;
    wifiManager.resetSettings();
    Matrix.reset = false;
  }

}
