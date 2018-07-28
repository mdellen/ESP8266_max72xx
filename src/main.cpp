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
#include "Font_Data.h"
#include "Font_Clock.h"

#include <WiFiManager.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h> // struct timeval
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
static char nodeID[5];
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
//uint8_t dot[] = { 8, 1, 14, 112, 128, 128, 112, 14, 1 }; //sine
// byte 0 is the number of column bytes (n) that form this character.
// byte 1..n – one byte for each column of the character. 
uint8_t dot[] = { 0x08, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}; 

const uint8_t F_PMAN1 = 6;
const uint8_t W_PMAN1 = 8;
uint8_t F_PMAN1a = 6;
uint8_t W_PMAN1a = 8;

static const uint8_t PROGMEM pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};
const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
uint8_t F_PMAN2a = 6;
uint8_t W_PMAN2a = 18;
static const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
};
// Sprite Definition
const uint8_t F_ROCKET = 2;
const uint8_t W_ROCKET = 11;
static const uint8_t PROGMEM rocket[F_ROCKET * W_ROCKET] =  // rocket
{
  0x18, 0x24, 0x42, 0x81, 0x99, 0x18, 0x99, 0x18, 0xa5, 0x5a, 0x81,
  0x18, 0x24, 0x42, 0x81, 0x18, 0x99, 0x18, 0x99, 0x24, 0x42, 0x99,
};

void createHString(char *pH, char *pL)
{
  for (; *pL != '\0'; pL++)
    *pH++ = *pL | 0x80; // offset character

  *pH = '\0'; // terminate the string
}

void scroll()
{
  //if (P.getZoneStatus(ZONE_LOWER) && P.getZoneStatus(ZONE_UPPER) && (Matrix.message[0] != '\0'))
  if (Matrix.message[0] != '\0')
  {
    if (ESP.getChipId() == 0xfcb9ef)
      delay(SCROLL_SPEED * 32);

    P.setFont(NULL);

    if (Matrix.BigFont)
    {
      P.displayClear();
      P.setIntensity(Matrix.brightness);
      P.setCharSpacing(5); // double height --> double spacing
      P.setFont(ZONE_UPPER, BigFontUpper);
      P.setFont(ZONE_LOWER, BigFontLower);
      P.displayZoneText(ZONE_UPPER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_UPPER, SCROLL_UPPER);
      P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
      P.synchZoneStart();
    }
    else
    {
      P.setIntensity(0, Matrix.brightness);
      P.setCharSpacing(0, 1); // double height --> double spacing
      P.displayZoneText(ZONE_LOWER, Matrix.message, PA_CENTER, SCROLL_SPEED, PAUSE_TIME, SCROLL_LOWER, SCROLL_LOWER);
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

  if (P.getZoneStatus(ZONE_LOWER))
  { //wait untill animation is done
    P.setIntensity(ZONE_UPPER, 0);
    P.setFont(1, numeric7Seg);
    P.setCharSpacing(1, 2);
    P.displayZoneText(ZONE_UPPER, tijd, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    sprintf(tijdS, "%02d", s);
    P.setIntensity(ZONE_LOWER, 0);
    P.setFont(0, numeric7Seg);
    P.setCharSpacing(0,0);
    //P.displayZoneText(ZONE_LOWER, tijdS, PA_CENTER, 30, 0, PA_PRINT, PA_NO_EFFECT);
    /*if (s <= 5) 
    P.displayZoneText(ZONE_LOWER, "$", PA_CENTER, 30, 0, PA_PRINT, PA_NO_EFFECT);
    else if (s >= 5 && s <30) 
    P.displayZoneText(ZONE_LOWER, "$$$", PA_CENTER, 30, 0, PA_PRINT, PA_NO_EFFECT);
    else if (s >= 30 && s <45) 
    P.displayZoneText(ZONE_LOWER, "$$$$$$", PA_CENTER, 30, 0, PA_PRINT, PA_NO_EFFECT);
    else if (s >= 45 && s <60) 
    P.displayZoneText(ZONE_LOWER, "$$$$$$$$$$$$", PA_CENTER, 30, 0, PA_PRINT, PA_NO_EFFECT);
    else*/
    //if (s % 2 == 0 )
    //P.displayZoneText(ZONE_LOWER, "", PA_CENTER, 50, 0, PA_OPENING, PA_CLOSING);
    //else
    //P.displayZoneText(ZONE_LOWER, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$", PA_CENTER, 50, 0, PA_OPENING, PA_CLOSING);
    if (s % 10 == 0) 
        P.displayZoneText(ZONE_LOWER, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$", PA_LEFT, 40, 0, PA_OPENING, PA_CLOSING);
   // P.displayZoneText(ZONE_LOWER, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$", PA_LEFT, 50, 0, PA_SPRITE, PA_SPRITE);

    //P.setSpriteData(rocket, W_ROCKET, F_ROCKET, rocket, W_ROCKET, F_ROCKET);
    //P.setSpriteData(ZONE_LOWER, pacman1, W_PMAN1, F_PMAN1, pacman1, W_PMAN1, F_PMAN1);
    //P.setSpriteData(pacman1, W_PMAN1a, F_PMAN1a, pacman2, W_PMAN2a, F_PMAN2a);
    //P.displayText(ZONE_LOWER, "$$$", PA_CENTER, 50, 1000, PA_SPRITE, PA_SPRITE);
    
    //P.addChar('$', degC);

    if (s % 2 == 0) flasher = true;
    else            flasher = false;
  }
  //sync = false;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n Starting");

  P.begin(MAX_ZONES);
  Matrix.sync = false;
  Matrix.BigFont = false;
  // Set up zones for 2 halves of the display
  P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
  P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);

  P.displayZoneText(ZONE_UPPER, "Wi-Fi", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayZoneText(ZONE_LOWER, ". . .", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();

  P.addChar('$', dot);

  WiFiManager wifiManager;
  wifiManager.setConnectTimeout(15);
  wifiManager.autoConnect("AutoConnectAP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  mqttSetup();

  //WELCOME ANIMATION
  P.displayZoneText(ZONE_UPPER, "", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayZoneText(ZONE_LOWER, "", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
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
}
