#ifndef MY_CLASS_H
#define MY_CLASS_H

#include <NTPClient.h>
#include <esp_task_wdt.h>
#include <WiFiUdp.h>
#include "HTTPClient.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Update.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

AsyncWebServer server(80);

String storedSSID;
String storedPassword;
const int eepromSize = 64; // Adjust this based on your needs
int eepromAddress = 0;

 

const char* ntpServer = "pool.ntp.org"; // You can change this to a different NTP server if desired
const long gmtOffset_sec = 0; // Your GMT offset in seconds
const int daylightOffset_sec = 0; // Daylight offset (usually 3600 seconds for DST)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


#define FIRMWARE_URL "ota-URL"
HTTPClient http;

#define WIFI_SSID "x"
#define WIFI_PASSWORD "123456789"


#endif
