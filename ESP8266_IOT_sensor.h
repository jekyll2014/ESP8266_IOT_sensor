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

#define SCHEDULES_NUMBER 10
#define EVENTS_NUMBER 10
#define TELEGRAM_USERS_NUMBER 10
#define GSM_USERS_NUMBER 10

struct sensorDataCollection
{
	int year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

#ifdef AMS2320_ENABLE
	float ams_temp;
	float ams_humidity;
#endif

#ifdef HTU21D_ENABLE
	float htu21d_temp;
	float htu21d_humidity;
#endif

#ifdef BME280_ENABLE
	float bme280_temp;
	float bme280_humidity;
	float bme280_pressure;
#endif

#ifdef BMP180_ENABLE
	float bmp180_temp;
	float bmp180_humidity;
	float bmp180_pressure;
#endif

#ifdef AHTx0_ENABLE
	float ahtx0_temp;
	float ahtx0_humidity;
#endif

#ifdef DS18B20_ENABLE
	float ds1820_temp;
#endif

#ifdef DHT_ENABLE
	float dht_temp;
	float dht_humidity;
#endif

#ifdef MH_Z19_UART_ENABLE
	int mh_temp;
	int mh_uart_co2;
#endif

#ifdef MH_Z19_PPM_ENABLE
	int mh_ppm_co2;
#endif

#ifdef ADC_ENABLE
	uint16_t adc;
#endif

	uint32_t InterruptCounters[PIN_NUMBER];

	bool inputs[PIN_NUMBER];

	bool outputs[PIN_NUMBER];
};

struct commandTokens
{
	int tokensNumber;
	String command;
	int index;
	String arguments[10];
};

void IRAM_ATTR int1count();
void IRAM_ATTR int2count();
void IRAM_ATTR int3count();
void IRAM_ATTR int4count();
void IRAM_ATTR int5count();
void IRAM_ATTR int6count();
void IRAM_ATTR int7count();
void IRAM_ATTR int8count();

// CO2 UART
#ifdef MH_Z19_UART_ENABLE
int co2SerialRead();
byte getCheckSum(byte*);
#endif

// CO2 PPM
#ifdef MH_Z19_PPM_ENABLE
int co2PPMRead();
#endif

// ADC
#ifdef ADC_ENABLE
uint16_t getAdc();
#endif

// Events
#ifdef EVENTS_ENABLE
String getEvent(uint8_t);
void processEvent(uint8_t);
String printHelpEvent();
#endif

// Schedules
#ifdef SCHEDULER_ENABLE
String getSchedule(uint8_t);
void writeScheduleExecTime(uint8_t, uint32_t);
uint32_t readScheduleExecTime(uint8_t);
String printHelpSchedule();
void processSchedule(uint8_t);
#endif

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

// Telnet
#ifdef TELNET_ENABLE
void sendToTelnet(String&, uint8_t);
#endif

// SMTP
#ifdef SMTP_ENABLE
bool sendMail(String&, String&, String&);
#endif

#ifdef MQTT_ENABLE
#define MQTT_MAX_PACKET 100
String getMqttServer();
int getMqttPort();
String getMqttUser();
String getMqttPassword();
String getMqttDeviceId();
String getMqttTopicIn();
String getMqttTopicOut();
bool getMqttClean();
bool mqtt_connect();
bool mqtt_send(String&, int, String&);
void mqtt_callback(char*, uint8_t*, uint16_t);
#endif

// Telegram
#ifdef TELEGRAM_ENABLE
#define TELEGRAM_MESSAGE_MAX_SIZE 300
#define TELEGRAM_MESSAGE_DELAY 2000
#define TELEGRAM_RETRIES 3
#define TELEGRAM_MESSAGE_BUFFER_SIZE 10

struct telegramMessage
{
	uint8_t retries;
	int64_t user;
	String message;
};

String uint64ToString(uint64_t);
uint64_t StringToUint64(String&);
bool sendToTelegramServer(int64_t, String&);
void addMessageToTelegramOutboundBuffer(int64_t, String, uint8_t);
void removeMessageFromTelegramOutboundBuffer();
void sendToTelegram(int64_t, String&, uint8_t);
void sendBufferToTelegram();
uint64_t getTelegramUser(uint8_t);
#endif

// GSM
#ifdef GSM_ENABLE
#ifdef GSM_M590_ENABLE
#define SMS_TIME_OUT 10000
#define SMS_CHECK_TIME_OUT 30000
#define AT_COMMAND_INIT1 F("ATE0")				//disable echo
#define AT_COMMAND_INIT2 F("AT+CNMI=0,0,0,0,0") //Set message indication Format
#define AT_COMMAND_INIT3 F("AT+CMGF=1")			//set text mode
#define AT_COMMAND_INIT4 F("AT+CSCS=\"GSM\"")	//Set "GSM” character set
#define AT_COMMAND_STATUS1 F("AT+CPAS")			//Check module’s status
#define AT_COMMAND_STATUS2 F("AT+CREG?")		//Check network registration status
#define AT_COMMAND_STATUS3 F("AT+CSQ")			//Signal intensity
#define AT_COMMAND_STATUS4 F("AT+CPMS?")		//Get SMS count
#define AT_COMMAND_SEND F("AT+CMGS=\"")			//Message sending
#define AT_COMMAND_GET_SMS F("AT+CMGR=")		//get sms message #
#define AT_COMMAND_DELETE_SMS F("AT+CMGD=")		//Delete message #
#endif

#ifdef GSM_SIM800_ENABLE
#define SMS_TIME_OUT 10000
#define SMS_CHECK_TIME_OUT 30000
#define AT_COMMAND_INIT1 F("ATE0")				//disable echo
#define AT_COMMAND_INIT2 F("AT+CNMI=0,0,0,0,0") //-Set message indication Format
#define AT_COMMAND_INIT3 F("AT+CMGF=1")			//-set text mode
#define AT_COMMAND_INIT4 F("AT+CSCS=\"GSM\"")	//-Set "GSM” character set
#define AT_COMMAND_STATUS1 F("AT+CPAS")			//Check module’s status
#define AT_COMMAND_STATUS2 F("AT+CREG?")		//Check network registration status
#define AT_COMMAND_STATUS3 F("AT+CSQ")			//Signal intensity
#define AT_COMMAND_STATUS4 F("AT+CPMS?")		//-Get SMS count
#define AT_COMMAND_SEND F("AT+CMGS=\"")			//Message sending
#define AT_COMMAND_GET_SMS F("AT+CMGR=")		//-get sms message #
#define AT_COMMAND_DELETE_SMS F("AT+CMGD=")		//-Delete message #
#endif

String getGsmUser(uint8_t);
String sendATCommand(String, bool);
bool initModem();
bool sendSMS(String&, String&);
smsMessage parseSMS(String&);
smsMessage getSMS();
bool deleteSMS(int);
String getModemStatus();
#endif

// Google script
#ifdef GSCRIPT_ENABLE
bool sendValueToGoogle(String&);
#endif

// PushingBox
#ifdef PUSHINGBOX_ENABLE
bool sendToPushingBoxServer(String);
bool sendToPushingBox(String&);
#endif

// HTTP
#ifdef HTTP_ENABLE
void handleRoot();
void handleNotFound();
#endif

// NTP
#ifdef NTP_TIME_ENABLE

#define NTP_PACKET_SIZE 48	// NTP time is in the first 48 bytes of message
#define UDP_LOCAL_PORT 8888 // local port to listen for UDP packets

void sendNTPpacket(IPAddress&);
time_t getNtpTime();
void restartNTP();
#endif

// Log
#ifdef LOG_ENABLE
void addToLog(sensorDataCollection&);
#endif

// EEPROM
uint32_t collectEepromSize();
String readConfigString(uint16_t, uint16_t);
void readConfigString(uint16_t, uint16_t, char*);
uint32_t readConfigLong(uint16_t);
float readConfigFloat(uint16_t);
void writeConfigString(uint16_t, uint16_t, String);
void writeConfigString(uint16_t, uint16_t, char*, uint8_t);
void writeConfigLong(uint16_t, uint32_t);
void writeConfigFloat(uint16_t, float);
uint16_t calculateEepromCrc();
bool checkEepromCrc();
void refreshEepromCrc();

// I/O
bool add_signal_pin(uint8_t);
String set_output(uint8_t, String&);

// Help printout
String printConfig(bool);
String printStatus(bool);
String printHelp();
String printHelpAction();

// Sensor data processing
sensorDataCollection collectData();
String parseSensorReport(sensorDataCollection&, String, bool);

// Command processing
String processCommand(String&, uint8_t, bool);
void ProcessAction(String&, uint8_t, bool);
commandTokens parseCommand(String&, char, char, bool);

// Sensors
#ifdef TEMPERATURE_SENSOR
float getTemperature(sensorDataCollection&);
#endif

#ifdef HUMIDITY_SENSOR
float getHumidity(sensorDataCollection&);
#endif

#ifdef CO2_SENSOR
int getCo2(sensorDataCollection&);
#endif

String timeToString(uint32_t);
String MacToStr(const uint8_t);

#endif
