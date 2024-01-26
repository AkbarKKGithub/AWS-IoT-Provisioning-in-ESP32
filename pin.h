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


#define FIRMWARE_URL "https://firebasestorage.googleapis.com/v0/b/oflow-iot.appspot.com/o/firmware.bin?alt=media&token=8ed0d198-c4db-486c-b87b-e1ba438186e5"
HTTPClient http;

#define WIFI_SSID "x"
#define WIFI_PASSWORD "123456789"



#define Top_Led 19
#define Mid_Led 18
#define Pump  14
#define Dry_Led 25
#define Fault_Led 13
#define Buzzer 22

#define Top_sens 16

#define Push_button 5
#define Dry_sens 33
#define Voltge_sens 34

bool top_lvl_flag=0;
bool old_top_lvl_flag=0;
bool mid_lvl_flag=0;
bool old_mid_lvl_flag=0;
bool fault_flag=0;
bool sensor_e_c_flag=0;
bool dry_run_flag=0;
bool dry_run_error_flag=0;
bool long_run_flag=0;
bool fault_voltage_flag=0;
bool pump_flag=0;
bool v_f=0;
uint8_t line_Voltage=0;
int off_time_min=0;
int off_t_remains=0;
uint8_t x=0;
uint8_t err_c=0;
uint8_t BootReason = 0;
uint8_t min_current = 10;
uint8_t max_current = 100;
uint8_t dry_c = 0;
int v_error=0;
int c_error=0;
int min_voltage=180;
int max_voltage=500;

unsigned long pump_run_time;
long pump_start_time;
long pump_restart_time;
long dry_run_time=150000;
long long_run_time=1000000;
long off_time_millis=0;
unsigned long timestamp;
int off_time=0;

#endif