#pragma once

#ifndef ESP8266_IOT_sensor
#define ESP8266_IOT_sensor

#define SWITCH_ON_NUMBER F("1")
#define SWITCH_OFF_NUMBER F("0")
#define SWITCH_ON F("on")
#define SWITCH_OFF F("off")
#define EOL F("\r\n")
#define CSV_DIVIDER F(";")
#define QUOTE F("\"")

extern String deviceName ;

// WiFi settings
extern uint8_t wifi_mode;
extern uint32_t connectTimeLimit; //time for connection await
extern uint32_t reconnectPeriod; //time for connection await
extern uint8_t APclientsConnected;
extern uint8_t currentWiFiMode;
extern uint8_t wiFiIntendedStatus;
extern uint32_t wifiReconnectLastTime;
extern uint32_t ConnectionStarted;
extern uint32_t WaitingStarted;
extern bool wifiEnable;

//extern WiFiClient serverClient[MAX_SRV_CLIENTS];
extern bool telnetEnable;

extern uint32_t checkSensorLastTime;
extern uint32_t checkSensorPeriod;

extern String uartCommand ;
extern bool uartReady;

extern uint8_t autoReport;


extern uint8_t SIGNAL_PINS;
extern uint8_t INPUT_PINS;
extern uint8_t OUTPUT_PINS;
extern uint8_t INTERRUPT_PINS;

void ICACHE_RAM_ATTR int1count();
void ICACHE_RAM_ATTR int2count();
void ICACHE_RAM_ATTR int3count();
void ICACHE_RAM_ATTR int4count();
void ICACHE_RAM_ATTR int5count();
void ICACHE_RAM_ATTR int6count();
void ICACHE_RAM_ATTR int7count();
void ICACHE_RAM_ATTR int8count();

#ifdef NTP_TIME_ENABLE
extern bool timeIsSet;
#endif

//CO2 UART
int co2SerialRead();
byte getCheckSum(byte*);

//CO2 PPM
int co2PPMRead();

//ADC
uint16_t getAdc();

//Events
String getEvent(uint8_t);
void processEvent(uint8_t);
String printHelpEvent();

//Schedules
String getSchedule(uint8_t);
void writeScheduleExecTime(uint8_t, uint32_t);
uint32_t readScheduleExecTime(uint8_t);
String printHelpSchedule();
void processSchedule(uint8_t);

//SMTP
bool sendMail(String&, String&, String&);

//Telegram
String uint64ToString(uint64_t);
uint64_t StringToUint64(String);
bool sendToTelegramServer(int64_t, String);
void addMessageToTelegramOutboundBuffer(int64_t, String, uint8_t);
void removeMessageFromTelegramOutboundBuffer();
void sendToTelegram(int64_t, String, uint8_t);
void sendBufferToTelegram();
uint64_t getTelegramUser(uint8_t);

//GSM
#ifdef GSM_ENABLE
String getGsmUser(uint8_t);
String sendATCommand(String, bool);
bool initModem();
bool sendSMS(String, String&);
smsMessage parseSMS(String);
smsMessage getSMS();
bool deleteSMS(int);
String getModemStatus();
#endif

//Google script
bool sendValueToGoogle(String&);

//PushingBox
bool sendToPushingBoxServer(String);
bool sendToPushingBox(String&);

//HTTP
void handleRoot();
void handleNotFound();

//NTP
void sendNTPpacket(IPAddress&);
time_t getNtpTime();

//Log
void addToLog(sensorDataCollection&);

//Telnet
void sendToTelnet(String&, uint8_t);

//EEPROM
uint32_t CollectEepromSize();
String readConfigString(uint16_t, uint16_t);
void readConfigString(uint16_t, uint16_t, char*);
uint32_t readConfigLong(uint16_t);
float readConfigFloat(uint16_t);
void writeConfigString(uint16_t, uint16_t, String);
void writeConfigString(uint16_t, uint16_t, char*, uint8_t);
void writeConfigLong(uint16_t, uint32_t);
void writeConfigFloat(uint16_t, float);

//I/O
bool add_signal_pin(uint8_t);
String set_output(String&);

//Help printout
String printConfig(bool);
String printStatus(bool);
String printHelp();
String printHelpAction();

//Sensor data processing
sensorDataCollection collectData();
String ParseSensorReport(sensorDataCollection&, String, bool);

//Script Processing
String processCommand(String, uint8_t, bool);
void ProcessAction(String&, uint8_t, bool);

//Sensors
float getTemperature(sensorDataCollection&);
float getHumidity(sensorDataCollection&);
int getCo2(sensorDataCollection&);

//Wi-Fi
String getStaSsid();
String getStaPassword();
String getApSsid();
String getApPassword();
WiFiPhyMode getWifiStandard();
float getWiFiPower();
void startWiFi();
void Start_OFF_Mode();
void Start_STA_Mode();
void Start_AP_Mode();
void Start_AP_STA_Mode();

String timeToString(uint32_t);

#ifdef MQTT_ENABLE
#include <PubSubClient.h>

#define MQTT_MAX_PACKET 100

extern WiFiClient espClient;
extern PubSubClient mqtt_client;

extern String mqttCommand;
extern bool mqttEnable;

String getMqttServer();
int getMqttPort();
String getMqttUser();
String getMqttPassword();
String getMqttDeviceId();
String getMqttTopicIn();
String getMqttTopicOut();
bool getMqttClean();
bool mqtt_connect();
bool mqtt_send(String&, int, String);
void mqtt_callback(char*, uint8_t*, uint16_t);

#endif

#endif