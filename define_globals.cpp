#include "Arduino.h"
#include "datastructures.h"

uint32_t InterruptCounter[PIN_NUMBER];
bool inputs[PIN_NUMBER];
uint16_t outputs[PIN_NUMBER];

String deviceName = "";
uint8_t wifi_mode = WIFI_MODE_OFF;
uint32_t connectTimeLimit = 30000; //time for connection await
uint32_t reconnectPeriod = 600000; //time for connection await
uint8_t APclientsConnected = 0;
uint8_t currentWiFiMode = 0;
uint8_t wiFiIntendedStatus = RADIO_STOP;
uint32_t wifiReconnectLastTime = 0;
uint32_t ConnectionStarted = 0;
uint32_t WaitingStarted = 0;
bool wifiEnable = true;
bool telnetEnable = true;

uint32_t checkSensorLastTime = 0;
uint32_t checkSensorPeriod = 5000;

bool uartReady = false;
String uartCommand = "";
bool timeIsSet = false;

uint8_t autoReport = 0;

uint8_t SIGNAL_PINS = 0;
uint8_t INPUT_PINS = 0;
uint8_t OUTPUT_PINS = 0;
uint8_t INTERRUPT_PINS = 0;

String mqttCommand = "";
bool mqttEnable = false;
