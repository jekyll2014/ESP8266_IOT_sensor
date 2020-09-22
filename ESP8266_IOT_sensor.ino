/* Untested feature:
 - SCHEDULE service
 - generalize index ranges (start all from 0)
 - generalize switches behavior (accept on/off as bool and 0..1023 for pwm)
 - save pwm outputs values at EEPROM
 - restore flag reset if PushingBox/E-Mail not sent
 - event action:
	 conditions:
		 input?=on,off,x,c
		 output?=on,off,x,c
		 counter>x
		 temperature<>x
		 humidity<>x
		 co2<>x
	 actions:
		 send_telegram

		 send_mail=address;message
		 send_pushingbox
		 send_GScript
		 save_log;
*/

/*
Planned features:
 - SSL/TLS MQTT, SMTP and TELEGRAM
 - OTA fw upgrade
 - mDNS, LLMNR responder
 - TIMERS service (start_timer?=n; stop_timer? commands running actions)
 - telegram
	- set TELEGRAM_URL "api.telegram.org"
	- set TELEGRAM_IP "149.154.167.198"
	- set TELEGRAM_PORT 443\
	- setFingerprint
	- use SSL
	- show simple keyboard
 - temperature/humidity offset setting
 - Amazon Alexa integration
 - http settings page
 - EVENT service - option to check action successful execution.
 - GoogleScript service - memory allocation problem
 - DEBUG mode verbose serial output
 - option to use UART for GPRS/sensors
 - SMS message paging
 - improve SMS management
 */

 /*
 Sensors to be supported:
	- BMP280
	- ACS712 (A0)
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include "configuration.h"
#include "datastructures.h"

#define SWITCH_ON_NUMBER F("1")
#define SWITCH_OFF_NUMBER F("0")
#define SWITCH_ON F("on")
#define SWITCH_OFF F("off")
#define EOL F("\r\n")
#define CSV_DIVIDER F(";")
#define QUOTE F("\"")

String deviceName = "";

// WiFi settings
#define MAX_SRV_CLIENTS 5 //how many clients should be able to telnet to this ESP8266
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

WiFiServer telnetServer(23);
WiFiClient serverClient[MAX_SRV_CLIENTS];
bool telnetEnable = true;

uint32_t checkSensorLastTime = 0;
uint32_t checkSensorPeriod = 5000;

String uartCommand = "";
bool uartReady = false;

bool timeIsSet = false;

uint8_t autoReport = 0;

uint8_t SIGNAL_PINS = 0;
uint8_t INPUT_PINS = 0;
uint8_t OUTPUT_PINS = 0;
uint8_t INTERRUPT_PINS = 0;

void ICACHE_RAM_ATTR int1count();
void ICACHE_RAM_ATTR int2count();
void ICACHE_RAM_ATTR int3count();
void ICACHE_RAM_ATTR int4count();
void ICACHE_RAM_ATTR int5count();
void ICACHE_RAM_ATTR int6count();
void ICACHE_RAM_ATTR int7count();
void ICACHE_RAM_ATTR int8count();

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

//MQTT
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
String getGsmUser(uint8_t);
String sendATCommand(String, bool);
bool initModem();
bool sendSMS(String, String&);
smsMessage parseSMS(String);
smsMessage getSMS();
bool deleteSMS(int);
String getModemStatus();

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

void processEvent(uint8_t);
String getSchedule(uint8_t);
sensorDataCollection collectData();
void mqtt_callback(char*, uint8_t*, uint16_t);
uint64_t getTelegramUser(uint8_t);
String sendATCommand(String, bool);
String waitResponse();

// DATA BUSES

#ifdef I2C_ENABLE
#include <Wire.h>

TwoWire i2c;
#endif

#ifdef ONEWIRE_ENABLE
#include <OneWire.h>

OneWire oneWire(ONEWIRE_DATA);
#endif

#ifdef UART2_ENABLE
#include <SoftwareSerial.h>

SoftwareSerial uart2(UART2_TX, UART2_RX);
uint32_t uart2_Speed = 9600;
#endif

// SENSORS

//AM2320 I2C temperature + humidity sensor
#ifdef AMS2320_ENABLE
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"

Adafruit_AM2320 am2320 = Adafruit_AM2320();
bool am2320_enable = true;
#endif

// HTU21D I2C temperature + humidity sensor
#ifdef HTU21D_ENABLE
#include "Adafruit_HTU21DF.h"

Adafruit_HTU21DF htu21d = Adafruit_HTU21DF();
bool htu21d_enable = true;
#endif

// BME280 I2C temperature + humidity + pressure sensor
#ifdef BME280_ENABLE
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME280_ADDRESS 0xF6

Adafruit_BME280 bme280;
bool bme280_enable = true;
#endif

// BMP180 I2C temperature + pressure sensor
#ifdef BMP180_ENABLE
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp180;
bool bmp180_enable = true;
#endif

// DS18B20 OneWire temperature sensor
#ifdef DS18B20_ENABLE
#include "DallasTemperature.h"

DallasTemperature ds1820(&oneWire);
bool ds1820_enable = true;
#endif

// DHT11/DHT21(AM2301)/DHT22(AM2302, AM2321) temperature + humidity sensor
#ifdef DHT_ENABLE
#include "DHT.h"

DHT dht(DHT_PIN, DHT_ENABLE);
#endif

// MH-Z19 CO2 sensor via UART
#ifdef MH_Z19_UART_ENABLE
uint32_t mhz19UartSpeed = 9600;
uint16_t co2_uart_avg[3] = { 0, 0, 0 };
int mhtemp_s = 0;

int co2SerialRead()
{
	const int STATUS_NO_RESPONSE = -2;
	const int STATUS_CHECKSUM_MISMATCH = -3;
	const int STATUS_INCOMPLETE = -4;
	const int STATUS_NOT_READY = -5;
	const int STATUS_PWM_NOT_CONFIGURED = -6;
	const int STATUS_SERIAL_NOT_CONFIGURED = -7;

	unsigned long lastRequest = 0;

	uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
	uart2.flush();
	delay(50);

	byte cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
	byte response[9];  // for answer

	uart2.write(cmd, 9);  // request PPM CO2
	lastRequest = millis();

	// clear the buffer
	memset(response, 0, 9);

	int waited = 0;
	while (uart2.available() == 0)
	{
		delay(100);  // wait a short moment to avoid false reading
		if (waited++ > 10) {
			uart2.flush();
			return STATUS_NO_RESPONSE;
		}
	}

	boolean skip = false;
	while (uart2.available() > 0 && (unsigned char)uart2.peek() != 0xFF)
	{
		if (!skip)
		{
			Serial.print(F("MHZ: - skipping unexpected readings:"));
			skip = true;
		}
		Serial.print(" ");
		Serial.print(uart2.peek(), HEX);
		uart2.read();
	}
	if (skip) Serial.println();

	if (uart2.available() > 0)
	{
		int count = uart2.readBytes(response, 9);
		if (count < 9)
		{
			uart2.flush();
			return STATUS_INCOMPLETE;
		}
	}
	else
	{
		uart2.flush();
		return STATUS_INCOMPLETE;
	}

	// checksum
	byte check = getCheckSum(response);
	if (response[8] != check) {
		mhtemp_s = STATUS_CHECKSUM_MISMATCH;
		uart2.flush();
		return STATUS_CHECKSUM_MISMATCH;
	}
	uart2.flush();
	uart2.end();

	int ppm_uart = 256 * (int)response[2] + response[3];
	mhtemp_s = response[4] - 44;  // - 40;
	byte status = response[5];

	return ppm_uart;
}

byte getCheckSum(byte* packet)
{
	unsigned char checksum = 0;
	for (byte i = 1; i < 8; i++) {
		checksum += packet[i];
	}
	checksum = 0xff - checksum;
	checksum += 1;
	return checksum;
}
#endif

// MH-Z19 CO2 sensor via PPM signal
#ifdef MH_Z19_PPM_ENABLE
uint16_t co2_ppm_avg[3] = { 0, 0, 0 };

int co2PPMRead()
{
	uint16_t th, ppm;
	const uint32_t timeout = 1000;
	const uint32_t timer = millis();
	do
	{
		th = pulseIn(MH_Z19_PPM_ENABLE, HIGH, 1004000) / 1000;
		const uint16_t tl = 1004 - th;
		ppm = 5000 * (th - 2) / (th + tl - 4);
		if (millis() - timer > timeout)
		{
			//Serial.println(F("Failed to read PPM"));
			return -1;
		}
		yield();
	} while (th == 0);
	return ppm;
}
#endif

#ifdef DISPLAY_ENABLED
uint32_t displaySwitchedLastTime = 0;
uint32_t displaySwitchPeriod = 3000;

// TM1637 display.
#ifdef TM1637DISPLAY_ENABLE
/*TM1637 display leds layout:
		a
		_
	f| |b
		-g
	e|_|c
		d
PINS: D3 - CLK, D4 - DIO */
#include <TM1637Display.h>

const uint8_t SEG_DEGREE[] = { SEG_A | SEG_B | SEG_F | SEG_G };
const uint8_t SEG_HUMIDITY[] = { SEG_C | SEG_E | SEG_F | SEG_G };
TM1637Display display(TM1637_CLK, TM1637_DIO);
uint8_t displayState = 0;
#endif

// SSD1306 I2C display
#ifdef SSD1306DISPLAY_ENABLE
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#define I2C_ADDRESS 0x3C
#define RST_PIN -1

const uint8_t* font = fixed_bold10x15;  // 10x15 pix
//const uint8_t* font = cp437font8x8;  // 8*8 pix
//const uint8_t* font = Adafruit5x7;  // 5*7 pix
SSD1306AsciiWire oled;
#endif
#endif

#ifdef ADC_ENABLE
uint16_t getAdc()
{
	float averageValue = 0;
	const uint16_t measurementsToAverage = 16;
	for (uint16_t i = 0; i < measurementsToAverage; ++i)
	{
		averageValue += (float)analogRead(A0);
		delay(1);
		yield();
	}
	averageValue /= measurementsToAverage;
	return (uint16_t)averageValue;
}
#endif

// SERVICES

// Sleep management
#ifdef SLEEP_ENABLE
uint32_t activeTimeOut_ms = uint32_t(2UL * connectTimeLimit);
uint32_t sleepTimeOut_us = uint32_t(20UL * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)
uint32_t activeStart;
bool sleepEnable = false;
#endif

// Event  service
#ifdef EVENTS_ENABLE
bool eventsFlags[eventsNumber];
bool eventsEnable = false;

String getEvent(uint8_t n)
{
	uint16_t singleEventLength = EVENTS_TABLE_size / eventsNumber;
	String eventStr = readConfigString((EVENTS_TABLE_addr + n * singleEventLength), singleEventLength);
	eventStr.trim();
	return eventStr;
}

void processEvent(uint8_t eventNum)
{
	String event = getEvent(eventNum);
	if (eventsFlags[eventNum]) return;
	//conditions:
	//	value<=>x; adc, temp, hum, co2,
	//	input?=0/1/c;
	//	output?=0/1/c;
	//	counter?>x;
	String condition = event.substring(0, event.indexOf(':'));
	condition.trim();
	if (condition.length() <= 0) return;
	condition.toLowerCase();
	event = event.substring(event.indexOf(':') + 1);
	if (condition.startsWith(F("input")))
	{
		if (INPUT_PINS > 0)
		{
			int t = condition.indexOf('=');
			if (t >= 6 && t < condition.length() - 1)
			{
				uint8_t outNum = condition.substring(5, t).toInt();
				String outStateStr = condition.substring(t + 1);
				bool outState = OUT_OFF;
				const String cTxt = F("c");
				if (outStateStr != cTxt)
				{
					if (outStateStr == SWITCH_ON) outState = OUT_ON;
					else if (outStateStr == SWITCH_OFF) outState = OUT_OFF;
					else outState = outStateStr.toInt();
				}
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (outStateStr == cTxt)
					{
						if (inputs[outNum - 1] != digitalRead(pins[outNum - 1]))
						{
							inputs[outNum - 1] = digitalRead(pins[outNum - 1]);
							ProcessAction(event, eventNum, true);
						}
					}
					else if (outState == digitalRead(pins[outNum - 1])) ProcessAction(event, eventNum, true);
				}
			}
		}
	}
	else if (condition.startsWith(F("output")))
	{
		if (OUTPUT_PINS > 0)
		{
			int t = condition.indexOf('=');
			if (t >= 7 && t < condition.length() - 1)
			{
				uint8_t outNum = condition.substring(6, t).toInt();
				String outStateStr = condition.substring(t + 1);
				uint16_t outState = OUT_OFF;
				const String cTxt = F("c");
				if (outStateStr != cTxt)
				{
					if (outStateStr == SWITCH_ON) outState = OUT_ON;
					else if (outStateStr == SWITCH_OFF) outState = OUT_OFF;
					else outState = outStateStr.toInt();
				}
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (outStateStr == cTxt)
					{
						if (outputs[outNum - 1] != digitalRead(pins[outNum - 1]))
						{
							ProcessAction(event, eventNum, true);
						}
					}
					else if (outState == digitalRead(pins[outNum - 1])) ProcessAction(event, eventNum, true);
				}
				else
				{
					//Serial.print(F("\r\nIncorrect output: "));
					//Serial.println(String(outNum));
				}
			}
		}
	}
	else if (condition.startsWith(F("counter")))
	{
		if (INTERRUPT_PINS > 0)
		{
			int t = condition.indexOf('>');
			if (t >= 8 && t < condition.length() - 1)
			{
				uint8_t outNum = condition.substring(7, t).toInt();
				String outStateStr = condition.substring(t + 1);
				uint32_t outState = outStateStr.toInt();
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (InterruptCounter[outNum - 1] > outState) ProcessAction(event, eventNum, true);
				}
			}
		}
	}

#ifdef ADC_ENABLE
	else if (condition.startsWith(F("adc")) && condition.length() > 4)
	{
		uint16_t adcValue = getAdc();
		char operation = condition[3];
		uint16_t value = condition.substring(4).toInt();
		if ((operation == '>' && adcValue > value) || (operation == '<' && adcValue < value))
		{
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#ifdef TEMPERATURE_SENSOR
	else if (condition.startsWith(F("temperature")) && condition.length() > 12)
	{
		char oreration = condition[11];
		int value = condition.substring(12).toInt();
		sensorDataCollection sensorData = collectData();
		int temperatureNow = (int)getTemperature(sensorData);
		if (oreration == '>' && temperatureNow > value)
		{
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && temperatureNow < value)
		{
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#ifdef HUMIDITY_SENSOR
	else if (condition.startsWith(F("humidity")) && condition.length() > 9)
	{
		char oreration = condition[8];
		int value = condition.substring(9).toInt();
		sensorDataCollection sensorData = collectData();
		int humidityNow = (int)getTemperature(sensorData);
		if (oreration == '>' && humidityNow > value)
		{
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && humidityNow < value)
		{
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#ifdef CO2_SENSOR
	else if (condition.startsWith(F("co2")) && condition.length() > 4)
	{
		char oreration = condition[3];
		int value = condition.substring(4).toInt();
		sensorDataCollection sensorData = collectData();
		int co2Now = getTemperature(sensorData);
		if (oreration == '>' && co2Now > value)
		{
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && co2Now < value)
		{
			ProcessAction(event, eventNum, true);
		}
	}
#endif
}

String printHelpEvent()
{
	return F("Events:\r\n"
		"input?=[on, off, c]\r\n"
		"output?=[on, off, c]\r\n"
		"counter>n\r\n"
		"adc<>n\r\n"
		"temperature<>n\r\n"
		"humidity<>n\r\n"
		"co2<>n\r\n");
}
#endif

// Scheduler service
#ifdef SCHEDULER_ENABLE
bool schedulerEnable = false;

String getSchedule(uint8_t n)
{
	uint16_t singleScheduleLength = SCHEDULER_TABLE_size / schedulesNumber;
	String scheduleStr = readConfigString(SCHEDULER_TABLE_addr + n * singleScheduleLength, singleScheduleLength);
	scheduleStr.trim();
	return scheduleStr;
}

void writeScheduleExecTime(uint8_t scheduleNum, uint32_t execTime)
{
	uint16_t singleScheduleLength = SCHEDULER_LASTRUN_TABLE_size / schedulesNumber;
	writeConfigLong(SCHEDULER_LASTRUN_TABLE_addr + scheduleNum * singleScheduleLength, execTime);
}

uint32_t readScheduleExecTime(uint8_t scheduleNum)
{
	uint16_t singleScheduleLength = SCHEDULER_LASTRUN_TABLE_size / schedulesNumber;
	uint32_t scheduleTime = readConfigLong(SCHEDULER_LASTRUN_TABLE_addr + scheduleNum * singleScheduleLength);
	return scheduleTime;
}

String printHelpSchedule()
{
	return F("Schedule type:\r\n"
		"daily@hh:mm;action1;action2;...\r\n"
		"weekly@wd.hh:mm;action1;action2;...\r\n"
		"monthly@md.hh:mm;action1;action2;...\r\n"
		"once@yyyy.mm.dd.hh:mm;action1;action2;...\r\n");
}

void processSchedule(uint8_t scheduleNum)
{
	//conditions:
	//	start daily - daily@hh:mm;action1;action2;...
	//	start weekly - weekly@week_day.hh:mm;action1;action2;...
	//	monthly - monthly@month_day.hh:mm;action1;action2;...
	//	start date - date@yyyy.mm.dd.hh:mm;action1;action2;...

	String schedule = getSchedule(scheduleNum);

	if (schedule.length() == 0) return;
	int t = schedule.indexOf('@');
	int t1 = schedule.indexOf(';');

	if (t <= 0 || t1 <= 0 || t1 < t) return;

	String condition = schedule.substring(0, t + 1);
	condition.trim();
	condition.toLowerCase();

	String time = schedule.substring(t + 1, t1);
	time.trim();
	time.toLowerCase();
	schedule = schedule.substring(t1 + 1);

	uint32_t execTime = readScheduleExecTime(scheduleNum);

	if (condition.startsWith(F("daily@")) && time.length() == 5)
	{
		uint8_t _hr = time.substring(0, 2).toInt();
		uint8_t _min = time.substring(3, 5).toInt();

		if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
		{
			tmElements_t plan;
			plan.Year = year() - 1970; // offset from 1970; 
			plan.Month = month();
			plan.Day = day();
			plan.Hour = _hr;
			plan.Minute = _min;
			plan.Second = 0;
			uint32_t plannedTime = makeTime(plan);
			if (readScheduleExecTime(scheduleNum) < plannedTime && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
		}
	}
	else if (condition.startsWith(F("weekly@")) && time.length() == 7)
	{
		uint8_t _weekDay = time.substring(0, 1).toInt();
		uint8_t _hr = time.substring(2, 4).toInt();
		uint8_t _min = time.substring(5, 7).toInt();

		tmElements_t plan;
		plan.Year = year() - 1970; // offset from 1970; 
		plan.Month = month();
		plan.Day = day() - 1;
		plan.Hour = _hr;
		plan.Minute = _min;
		plan.Second = 0;
		uint32_t plannedTime = makeTime(plan);

		if (_weekDay >= 1 && _weekDay <= 7 && _hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
		{
			if (readScheduleExecTime(scheduleNum) < plannedTime && weekday() == _weekDay && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
		}
	}
	else if (condition.startsWith(F("monthly@")) && time.length() == 8)
	{
		uint8_t _day = time.substring(0, 2).toInt();
		uint8_t _hr = time.substring(3, 5).toInt();
		uint8_t _min = time.substring(6, 8).toInt();

		if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60 && _day >= 1 && _day <= 31)
		{
			tmElements_t plan;
			plan.Year = year() - 1970; // offset from 1970; 
			plan.Month = month();
			plan.Day = _day;
			plan.Hour = _hr;
			plan.Minute = _min;
			plan.Second = 0;
			uint32_t plannedTime = makeTime(plan);
			if (readScheduleExecTime(scheduleNum) < plannedTime && day() >= _day && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
		}
	}
	else if (condition.startsWith(F("once@")) && time.length() == 16 && execTime == 0)
	{
		uint8_t _yr = time.substring(0, 4).toInt();
		uint8_t _month = time.substring(5, 7).toInt();
		uint8_t _day = time.substring(8, 10).toInt();
		uint8_t _hr = time.substring(11, 13).toInt();
		uint8_t _min = time.substring(14, 16).toInt();

		if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60 && _day >= 1 && _day <= 31 && _month >= 1 && _month <= 12)
		{
			if (year() >= _yr && month() >= _month && day() >= _day && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
		}
	}
}
#endif

// SMTP
#ifdef SMTP_ENABLE
#include <sendemail.h>

bool smtpEnable = false;

bool sendMail(String& subject, String& message, String& addressTo)
{
	String smtpServerAddress = readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	uint16_t smtpServerPort = readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size).toInt();
	String smtpLogin = readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	String smtpPassword = readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	if (addressTo.length() <= 0) addressTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);

	bool result = false;
	if (smtpEnable && WiFi.status() == WL_CONNECTED)
	{
		SendEmail smtp(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 20000, true);
		result = smtp.send(smtpLogin, addressTo, subject, message);
	}
	return result;
}
#endif

// MQTT
#ifdef MQTT_ENABLE
#include <PubSubClient.h>

#define MQTT_MAX_PACKET 100

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
String mqttCommand = "";
bool mqttEnable = false;

String getMqttServer()
{
	return readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
}

int getMqttPort()
{
	return readConfigString(MQTT_PORT_addr, MQTT_PORT_size).toInt();
}

String getMqttUser()
{
	return readConfigString(MQTT_USER_addr, MQTT_USER_size);
}

String getMqttPassword()
{
	return readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
}

String getMqttDeviceId()
{
	return readConfigString(MQTT_ID_addr, MQTT_ID_size);
}

String getMqttTopicIn()
{
	return readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
}

String getMqttTopicOut()
{
	return readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
}

bool getMqttClean()
{
	bool mqttClean = false;
	if (readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size) == SWITCH_ON_NUMBER) mqttClean = true;
	return mqttClean;
}

bool mqtt_connect()
{
	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		IPAddress mqtt_ip_server;
		String mqtt_server = getMqttServer();
		uint16_t mqtt_port = getMqttPort();
		String mqtt_User = getMqttUser();
		String mqtt_Password = getMqttPassword();
		String mqtt_device_id = getMqttDeviceId();
		String mqtt_topic_in = getMqttTopicIn();
		bool mqttClean = getMqttClean();

		if (mqtt_ip_server.fromString(mqtt_server))
		{
			mqtt_server = "";
			mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else
		{
			mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
		}

		mqtt_client.setCallback(mqtt_callback);

		if (mqtt_device_id.length() <= 0)
		{
			mqtt_device_id = deviceName;
			mqtt_device_id += "_";
			mqtt_device_id += WiFi.macAddress();
		}

		if (mqtt_User.length() > 0)
		{
			result = mqtt_client.connect(mqtt_device_id.c_str(), mqtt_User.c_str(), mqtt_Password.c_str(), "", 0, false, "", mqttClean);
		}
		else
		{
			result = mqtt_client.connect(mqtt_device_id.c_str());
		}
		if (!result)
		{
			return result;
		}
		result = mqtt_client.subscribe(mqtt_topic_in.c_str());
	}
	return result;
}

bool mqtt_send(String& message, int dataLength, String topic)
{
	if (!mqtt_client.connected())
	{
		mqtt_connect();
	}

	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		if (topic.length() <= 0) topic = getMqttTopicOut();

		if (dataLength > MQTT_MAX_PACKET)
		{
			result = mqtt_client.beginPublish(topic.c_str(), dataLength, false);
			for (uint16_t i = 0; i < dataLength; i++)
			{
				mqtt_client.write(message[i]);
				yield();
			}
			result = mqtt_client.endPublish();
		}
		else
		{
			result = mqtt_client.publish(topic.c_str(), message.c_str(), dataLength);
		}
	}
	return result;
}

void mqtt_callback(char* topic, uint8_t* payload, uint16_t dataLength)
{
	for (uint16_t i = 0; i < dataLength; i++)
	{
		mqttCommand += char(payload[i]);
		yield();
	}
}
#endif

// Telegram
#ifdef TELEGRAM_ENABLE
//#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "CTBot.h"

#define TELEGRAM_MESSAGE_MAX_SIZE 300
#define TELEGRAM_MESSAGE_DELAY 2000
#define TELEGRAM_RETRIES 3
#define telegramMessageBufferSize  10

CTBot myBot;
struct telegramMessage
{
	uint8_t retries;
	int64_t user;
	String message;
};
//int64_t telegramUsers[telegramUsersNumber];
telegramMessage telegramOutboundBuffer[telegramMessageBufferSize];
uint8_t telegramOutboundBufferPos = 0;
uint32_t telegramLastTime = 0;
bool telegramEnable = false;

String uint64ToString(uint64_t input)
{
	String result = "";
	uint8_t base = 10;
	do
	{
		char c = input % base;
		input /= base;
		if (c < 10) c += '0';
		else c += 'A' - 10;
		result = c + result;
		yield();
	} while (input);
	return result;
}

uint64_t StringToUint64(String value)
{
	for (uint8_t i = 0; i < value.length(); i++)
	{
		if (value[i] < '0' || value[i]>'9') return 0;
	}
	uint64_t result = 0;
	const char* p = value.c_str();
	const char* q = p + value.length();
	while (p < q)
	{
		result = (result << 1) + (result << 3) + *(p++) - '0';
		yield();
	}
	return result;
}

bool sendToTelegramServer(int64_t user, String message)
{
	bool result = false;
	if (telegramEnable && WiFi.status() == WL_CONNECTED)
	{
		result = myBot.testConnection();
		if (result)
		{
			result = myBot.sendMessage(user, message);
			if (result)
			{
				result = true;
			}
		}
	}
	return result;
}

void addMessageToTelegramOutboundBuffer(int64_t user, String message, uint8_t retries)
{
	if (telegramOutboundBufferPos >= telegramMessageBufferSize)
	{
		for (uint8_t i = 0; i < telegramMessageBufferSize - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
			yield();
		}
		telegramOutboundBufferPos = telegramMessageBufferSize - 1;
	}
	telegramOutboundBuffer[telegramOutboundBufferPos].user = user;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = message;
	telegramOutboundBuffer[telegramOutboundBufferPos].retries = retries;
	telegramOutboundBufferPos++;
}

void removeMessageFromTelegramOutboundBuffer()
{
	if (telegramOutboundBufferPos <= 0) return;
	for (uint8_t i = 0; i < telegramOutboundBufferPos; i++)
	{
		if (i < telegramMessageBufferSize - 1) telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		else
		{
			telegramOutboundBuffer[i].message = "";
			telegramOutboundBuffer[i].user = 0;
		}
		yield();
	}
	telegramOutboundBufferPos--;
}

void sendToTelegram(int64_t user, String message, uint8_t retries)
{
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= TELEGRAM_MESSAGE_MAX_SIZE)
		{
			addMessageToTelegramOutboundBuffer(user, message.substring(0, TELEGRAM_MESSAGE_MAX_SIZE), retries);
			message = message.substring(TELEGRAM_MESSAGE_MAX_SIZE);
		}
		else
		{
			addMessageToTelegramOutboundBuffer(user, message, retries);
			message = "";
		}
		yield();
	}
}

void sendBufferToTelegram()
{
	if (telegramEnable && WiFi.status() == WL_CONNECTED && myBot.testConnection())
	{
		bool isRegisteredUser = false;
		bool isAdmin = false;
		TBMessage msg;
		while (myBot.getNewMessage(msg))
		{
			//consider 1st sender an admin if admin is empty
			if (getTelegramUser(0) != 0)
			{
				//check if it's registered user
				for (uint8_t i = 0; i < telegramUsersNumber; i++)
				{
					if (getTelegramUser(i) == msg.sender.id)
					{
						isRegisteredUser = true;
						if (i == 0) isAdmin = true;
						break;
					}
					yield();
				}
			}
			//process data from TELEGRAM
			if (isRegisteredUser && msg.text.length() > 0)
			{
				//sendToTelegram(msg.sender.id, deviceName + ": " + msg.text, telegramRetries);
				String str = processCommand(msg.text, CHANNEL_TELEGRAM, isAdmin);
				sendToTelegram(msg.sender.id, deviceName + ": " + str, TELEGRAM_RETRIES);
			}
			yield();
		}

		if (telegramOutboundBuffer[0].message.length() != 0)
		{
			if (sendToTelegramServer(telegramOutboundBuffer[0].user, telegramOutboundBuffer[0].message))
			{
				removeMessageFromTelegramOutboundBuffer();
			}
			else
			{
				telegramOutboundBuffer[0].retries--;
				if (telegramOutboundBuffer[0].retries <= 0)
				{
					removeMessageFromTelegramOutboundBuffer();
				}
			}
		}
	}
}

uint64_t getTelegramUser(uint8_t n)
{
	uint16_t singleUserLength = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	String userStr = readConfigString((TELEGRAM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return StringToUint64(userStr);
}
#endif

// GSM modem
#ifdef GSM_ENABLE

#ifdef GSM_M590_ENABLE
#define SMS_TIME_OUT          10000
#define SMS_CHECK_TIME_OUT    30000
#define AT_COMMAND_INIT1      F("ATE0")              //disable echo
#define AT_COMMAND_INIT2      F("AT+CNMI=0,0,0,0,0") //Set message indication Format
#define AT_COMMAND_INIT3      F("AT+CMGF=1")         //set text mode
#define AT_COMMAND_INIT4      F("AT+CSCS=\"GSM\"")   //Set "GSM” character set
#define AT_COMMAND_STATUS1    F("AT+CPAS")         //Check module’s status
#define AT_COMMAND_STATUS2    F("AT+CREG?")        //Check network registration status
#define AT_COMMAND_STATUS3    F("AT+CSQ")          //Signal intensity
#define AT_COMMAND_STATUS4    F("AT+CPMS?")          //Get SMS count
#define AT_COMMAND_SEND       F("AT+CMGS=\"")         //Message sending
#define AT_COMMAND_GET_SMS    F("AT+CMGR=")         //get sms message #
#define AT_COMMAND_DELETE_SMS F("AT+CMGD=")       //Delete message #
#endif

#ifdef GSM_SIM800_ENABLE
#define SMS_TIME_OUT          10000
#define SMS_CHECK_TIME_OUT    30000
#define AT_COMMAND_INIT1      F("ATE0")              //disable echo
#define AT_COMMAND_INIT2      F("AT+CNMI=0,0,0,0,0") //Set message indication Format
#define AT_COMMAND_INIT3      F("AT+CMGF=1")         //set text mode
#define AT_COMMAND_INIT4      F("AT+CSCS=\"GSM\"")   //Set "GSM” character set
#define AT_COMMAND_STATUS1    F("AT+CPAS")         //Check module’s status
#define AT_COMMAND_STATUS2    F("AT+CREG?")        //Check network registration status
#define AT_COMMAND_STATUS3    F("AT+CSQ")          //Signal intensity
#define AT_COMMAND_STATUS4    F("AT+CPMS?")        //Get SMS count
#define AT_COMMAND_SEND       F("AT+CMGS=\"")          //Message sending
#define AT_COMMAND_GET_SMS    F("AT+CMGR=")         //get sms message #
#define AT_COMMAND_DELETE_SMS F("AT+CMGD=")       //Delete message #
#endif

uint32_t gsmTimeOut = 0;
uint32_t gsmUartSpeed = 19200;
bool gsmEnable = false;

String getGsmUser(uint8_t n)
{
	const uint8_t singleUserLength = GSM_USERS_TABLE_size / gsmUsersNumber;
	String userStr = readConfigString((GSM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}

String sendATCommand(String cmd, bool waiting)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	uart2.flush();
	String _resp = "";
	uart2.println(cmd);
	//Serial.println(">>" + cmd);
	if (waiting)
	{
		const uint32_t _timeout = millis() + SMS_TIME_OUT;
		while (!uart2.available() && millis() < _timeout)
		{
			yield();
		}
		while (uart2.available())
		{
			_resp += uart2.readString();
			yield();
		}
		//Serial.println("<<" + _resp);
		if (_resp.length() <= 0)
		{
			//Serial.println("Timeout...");
		}

		if (stopSerial) uart2.end();
		return _resp;
	}
	return _resp;
}

bool initModem()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	bool resp = true;
	String response = sendATCommand(AT_COMMAND_INIT1, true);
	if (response.lastIndexOf(F("OK")) <= 0) resp &= false;
	response = sendATCommand(AT_COMMAND_INIT2, true);
	if (response.lastIndexOf(F("OK")) <= 0) resp &= false;
	response = sendATCommand(AT_COMMAND_INIT3, true);
	if (response.lastIndexOf(F("OK")) <= 0) resp &= false;
	response = sendATCommand(AT_COMMAND_INIT4, true);
	if (response.lastIndexOf(F("OK")) <= 0) resp &= false;
	if (stopSerial) uart2.end();

	return resp;
}

bool sendSMS(String phone, String& smsText)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	initModem();
	String cmdTmp = AT_COMMAND_SEND;
	cmdTmp.reserve(175);
	cmdTmp += phone;
	cmdTmp += F("\"");
	sendATCommand(cmdTmp, true);
	cmdTmp = "";
	if (smsText.length() >= 170) cmdTmp += smsText.substring(0, 168);
	else cmdTmp += smsText;
	//Serial.println("Sending sms to " + phone + ":\'" + cmdTmp + "\'");
	//Serial.println(String(cmdTmp.length()));
	for (uint16_t i = 0; i < cmdTmp.length(); i++)
	{
		uart2.write((char)cmdTmp[i]);
		delay(50);
		yield();
	}
	delay(500);
	yield();
	uart2.flush();
	uart2.write((char)26);
	yield();
	cmdTmp = "";
	const uint32_t _timeout = millis() + SMS_TIME_OUT;
	while (!uart2.available() && millis() < _timeout)
	{
		yield();
	}
	while (uart2.available())
	{
		cmdTmp += uart2.readString();
		yield();
	}
	if (cmdTmp.length() <= 0)
	{
		//Serial.println("Timeout...");
	}

	if (stopSerial) uart2.end();

	if (cmdTmp.lastIndexOf(F("OK")) > 0) return true;
	return false;
}

smsMessage parseSMS(String msg)
{
	String msgheader = "";
	String msgbody = "";
	String msgphone = "";

	msg = msg.substring(msg.indexOf(F("+CMGR: ")));
	msgheader = msg.substring(0, msg.indexOf("\r"));

	msgbody = msg.substring(msgheader.length() + 2);
	msgbody = msgbody.substring(0, msgbody.lastIndexOf(F("OK")));
	msgbody.trim();

	int firstIndex = msgheader.indexOf(F("\",\"")) + 3;
	int secondIndex = msgheader.indexOf(F("\",\""), firstIndex);
	msgphone = msgheader.substring(firstIndex, secondIndex);
	if (msgphone.length() <= 0) msgphone = F(" ");

	smsMessage response;
	response.PhoneNumber = msgphone;
	for (uint8_t i = 0; i < gsmUsersNumber; i++)
	{
		String gsmUser = getGsmUser(i);
		if (msgphone == gsmUser)
		{
			if (i == 0) response.IsAdmin = true;
			response.Message = msgbody;
			break;
		}
		yield();
	}

	return response;
}

smsMessage getSMS()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	initModem();
	//getModemStatus();
	smsMessage sms;
	for (int i = 1; i <= 15; i++)
	{
		const String cmd = String(AT_COMMAND_GET_SMS) + String(i);
		String _response = sendATCommand(cmd, true);
		_response.trim();
		if (_response.lastIndexOf(F("OK")) > 0)
		{
			deleteSMS(i);
			sms = parseSMS(_response);
			break;
		}
		yield();
	}
	//Serial.println("Sms: " + sms.Message);
	//Serial.println("From: " + sms.PhoneNumber);
	if (stopSerial) uart2.end();

	return sms;
}

bool deleteSMS(int n)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	const String cmd = String(AT_COMMAND_DELETE_SMS) + String(n);
	const String response = sendATCommand(cmd, true);
	if (stopSerial) uart2.end();
	if (response.lastIndexOf(F("OK")) > 0) return true;
	else return false;
}

String getModemStatus()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 64);
		stopSerial = true;
	}

	String response = sendATCommand(AT_COMMAND_STATUS1, true);
	response += sendATCommand(AT_COMMAND_STATUS2, true);
	response += sendATCommand(AT_COMMAND_STATUS3, true);
	response += sendATCommand(AT_COMMAND_STATUS4, true);

	if (stopSerial) uart2.end();

	//if (response.lastIndexOf(F("OK")) > 0) return true;
	//else return false;
	return response;
}
#endif

// Google script
#ifdef GSCRIPT_ENABLE
#include <HTTPSRedirect.h>

bool gScriptEnable = false;

bool sendValueToGoogle(String& value)
{
	bool flag = false;
	if (gScriptEnable && WiFi.status() == WL_CONNECTED)
	{
		const char* host = "script.google.com";
		//const char* googleRedirHost = "script.googleusercontent.com";
		//const char* fingerprint = "C2 00 62 1D F7 ED AF 8B D1 D5 1D 24 D6 F0 A1 3A EB F1 B4 92";
		//String[] valuesNames = {"?tag", "?value="};
		String gScriptId = readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
		const uint16_t httpsPort = 443;

		HTTPSRedirect* gScriptClient = new HTTPSRedirect(httpsPort);
		gScriptClient->setInsecure();
		gScriptClient->setPrintResponseBody(true);

		for (uint8_t i = 0; i < 5; i++)
		{
			int retval = gScriptClient->connect(host, httpsPort);
			if (retval == 1)
			{
				flag = true;
				break;
			}
			//else Serial.println(F("Connection failed. Retrying..."));
			yield();
		}
		if (flag)
		{
			String urlFinal = F("/macros/s/");
			urlFinal += gScriptId;
			urlFinal += F("/exec");
			urlFinal += F("?value=");
			urlFinal += value;
			flag = gScriptClient->GET(urlFinal, host, true);
			if (flag)
			{
				gScriptClient->stopAll;
				gScriptClient->~HTTPSRedirect();
			}
		}
	}
	return flag;
}
#endif

// PushingBox
#ifdef PUSHINGBOX_ENABLE
#define PUSHINGBOX_SUBJECT F("device_name")

const uint16_t pushingBoxMessageMaxSize = 1000;
const char* pushingBoxServer = "api.pushingbox.com";
bool pushingBoxEnable = false;

bool sendToPushingBoxServer(String message)
{
	bool flag = false;
	if (pushingBoxEnable && WiFi.status() == WL_CONNECTED)
	{
		WiFiClient client;

		if (client.connect(pushingBoxServer, 80))
		{
			String pushingBoxId = readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
			String pushingBoxParameter = readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);

			String postStr = F("devid=");
			postStr += pushingBoxId;
			postStr += F("&");
			postStr += PUSHINGBOX_SUBJECT;
			postStr += F("=");
			postStr += deviceName;
			postStr += F("&");
			postStr += pushingBoxParameter;
			postStr += F("=");
			postStr += String(message);
			postStr += F("\r\n\r\n");

			client.print(F("POST /pushingbox HTTP/1.1\n"));
			client.print(F("Host: "));
			client.print(String(pushingBoxServer));
			client.print(F("\nConnection: close\n"));
			client.print(F("Content-Type: application/x-www-form-urlencoded\n"));
			client.print(F("Content-Length: "));
			client.print(postStr.length());
			client.print(F("\n\n"));
			client.print(postStr);
			// check server reply
			flag = true;
		}
		client.stop();
	}
	return flag;
}

bool sendToPushingBox(String& message)
{
	bool result = true;
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= pushingBoxMessageMaxSize)
		{
			result &= sendToPushingBoxServer(message.substring(0, pushingBoxMessageMaxSize));
			message = message.substring(pushingBoxMessageMaxSize);
		}
		else
		{
			result &= sendToPushingBoxServer(message);
			message = "";
		}
		yield();
	}
	return result;
}
#endif

// HTTP server
#ifdef HTTP_SERVER_ENABLE
#include <ESP8266WebServer.h>

uint16_t httpPort = 80;
ESP8266WebServer http_server(httpPort);
bool httpServerEnable = false;

void handleRoot()
{
	sensorDataCollection sensorData = collectData();
	String str = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: ");
	str += String(uint16_t(checkSensorPeriod / 1000UL));
	str += F("\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
	//str += ParseSensorReport(history_log[history_record_number - 1], F("<br>"), false);
	str += ParseSensorReport(sensorData, F("<br>"), false);
	str += F("<br />\r\n</html>\r\n");
	http_server.sendContent(str);
}

void handleNotFound()
{
	String message = F("File Not Found\n\nURI: ");
	message += http_server.uri();
	message += F("\nMethod: ");
	message += (http_server.method() == HTTP_GET) ? F("GET") : F("POST\nArguments: ");
	message += http_server.args();
	message += EOL;
	for (uint8_t i = 0; i < http_server.args(); i++)
	{
		message += F(" ");
		message += http_server.argName(i);
		message += F(": ");
		message += http_server.arg(i);
		message += EOL;
		yield();
	}
	http_server.send(404, F("text/plain"), message);
}
#endif

// NTP Server
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>

#define NTP_PACKET_SIZE 48 // NTP time is in the first 48 bytes of message
#define UDP_LOCAL_PORT 8888  // local port to listen for UDP packets

WiFiUDP Udp;
int ntpRefreshDelay = 180;
bool NTPenable = false;

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
	uint8_t packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

time_t getNtpTime()
{
	if (!NTPenable || WiFi.status() != WL_CONNECTED) return 0;

	while (Udp.parsePacket() > 0) yield(); // discard any previously received packets

	IPAddress timeServerIP(129, 6, 15, 28); //129.6.15.28 = time-a.nist.gov

	String ntpServer = readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	const int timeZone = readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size).toInt();


	if (!timeServerIP.fromString(ntpServer))
	{
		if (WiFi.hostByName(ntpServer.c_str(), timeServerIP) != 1)
		{
			timeServerIP = IPAddress(129, 6, 15, 28);
		}
	}

	sendNTPpacket(timeServerIP);
	const uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		const int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			uint8_t packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			timeIsSet = true;
			// convert four bytes starting at location 40 to a long integer
			uint32_t secsSince1900 = (uint32_t)packetBuffer[40] << 24;
			secsSince1900 |= (uint32_t)packetBuffer[41] << 16;
			secsSince1900 |= (uint32_t)packetBuffer[42] << 8;
			secsSince1900 |= (uint32_t)packetBuffer[43];
			NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
		yield();
	}
	return 0; // return 0 if unable to get the time
}
#endif

// Log history
#ifdef LOG_ENABLE
// Log settings for 32 hours of storage
#define LOG_SIZE 120 // Save sensors value each 10 minutes: ( 32hours * 60minutes/hour ) / 10minutes/pcs = 192 pcs

sensorDataCollection history_log[LOG_SIZE];
uint16_t history_record_number = 0;
uint32_t logLastTime = 0;
//uint32_t logPeriod = 60 / (LOG_SIZE / 32) * 60 * 1000 / 5; // 60minutes/hour / ( 192pcs / 32hours ) * 60seconds/minute * 1000 msec/second
uint32_t logPeriod = 600000UL;

void addToLog(sensorDataCollection& record)
{
	history_log[history_record_number] = record;
	history_record_number++;

	//if log is full - roll back to 0
	if (history_record_number >= LOG_SIZE)
	{
		history_record_number = 0;
	}
}
#endif

void setup()
{
	Serial.begin(115200);

	deviceName.reserve(DEVICE_NAME_size + 1);
	uartCommand.reserve(257);

	//mark signal pins to avoid using them as discrete I/O
#ifdef I2C_ENABLE
	if (!add_signal_pin(PIN_WIRE_SDA))
	{
		Serial.println(F("Error setting I2C SDA pin to "));
		Serial.println(String(PIN_WIRE_SDA));
	}
	if (!add_signal_pin(PIN_WIRE_SCL))
	{
		Serial.println(F("Error setting I2C SCL pin to "));
		Serial.println(String(PIN_WIRE_SCL));
	}

	Wire.begin(PIN_WIRE_SDA, PIN_WIRE_SCL);
#endif

#ifdef UART2_ENABLE
	if (!add_signal_pin(UART2_TX))
	{
		Serial.println(F("Error setting UART2 TX pin to "));
		Serial.println(String(UART2_TX));
	}
	if (!add_signal_pin(UART2_RX))
	{
		Serial.println(F("Error setting UART2 RX pin to "));
		Serial.println(String(UART2_RX));
	}

#ifdef MH_Z19_UART_ENABLE
	uart2_Speed = mhz19UartSpeed;
#endif

#ifdef GSM_ENABLE
	uart2_Speed = gsmUartSpeed;
#endif

	//uart2.begin(uart2_Speed, SWSERIAL_8N1, UART2_TX, UART2_RX, false, 256);
#endif

#ifdef ONEWIRE_ENABLE
	if (!add_signal_pin(ONEWIRE_DATA))
	{
		Serial.println(F("Error setting ONEWIRE pin to "));
		Serial.println(String(ONEWIRE_DATA));
	}
#endif

#ifdef DHT_ENABLE
	if (!add_signal_pin(DHT_PIN))
	{
		Serial.println(F("Error setting DHT pin to "));
		Serial.println(String(DHT_PIN));
	}
#endif

#ifdef TM1637DISPLAY_ENABLE
	if (!add_signal_pin(TM1637_CLK))
	{
		Serial.println(F("Error setting TM1637 CLK pin to "));
		Serial.println(String(TM1637_CLK));
	}
	if (!add_signal_pin(TM1637_DIO))
	{
		Serial.println(F("Error setting TM1637 DIO pin to "));
		Serial.println(String(TM1637_DIO));
	}
#endif

#ifdef MH_Z19_PPM_ENABLE
	if (!add_signal_pin(MH_Z19_PPM_ENABLE))
	{
		Serial.println(F("Error setting MH-Z19 PPM pin to "));
		Serial.println(String(MH_Z19_PPM_ENABLE));
	}
#endif

#ifdef MQTT_ENABLE
	mqttCommand.reserve(256);
#endif

#ifdef TELEGRAM_ENABLE
	for (uint8_t b = 0; b < telegramMessageBufferSize; b++)
	{
		telegramOutboundBuffer[0].message.reserve(TELEGRAM_MESSAGE_MAX_SIZE);
		yield();
	}
#endif

	//init EEPROM of certain size
	EEPROM.begin(CollectEepromSize() + 100);

	//clear EEPROM if it's just initialized
	if (EEPROM.read(0) == 0xff)
	{
		for (uint32_t i = 0; i < EEPROM.length(); i++)
		{
			EEPROM.write(i, 0);
			yield();
		}
	}

	connectTimeLimit = uint32_t(readConfigString(CONNECT_TIME_addr, CONNECT_TIME_size).toInt() * 1000UL);
	if (connectTimeLimit == 0) connectTimeLimit = 60000UL;
	reconnectPeriod = uint32_t(readConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size).toInt() * 1000UL);
	if (reconnectPeriod == 0) reconnectPeriod = 600000UL;

	deviceName = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);

	checkSensorPeriod = readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt() * 1000UL;
	autoReport = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();

#ifdef LOG_ENABLE
	logPeriod = readConfigString(LOG_PERIOD_addr, LOG_PERIOD_size).toInt() * 1000UL;
#endif

	// init WiFi
	startWiFi();

	uint16_t telnetPort = readConfigString(TELNET_PORT_addr, TELNET_PORT_size).toInt();
	telnetServer.begin(telnetPort);
	telnetServer.setNoDelay(true);

#ifdef EVENTS_ENABLE
	if (readConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size) == SWITCH_ON_NUMBER) eventsEnable = true;
#endif

#ifdef SCHEDULER_ENABLE
	if (readConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size) == SWITCH_ON_NUMBER) schedulerEnable = true;
#endif

	String tmpStr;
	tmpStr.reserve(101);

#ifdef MQTT_ENABLE
	if (readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size) == SWITCH_ON_NUMBER) mqttEnable = true;
#endif

#ifdef SMTP_ENABLE
	if (readConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size) == SWITCH_ON_NUMBER) smtpEnable = true;
#endif

#ifdef GSM_ENABLE
	if (readConfigString(GSM_ENABLE_addr, GSM_ENABLE_size) == SWITCH_ON_NUMBER) gsmEnable = true;
	if (gsmEnable)
	{
		initModem();
	}
#endif

#ifdef TELEGRAM_ENABLE
	String telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	if (readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size) == SWITCH_ON_NUMBER) telegramEnable = true;
	if (telegramEnable)
	{
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif

#ifdef GSCRIPT_ENABLE
	if (readConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size) == SWITCH_ON_NUMBER) gScriptEnable = true;
#endif

#ifdef PUSHINGBOX_ENABLE
	if (readConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size) == SWITCH_ON_NUMBER) pushingBoxEnable = true;
#endif

#ifdef HTTP_SERVER_ENABLE
	httpPort = readConfigString(HTTP_PORT_addr, HTTP_PORT_size).toInt();
	if (readConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size) == SWITCH_ON_NUMBER) httpServerEnable = true;
	http_server.on(F("/"), handleRoot);
	/*http_server.on(F("/inline"), []()
		{
		http_server.send(200, F("text/plain"), F("this works as well"));
		});*/
	http_server.onNotFound(handleNotFound);
	if (httpServerEnable) http_server.begin(httpPort);
#endif

#ifdef NTP_TIME_ENABLE
	ntpRefreshDelay = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt();
	if (readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size) == SWITCH_ON_NUMBER) NTPenable = true;
	if (NTPenable)
	{
		Udp.begin(UDP_LOCAL_PORT);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
	}
#endif

#ifdef AMS2320_ENABLE
	am2320 = Adafruit_AM2320(&Wire);
	if (!am2320.begin())
	{
		am2320_enable = false;
		Serial.println(F("Can't find AMS2320 sensor!"));
	}
#endif

#ifdef HTU21D_ENABLE
	htu21d = Adafruit_HTU21DF();
	if (!htu21d.begin())
	{
		htu21d_enable = false;
		Serial.println(F("Can't find HTU21D sensor!"));
	}
#endif

#ifdef BME280_ENABLE
	if (!bme280.begin(BME280_ADDRESS))
	{
		bme280_enable = false;
		Serial.println(F("Can't find BME280 sensor!"));
	}
#endif

#ifdef BMP180_ENABLE	
	if (!bmp180.begin())
	{
		bmp180_enable = false;
		Serial.println(F("Can't find BMP180 sensor!"));
	}
#endif

#ifdef DS18B20_ENABLE
	ds1820.begin();
	if (ds1820.getDeviceCount() <= 0)
	{
		ds1820_enable = false;
		Serial.println(F("Can't find DS18B20 sensor!"));
	}
#endif

#ifdef DHT_ENABLE
	dht.begin();
#endif

#ifdef MH_Z19_UART_ENABLE
	co2_uart_avg[0] = co2_uart_avg[1] = co2_uart_avg[2] = co2SerialRead();
#endif

#ifdef MH_Z19_PPM_ENABLE
	pinMode(MH_Z19_PPM_ENABLE, INPUT);
	co2_ppm_avg[0] = co2_ppm_avg[1] = co2_ppm_avg[2] = co2PPMRead();
#endif

#ifdef DISPLAY_ENABLED
	displaySwitchPeriod = uint32_t(readConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size).toInt() * 1000UL);

#ifdef TM1637DISPLAY_ENABLE
	display.setBrightness(1);
	display.showNumberDec(0, false);
#endif

#ifdef SSD1306DISPLAY_ENABLE
	i2c.setClock(400000L);
	oled.begin(&Adafruit128x64, I2C_ADDRESS);
	oled.setFont(font);
#endif
#endif

	//setting pin modes
	for (uint8_t num = 0; num < PIN_NUMBER; num++)
	{
		if (bitRead(SIGNAL_PINS, num)) continue; //pin reserved to data bus

		uint16_t l = PIN_MODE_size / PIN_NUMBER;
		uint8_t pinModeCfg = readConfigString(PIN_MODE_addr + num * l, l).toInt();

		if (pinModeCfg == INPUT || pinModeCfg == INPUT_PULLUP) // pin is input or input_pullup
		{
			bitSet(INPUT_PINS, num);
			pinMode(pins[num], pinModeCfg);
			inputs[num] = digitalRead(pins[num]);

			//setting interrupt modes
			l = INTERRUPT_MODE_size / PIN_NUMBER;
			uint8_t intModeCfg = readConfigString(INTERRUPT_MODE_addr + num * l, l).toInt();
			if (intModeCfg >= 1 && intModeCfg < intModeList->length())
			{
				bitSet(INTERRUPT_PINS, num);
				if (num == 0) attachInterrupt(pins[num], int1count, intModeCfg);
				else if (num == 1) attachInterrupt(pins[num], int2count, intModeCfg);
				else if (num == 2) attachInterrupt(pins[num], int3count, intModeCfg);
				else if (num == 3) attachInterrupt(pins[num], int4count, intModeCfg);
				else if (num == 4) attachInterrupt(pins[num], int5count, intModeCfg);
				else if (num == 5) attachInterrupt(pins[num], int6count, intModeCfg);
				else if (num == 6) attachInterrupt(pins[num], int7count, intModeCfg);
				else if (num == 7) attachInterrupt(pins[num], int8count, intModeCfg);
			}
		}
		else if (pinModeCfg == OUTPUT) // pin is output
		{
			bitSet(OUTPUT_PINS, num);
			pinMode(pins[num], pinModeCfg);
			tmpStr = F("set_output");
			tmpStr += String(num + 1);
			tmpStr += F("=");
			l = OUTPUT_INIT_size / PIN_NUMBER;
			tmpStr += readConfigString(OUTPUT_INIT_addr + num * l, l);
			set_output(tmpStr);
			outputs[num] = digitalRead(pins[num]);
		}

		yield();
	}

#ifdef SLEEP_ENABLE
	if (readConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size) == SWITCH_ON_NUMBER) sleepEnable = true;

	activeTimeOut_ms = uint32_t(readConfigString(SLEEP_ON_addr, SLEEP_ON_size).toInt() * 1000UL);
	if (activeTimeOut_ms == 0) activeTimeOut_ms = uint32_t(2 * connectTimeLimit);

	sleepTimeOut_us = uint32_t(readConfigString(SLEEP_OFF_addr, SLEEP_OFF_size).toInt() * 1000000UL);
	if (sleepTimeOut_us == 0) sleepTimeOut_us = uint32_t(20 * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)

	if (sleepEnable) activeStart = millis();
#endif	
}

void loop()
{
	//check if connection is present and reconnect
	if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_STA || wifi_mode == WIFI_MODE_AUTO)
	{
		//check if connection is lost
		if (wiFiIntendedStatus == RADIO_CONNECTED && WiFi.status() != WL_CONNECTED)
		{
			wiFiIntendedStatus = RADIO_CONNECTING;
			ConnectionStarted = millis();
		}

		//check if connection is established
		if (wiFiIntendedStatus == RADIO_CONNECTING && WiFi.status() == WL_CONNECTED)
		{
			/*Serial.print(F("STA connected to \""));
			Serial.print(String(STA_Ssid));
			Serial.println(F("\" AP."));
			Serial.print(F("IP: "));
			Serial.println(WiFi.localIP());*/
			wiFiIntendedStatus = RADIO_CONNECTED;
		}
	}
	yield();
	//check if connecting is too long and switch AP/STA
	if (wifi_mode == WIFI_MODE_AUTO)
	{
		//check connect to AP time limit and switch to AP if AUTO mode 
		if (wiFiIntendedStatus == RADIO_CONNECTING && ConnectionStarted + connectTimeLimit < millis())
		{
			Start_AP_Mode();
			wiFiIntendedStatus = RADIO_WAITING;
			WaitingStarted = millis();
		}

		//check if waiting time finished and no clients connected to AP
		if (wiFiIntendedStatus == RADIO_WAITING && WaitingStarted + reconnectPeriod < millis() && WiFi.softAPgetStationNum() <= 0)
		{
			Start_STA_Mode();
			wiFiIntendedStatus = RADIO_CONNECTING;
			ConnectionStarted = millis();
		}
	}
	yield();
	//check if clients connected/disconnected to AP
	if ((wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA) && APclientsConnected != WiFi.softAPgetStationNum())
	{
		/*if (APclientsConnected < WiFi.softAPgetStationNum())
		{
			Serial.println(F("AP client connected: "));
		}
		else
		{
			Serial.println(F("AP Client disconnected: "));
		}*/
		APclientsConnected = WiFi.softAPgetStationNum();
	}
	yield();
	//check if it's time to get sensors value
	if (millis() - checkSensorLastTime > checkSensorPeriod)
	{
		if (autoReport > 0)
		{
			sensorDataCollection sensorData = collectData();
			if (bitRead(autoReport, CHANNEL_UART))
			{
				String str = ParseSensorReport(sensorData, EOL, false);
				Serial.println(str);
			}
			if ((WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) && telnetEnable && bitRead(autoReport, CHANNEL_TELNET))
			{
				String str = ParseSensorReport(sensorData, EOL, false);
				for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
				{
					if (serverClient[i] && serverClient[i].connected())
					{
						sendToTelnet(str, i);
					}
					yield();
				}
			}
			if (WiFi.status() == WL_CONNECTED)
			{
#ifdef MQTT_ENABLE
				if (mqttEnable && bitRead(autoReport, CHANNEL_MQTT))
				{
					String str = ParseSensorReport(sensorData, ",", true);
					String topic = "";
					topic.reserve(100);
					mqtt_send(str, str.length(), topic);
				}
#endif

#ifdef TELEGRAM_ENABLE
				if (telegramEnable && bitRead(autoReport, CHANNEL_TELEGRAM))
				{
					String str = ParseSensorReport(sensorData, EOL, false);
					for (uint8_t i = 0; i < telegramUsersNumber; i++)
					{
						uint64_t user = getTelegramUser(i);
						if (user > 0)
						{
							sendToTelegram(user, str, TELEGRAM_RETRIES);
						}
						yield();
					}
				}
#endif

#ifdef GSCRIPT_ENABLE
				if (gScriptEnable && bitRead(autoReport, CHANNEL_GSCRIPT))
				{
					String str = ParseSensorReport(sensorData, EOL, false);
					sendValueToGoogle(str);
				}
#endif

#ifdef PUSHINGBOX_ENABLE
				if (pushingBoxEnable && bitRead(autoReport, CHANNEL_PUSHINGBOX))
				{
					String str = ParseSensorReport(sensorData, EOL, false);
					sendToPushingBox(str);
				}
#endif
			}
		}
		checkSensorLastTime = millis();
	}
	yield();
#ifdef LOG_ENABLE
	//check if it's time to collect log
	if (millis() - logLastTime > logPeriod)
	{
		sensorDataCollection sensorData = collectData();
		addToLog(sensorData);
		logLastTime = millis();

#ifdef SMTP_ENABLE
		if (history_record_number >= (uint16_t)(LOG_SIZE * 0.7))
		{
			if (smtpEnable && WiFi.status() == WL_CONNECTED && history_record_number > 0)
			{
				bool sendOk = true;
				uint16_t n = 0;
				String tmpStr = ParseSensorReport(history_log[n], CSV_DIVIDER, false);
				tmpStr += EOL;
				n++;
				while (n < history_record_number)
				{
					uint32_t freeMem = ESP.getFreeHeap() / 3;
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number) break;
						tmpStr += ParseSensorReport(history_log[n], CSV_DIVIDER, false);
						tmpStr += EOL;
						yield();
					}
					String subj = deviceName + " log";
					String addrTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);
					sendOk = sendMail(subj, tmpStr, addrTo);
					tmpStr = "";
					yield();
					if (sendOk)
					{
						//history_record_number = 0;
					}
					else
					{
						//Serial.print(F("Error sending log to E-mail "));
						//Serial.print(String(history_record_number));
						//Serial.println(F(" records."));
					}
					yield();
				}
			}
		}
#endif
	}
#endif
	yield();
	//check UART for data
	while (Serial.available() && !uartReady)
	{
		char c = Serial.read();
		if (c == '\r' || c == '\n') uartReady = true;
		else uartCommand += (char)c;
		yield();
	}

	//process data from UART
	if (uartReady)
	{
		uartReady = false;
		if (uartCommand.length() > 0)
		{
			String str = processCommand(uartCommand, CHANNEL_UART, true);
			Serial.println(str);
		}
		uartCommand = "";
	}
	yield();
	//process data from HTTP/Telnet/MQTT/TELEGRAM if available
	if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0)
	{
#ifdef HTTP_SERVER_ENABLE
		if (httpServerEnable) http_server.handleClient();
#endif

		if (telnetEnable)
		{
			//check if there are any new clients
			if (telnetServer.hasClient())
			{
				uint8_t i;
				//find free/disconnected spot
				for (i = 0; i < MAX_SRV_CLIENTS; i++)
				{
					if (!serverClient[i] || !serverClient[i].connected())
					{
						if (serverClient[i])
						{
							serverClient[i].stop();
						}
						serverClient[i] = telnetServer.available();
						break;
					}
					yield();
				}
				//no free/disconnected spot so reject
				if (i >= MAX_SRV_CLIENTS)
				{
					WiFiClient serverClient = telnetServer.available();
					serverClient.stop();
					//Serial.print(F("No more slots. Client rejected."));
				}
			}

			//check network clients for incoming data and process commands
			for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
			{
				if (serverClient[i] && serverClient[i].connected())
				{
					if (serverClient[i].available())
					{
						String telnetCommand;
						telnetCommand.reserve(255);
						//get data from the telnet client
						while (serverClient[i].available())
						{
							char c = serverClient[i].read();
							if (c == '\r' || c == '\n')
							{
								if (telnetCommand.length() > 0)
								{
									String str = processCommand(telnetCommand, CHANNEL_TELNET, true);
									telnetCommand = "";
									yield();
									sendToTelnet(str, i);
								}
							}
							else telnetCommand += (char)c;
							yield();
						}
					}
				}
			}
		}
		yield();
	}
	if (WiFi.status() == WL_CONNECTED)
	{
#ifdef MQTT_ENABLE
		if (mqttEnable)
		{
			if (!mqtt_client.connected()) mqtt_connect();
			else mqtt_client.loop();
			if (mqttCommand.length() > 0)
			{
				String str = processCommand(mqttCommand, CHANNEL_MQTT, true);
				String topic = "";
				topic.reserve(100);
				mqtt_send(str, str.length(), topic);
				mqttCommand = "";
			}
			yield();
		}
#endif

#ifdef TELEGRAM_ENABLE
		//check TELEGRAM for data
		if (telegramEnable && millis() - telegramLastTime > TELEGRAM_MESSAGE_DELAY)
		{
			sendBufferToTelegram();
			telegramLastTime = millis();
		}
#endif
	}
	yield();
#ifdef GSM_ENABLE
	if (gsmEnable && millis() > gsmTimeOut)
	{
		gsmTimeOut = millis() + SMS_CHECK_TIME_OUT;
		//getModemStatus();
		smsMessage newSms = getSMS();
		if (newSms.Message != "")
		{
			//Serial.println("New SMS from " + newSms.PhoneNumber + ":\"" + newSms.Message + "\"");
			//process data from GSM
			String str = processCommand(newSms.Message, CHANNEL_GSM, newSms.IsAdmin);
			str.trim();
			if (str.startsWith(F("^")))
			{
				bool f = sendSMS(newSms.PhoneNumber, str);
				if (!f)
				{
					//Serial.println(F("SMS not sent"));
				}
			}
		}
	}
#endif
	yield();
#ifdef DISPLAY_ENABLED
	if (displaySwitchPeriod > 0)
	{
		if (millis() - displaySwitchedLastTime > displaySwitchPeriod)
		{
			displaySwitchedLastTime = millis();
			sensorDataCollection sensorData = collectData();

#ifdef TM1637DISPLAY_ENABLE

			//show CO2
#ifdef CO2_SENSOR
			if (displayState == 0)
			{
				int co2_avg = getCo2(sensorData);
				if (co2_avg > -1000) display.showNumberDec(co2_avg, false);
				else displayState++;
			}
#endif

			//show Temperature
#ifdef TEMPERATURE_SENSOR
			if (displayState == 1)
			{
				int temp = (int)getTemperature(sensorData);
				if (temp > -1000)
				{
					display.showNumberDec(temp, false, 3, 0);
					display.setSegments(SEG_DEGREE, 1, 3);
				}
				else displayState++;
			}
#endif

			//show Humidity
#ifdef HUMIDITY_SENSOR
			if (displayState == 2)
			{
				int humidity = (int)getHumidity(sensorData);
				if (humidity > -1000)
				{
					display.showNumberDec(humidity, false, 3, 0);
					display.setSegments(SEG_HUMIDITY, 1, 3);
				}
				else displayState++;
			}
#endif

			//show clock
			if (displayState == 3)
			{
				if (timeIsSet)
				{
					display.showNumberDecEx(hour(), 0xff, true, 2, 0);
					display.showNumberDecEx(minute(), 0xff, true, 2, 2);
				}
				else displayState++;
			}
			displayState++;
			if (displayState > 3) displayState = 0;
#endif

#ifdef SSD1306DISPLAY_ENABLE
			oled.clear();
			String tmpStr = "";
			uint8_t n = 0;
			//show WiFi status
			//if (WiFi.status() == WL_CONNECTED) tmpStr += F("Wi-Fi: ok\r\n");
			//else tmpStr += F("Wi-Fi: fail\r\n");
			n++;

			//show clock
			if (timeIsSet)
			{
				uint8_t _hr = hour();
				uint8_t _min = minute();
				if (_hr < 10) tmpStr += F("0");
				tmpStr += String(_hr);
				tmpStr += F(":");
				if (_min < 10) tmpStr += F("0");
				tmpStr += String(_min);
				tmpStr += F("\r\n");
				n++;
			}

			//show Temperature
#ifdef TEMPERATURE_SENSOR
			float temp = getTemperature(sensorData);
			if (temp > -1000)
			{
				tmpStr += F("T: ");
				tmpStr += String(temp);
				tmpStr += F("\r\n");
				n++;
			}
#endif

			//show Humidity
#ifdef HUMIDITY_SENSOR
			float humidity = getHumidity(sensorData);
			if (humidity > -1000)
			{
				tmpStr += F("H: ");
				tmpStr += String(humidity);
				tmpStr += F("%\r\n");
				n++;
			}
#endif

			//show CO2
#ifdef CO2_SENSOR
			int co2_avg = getCo2(sensorData);
			if (co2_avg > -1000)
			{
				tmpStr += F("CO2: ");
				tmpStr += String(co2_avg);
				tmpStr += F("\r\n");
				n++;
		}
#endif

			//is strings are not many scale them
			if (n < 3) oled.set2X();
			else oled.set1X();
			oled.print(tmpStr);
#endif
	}
}
#endif
	yield();
#ifdef EVENTS_ENABLE
	if (eventsEnable)
	{
		for (uint8_t i = 0; i < eventsNumber; i++)
		{
			processEvent(i);
			yield();
		}
	}
#endif

#ifdef SCHEDULER_ENABLE
	if (schedulerEnable && timeIsSet)
	{
		for (uint8_t i = 0; i < schedulesNumber; i++)
		{
			processSchedule(i);
			yield();
		}
	}
#endif

#ifdef SLEEP_ENABLE
	if (sleepEnable && millis() > uint32_t(activeStart + activeTimeOut_ms))
	{
		ESP.deepSleep(sleepTimeOut_us);
	}
#endif
}

uint32_t CollectEepromSize()
{
	uint32_t eeprom_size = DEVICE_NAME_size;
	eeprom_size += STA_SSID_size;
	eeprom_size += STA_PASS_size;
	eeprom_size += AP_SSID_size;
	eeprom_size += AP_PASS_size;
	eeprom_size += WIFI_STANDART_size;
	eeprom_size += WIFI_MODE_size;
	eeprom_size += CONNECT_TIME_size;
	eeprom_size += CONNECT_PERIOD_size;

	eeprom_size += SENSOR_READ_DELAY_size;
	eeprom_size += LOG_PERIOD_size;
	eeprom_size += DISPLAY_REFRESH_size;
	eeprom_size += AUTOREPORT_size;

	eeprom_size += PIN_MODE_size;
	eeprom_size += OUTPUT_INIT_size;
	eeprom_size += INTERRUPT_MODE_size;

	eeprom_size += SLEEP_ON_size;
	eeprom_size += SLEEP_OFF_size;
	eeprom_size += SLEEP_ENABLE_size;

	eeprom_size += EVENTS_TABLE_size;
	eeprom_size += EVENTS_ENABLE_size;

	eeprom_size += SCHEDULER_TABLE_size;
	eeprom_size += SCHEDULER_LASTRUN_TABLE_size;
	eeprom_size += SCHEDULER_ENABLE_size;

	eeprom_size += NTP_SERVER_size;
	eeprom_size += NTP_TIME_ZONE_size;
	eeprom_size += NTP_REFRESH_DELAY_size;
	eeprom_size += NTP_ENABLE_size;

	eeprom_size += TELNET_PORT_size;
	eeprom_size += TELNET_ENABLE_size;

	eeprom_size += HTTP_PORT_size;
	eeprom_size += HTTP_ENABLE_size;

	eeprom_size += MQTT_SERVER_size;
	eeprom_size += MQTT_PORT_size;
	eeprom_size += MQTT_USER_size;
	eeprom_size += MQTT_PASS_size;
	eeprom_size += MQTT_ID_size;
	eeprom_size += MQTT_TOPIC_IN_size;
	eeprom_size += MQTT_TOPIC_OUT_size;
	eeprom_size += MQTT_CLEAN_size;
	eeprom_size += MQTT_ENABLE_size;

	eeprom_size += TELEGRAM_TOKEN_size;
	eeprom_size += TELEGRAM_USERS_TABLE_size;
	eeprom_size += TELEGRAM_ENABLE_size;

	eeprom_size += PUSHINGBOX_ID_size;
	eeprom_size += PUSHINGBOX_PARAM_size;
	eeprom_size += PUSHINGBOX_ENABLE_size;

	eeprom_size += SMTP_SERVER_ADDRESS_size;
	eeprom_size += SMTP_SERVER_PORT_size;
	eeprom_size += SMTP_LOGIN_size;
	eeprom_size += SMTP_PASSWORD_size;
	eeprom_size += SMTP_TO_size;
	eeprom_size += SMTP_ENABLE_size;

	eeprom_size += GSCRIPT_ID_size;
	eeprom_size += GSCRIPT_ENABLE_size;

	eeprom_size += GSM_USERS_TABLE_size;
	eeprom_size += GSM_ENABLE_size;

	return eeprom_size;
}

String readConfigString(uint16_t startAt, uint16_t maxlen)
{
	String str = "";
	str.reserve(maxlen);
	for (uint16_t i = 0; i < maxlen; i++)
	{
		char c = (char)EEPROM.read(startAt + i);
		if (c == 0) i = maxlen;
		else str += c;
		yield();
	}
	return str;
}

void readConfigString(uint16_t startAt, uint16_t maxlen, char* array)
{
	for (uint16_t i = 0; i < maxlen; i++)
	{
		array[i] = (char)EEPROM.read(startAt + i);
		if (array[i] == 0) i = maxlen;
		yield();
	}
}

uint32_t readConfigLong(uint16_t startAt)
{
	union Convert
	{
		uint32_t number;
		uint8_t byteNum[4];
	} p;
	for (uint8_t i = 0; i < 4; i++)
	{
		p.byteNum[i] = EEPROM.read(startAt + i);
		yield();
	}
	return p.number;
}

float readConfigFloat(uint16_t startAt)
{
	union Convert
	{
		float number;
		uint8_t byteNum[4];
	} p;
	for (uint8_t i = 0; i < 4; i++)
	{
		p.byteNum[i] = EEPROM.read(startAt + i);
		yield();
	}
	return p.number;
}

void writeConfigString(uint16_t startAt, uint16_t maxLen, String str)
{
	for (uint16_t i = 0; i < maxLen; i++)
	{
		if (i < str.length())EEPROM.write(startAt + i, (uint8_t)str[i]);
		else EEPROM.write(startAt + i, 0);
		yield();
	}
	EEPROM.commit();
}

void writeConfigString(uint16_t startAt, uint16_t maxlen, char* array, uint8_t length)
{
	for (uint16_t i = 0; i < maxlen; i++)
	{
		if (i >= length)
		{
			EEPROM.write(startAt + i, 0);
			break;
		}
		EEPROM.write(startAt + i, (uint8_t)array[i]);
		yield();
	}
	EEPROM.commit();
}

void writeConfigLong(uint16_t startAt, uint32_t data)
{
	union Convert
	{
		uint32_t number;
		uint8_t byteNum[4];
	} p;
	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	EEPROM.commit();
}

void writeConfigFloat(uint16_t startAt, float data)
{
	union Convert
	{
		float number;
		uint8_t byteNum[4];
	} p;

	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	EEPROM.commit();
}

void sendToTelnet(String& str, uint8_t clientN)
{
	if (serverClient[clientN] && serverClient[clientN].connected())
	{
		serverClient[clientN].print(str);
		delay(1);
	}
}

bool add_signal_pin(uint8_t pin)
{
	for (uint8_t num = 0; num < PIN_NUMBER; num++)
	{
		if (pins[num] == pin)
		{
			if (bitRead(SIGNAL_PINS, num))
			{
				return false;
			}
			else
			{
				bitSet(SIGNAL_PINS, num);
				return true;
			}
		}
	}
	return false;
}

String set_output(String& outputSetter)
{
	int t = outputSetter.indexOf('=');
	String str;
	str.reserve(30);
	str = "";
	if (t >= 11 && t < outputSetter.length() - 1)
	{
		uint8_t outNum = outputSetter.substring(10, t).toInt();
		String outStateStr = outputSetter.substring(t + 1);
		uint16_t outState;
		bool pwm_mode = false;
		if (outStateStr == SWITCH_ON) outState = uint16_t(OUT_ON);
		else if (outStateStr == SWITCH_OFF) outState = uint16_t(OUT_OFF);
		else
		{
			pwm_mode = true;
			outState = outStateStr.toInt();
		}
		if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
		{
			outputs[outNum - 1] = outState;
			if (pwm_mode) analogWrite(pins[outNum - 1], outputs[outNum - 1]);
			else digitalWrite(pins[outNum - 1], outputs[outNum - 1]);
			str = F("Output");
			str += String(outNum);
			str += F("=");
			str += String((outState));
		}
		else
		{
			str = F("Incorrect output #");
			str += String(outNum);
			str += EOL;
		}
	}
	return str;
}

String printConfig(bool toJson = false)
{
	String eq = F(": ");
	String delimiter = F("\r\n");
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
	}
	String str;
	//str.reserve(3000);
	if (toJson) str = F("{\"");
	yield();
	str += F("Device name");
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;

	str += F("WiFi STA SSID");
	str += eq;
	str += readConfigString(STA_SSID_addr, STA_SSID_size);
	str += delimiter;

	str += F("WiFi STA password");
	str += eq;
	str += readConfigString(STA_PASS_addr, STA_PASS_size);
	str += delimiter;

	str += F("WiFi AP SSID");
	str += eq;
	str += readConfigString(AP_SSID_addr, AP_SSID_size);
	str += delimiter;

	str += F("WiFi AP password");
	str += eq;
	str += readConfigString(AP_PASS_addr, AP_PASS_size);
	str += delimiter;

	str += F("WiFi connect time limit");
	str += eq;
	str += readConfigString(CONNECT_TIME_addr, CONNECT_TIME_size);
	str += delimiter;

	str += F("WiFi disconnect time");
	str += eq;
	str += readConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size);
	str += F(" sec.");
	str += delimiter;

	str += F("WiFi power");
	str += eq;
	str += String(readConfigFloat(WIFI_POWER_addr));
	str += F(" dB");
	str += delimiter;

	str += F("WiFi standart");
	str += eq;
	str += F("802.11");
	str += readConfigString(WIFI_STANDART_addr, WIFI_STANDART_size);
	str += delimiter;

	str += F("WiFi mode");
	str += eq;
	long m = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (m < 0 || m >= wifiModes->length()) m = WIFI_MODE_AUTO;
	str += wifiModes[m];
	str += delimiter;

	str += F("Sensor read period");
	str += eq;
	str += String(readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt());
	str += delimiter;

	str += F("Autoreport");
	str += eq;
	m = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();
	for (uint8_t b = 0; b < PIN_NUMBER; b++)
	{
		if (bitRead(m, b))
		{
			str += channels[b];
			str += F(",");
		}
		yield();
	}
	str += delimiter;

	for (m = 0; m < PIN_NUMBER; m++)
	{
		str += F("Pin");
		str += String(m + 1);
		str += F("(port ");
		str += String(pins[m]);
		str += F(")");
		str += eq;

		uint16_t l = PIN_MODE_size / PIN_NUMBER;
		uint8_t pinModeCfg = readConfigString(PIN_MODE_addr + m * l, l).toInt();

		if (bitRead(SIGNAL_PINS, m)) //pin reserved to data bus
		{
			switch (pins[m])
			{
#ifdef I2C_ENABLE
			case PIN_WIRE_SCL:
				str += F("I2C SCL");
				break;
			case PIN_WIRE_SDA:
				str += F("I2C SDA");
				break;
#endif
#ifdef UART2_ENABLE
			case UART2_TX:
				str += F("SoftUART TX");
				break;
			case UART2_RX:
				str += F("SoftUART RX");
				break;
#endif
#ifdef ONEWIRE_ENABLE
			case ONEWIRE_DATA:
				str += F("OneWire");
				break;
#endif
#ifdef DHT_ENABLE
			case DHT_ENABLE:
				str += F("DHT");
				break;
#endif
#ifdef MH_Z19_PPM_ENABLE
			case MH_Z19_PPM_ENABLE:
				str += F("MH-Z19 PPM");
				break;
#endif
#ifdef TM1637DISPLAY_ENABLE
			case TM1637_CLK:
				str += F("TM1637 CLK");
				break;
			case TM1637_DIO:
				str += F("TM1637 DIO");
				break;
#endif
			}
		}
		else if (pinModeCfg == INPUT || pinModeCfg == INPUT_PULLUP) // pin is input or input_pullup
		{
			if (pinModeCfg == INPUT) str += F("INPUT");
			else str += F("INPUT_PULLUP");
			str += F(", interrupt mode ");
			uint16_t l = INTERRUPT_MODE_size / PIN_NUMBER;
			uint8_t n = readConfigString(INTERRUPT_MODE_addr + m * l, l).toInt();
			if (n < 0 || n >= intModeList->length()) n = 0;
			str += intModeList[n];
		}
		else if (pinModeCfg == OUTPUT) // pin is output
		{
			str += F("OUT");
			str += F(", initial state ");
			uint16_t l = OUTPUT_INIT_size / PIN_NUMBER;
			str += readConfigString(OUTPUT_INIT_addr + m * l, l);
		}
		else if (pinModeCfg == 3) // pin is off
		{
			str += F("OFF");
		}
		else
		{
			str += F("UNDEFINED");
		}
		str += delimiter;

		yield();
	}

#ifdef LOG_ENABLE
	//Log settings
	str += F("Device log size");
	str += eq;
	str += String(LOG_SIZE);
	str += delimiter;

	str += F("Log save period");
	str += eq;
	str += String(readConfigString(LOG_PERIOD_addr, LOG_PERIOD_size).toInt());
	str += delimiter;
#endif

#ifdef SLEEP_ENABLE
	str += F("Sleep ON/OFF time");
	str += eq;
	str += readConfigString(SLEEP_ON_addr, SLEEP_ON_size);
	str += F("/");
	str += readConfigString(SLEEP_OFF_addr, SLEEP_OFF_size);
	str += delimiter;

	str += F("SLEEP mode enabled");
	str += eq;
	str += readConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size);
	str += delimiter;
#endif

	str += F("Telnet port");
	str += eq;
	str += readConfigString(TELNET_PORT_addr, TELNET_PORT_size);
	str += delimiter;

	str += F("Telnet clients limit");
	str += eq;
	str += String(MAX_SRV_CLIENTS);
	str += delimiter;

	str += F("Telnet service enabled");
	str += eq;
	str += readConfigString(TELNET_ENABLE_addr, TELNET_ENABLE_size);
	str += delimiter;

#ifdef HTTP_SERVER_ENABLE
	str += F("HTTP port");
	str += eq;
	str += readConfigString(HTTP_PORT_addr, HTTP_PORT_size);
	str += delimiter;

	str += F("HTTP service enabled");
	str += eq;
	str += readConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size);
	str += delimiter;
#endif

#ifdef MQTT_ENABLE
	str += F("MQTT server");
	str += eq;
	str += readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
	str += delimiter;

	str += F("MQTT port");
	str += eq;
	str += readConfigString(MQTT_PORT_addr, MQTT_PORT_size);
	str += delimiter;

	str += F("MQTT login");
	str += eq;
	str += readConfigString(MQTT_USER_addr, MQTT_USER_size);
	str += delimiter;

	str += F("MQTT password");
	str += eq;
	str += readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
	str += delimiter;

	str += F("MQTT ID");
	str += eq;
	str += readConfigString(MQTT_ID_addr, MQTT_ID_size);
	str += delimiter;

	str += F("MQTT receive topic");
	str += eq;
	str += readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
	str += delimiter;

	str += F("MQTT post topic");
	str += eq;
	str += readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
	str += delimiter;

	str += F("MQTT clean session");
	str += eq;
	str += readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size);
	str += delimiter;

	str += F("MQTT service enabled");
	str += eq;
	str += readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size);
	str += delimiter;
#endif

#ifdef SMTP_ENABLE
	str += F("SMTP server");
	str += eq;
	str += readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	str += delimiter;

	str += F("SMTP port");
	str += eq;
	str += readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size);
	str += delimiter;

	str += F("SMTP login");
	str += eq;
	str += readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	str += delimiter;

	str += F("SMTP password");
	str += eq;
	str += readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	str += delimiter;

	str += F("Mail To");
	str += eq;
	str += readConfigString(SMTP_TO_addr, SMTP_TO_size);
	str += delimiter;

	str += F("SMTP service enabled");
	str += eq;
	str += readConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size);
	str += delimiter;
#endif

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM token: ");
	str += readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	str += delimiter;

	str += F("Admin #0");
	str += eq;
	str += uint64ToString(getTelegramUser(0));
	str += delimiter;

	for (m = 1; m < telegramUsersNumber; m++)
	{
		str += F("User #");
		str += String(m);
		str += eq;
		str += uint64ToString(getTelegramUser(m));
		str += delimiter;
		yield();
	}

	str += F("TELEGRAM service enabled");
	str += eq;
	str += readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size);
	str += delimiter;
#endif

#ifdef GSCRIPT_ENABLE
	str += F("GSCRIPT token");
	str += eq;
	str += readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	str += delimiter;

	str += F("GSCRIPT service enabled");
	str += eq;
	str += readConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size);
	str += delimiter;
#endif

#ifdef PUSHINGBOX_ENABLE
	str += F("PUSHINGBOX token");
	str += eq;
	str += readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
	str += delimiter;

	str += F("PUSHINGBOX parameter name");
	str += eq;
	str += readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	str += delimiter;

	str += F("PUSHINGBOX service enabled");
	str += eq;
	str += readConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size);
	str += delimiter;
#endif

#ifdef NTP_TIME_ENABLE
	str += F("NTP server");
	str += eq;
	str += readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	str += delimiter;

	str += F("NTP time zone");
	str += eq;
	str += readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size);
	str += delimiter;

	str += F("NTP refresh delay");
	str += eq;
	str += readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size);
	str += delimiter;

	str += F("NTP service enabled");
	str += eq;
	str += readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size);
	str += delimiter;
#endif

#ifdef EVENTS_ENABLE
	for (m = 0; m < eventsNumber; m++)
	{
		str += F("EVENT #");
		str += String(m);
		str += eq;
		str += getEvent(m);
		str += delimiter;
		yield();
	}
	str += F("EVENTS service enabled");
	str += eq;
	str += readConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size);
	str += delimiter;
#endif

#ifdef SCHEDULER_ENABLE
	for (m = 0; m < schedulesNumber; m++)
	{
		str += F("SCHEDULE #");
		str += String(m);
		str += eq;
		str += getSchedule(m);

		uint32_t lastExec = readScheduleExecTime(m);
		if (lastExec > 0)
		{
			str += F(" [executed ");
			str += timeToString(lastExec);
			str += F("]");
		}

		str += delimiter;
		yield();
	}
	str += F("SCHEDULER service enabled");
	str += eq;
	str += readConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size);
	str += delimiter;
#endif

#ifdef UART2_ENABLE
	str += F("SoftUART speed");
	str += eq;
	str += String(uart2_Speed);
	str += delimiter;
#endif

#ifdef GSM_M590_ENABLE
	str += F("M590 GSM modem");
	str += eq;
	str += F("SoftUART");
	str += delimiter;
#endif

#ifdef GSM_SIM800_ENABLE
	str += F("SIM800 GSM modem");
	str += eq;
	str += F("SoftUART");
	str += delimiter;
#endif

#ifdef GSM_ENABLE
	for (m = 0; m < gsmUsersNumber; m++)
	{
		str += F("Phone #");
		str += String(m);
		str += eq;
		str += getGsmUser(m);
		str += delimiter;
		yield();
	}
	str += F("GSM service enabled");
	str += eq;
	str += readConfigString(GSM_ENABLE_addr, GSM_ENABLE_size);
	str += delimiter;
#endif

#ifdef TM1637DISPLAY_ENABLE
	str += F("TM1637 CLK/DIO pin");
	str += eq;
	str += TM1637_CLK;
	str += F("/");
	str += TM1637_DIO;
	str += delimiter;
#endif

#ifdef SSD1306DISPLAY_ENABLE
	str += F("SSD1306 display");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif

#ifdef DISPLAY_ENABLED
	str += F("Display refresh delay");
	str += eq;
	str += readConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size);
	str += delimiter;
#endif

#ifdef AMS2320_ENABLE
	str += F("AMS2320");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif

#ifdef HTU21D_ENABLE
	str += F("HTU21D");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif

#ifdef BME280_ENABLE
	str += F("BME280");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif

#ifdef BMP180_ENABLE
	str += F("BMP180");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif

#ifdef DS18B20_ENABLE
	str += F("DS18B20");
	str += eq;
	str += F("OneWire");
	str += delimiter;
#endif

#ifdef DHT_ENABLE
	str += "DHT" + DHT_ENABLE;
	str += F(" pin");
	str += eq;
	str += DHT_PIN;
	str += delimiter;
#endif

#ifdef MH_Z19_UART_ENABLE
	str += F("MH-Z19 CO2");
	str += eq;
	str += F("SoftUART");
	str += delimiter;
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += F("MH-Z19 CO2 PPM pin");
	str += eq;
	str += MH_Z19_PPM_ENABLE;
	str += delimiter;
#endif

#ifdef ADC_ENABLE
	str += F("ADC pin");
	str += eq;
	str += F("A0");
	str += delimiter;
#endif

	if (toJson) str += F("\"}");

	return str;
}

String printStatus(bool toJson = false)
{
	String delimiter = F("\r\n");
	String eq = F(": ");
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
	}

	String str;
	str.reserve(1024);
	if (toJson) str = F("{\"");

	str += F("Device name");
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;
	str += F("Time");
	str += eq;
	if (timeIsSet) str += timeToString(now());
	else str += F("not set");
	str += delimiter;
	str += F("WiFi enabled");
	str += eq;
	str += String(wifiEnable);
	str += delimiter;
	str += F("Telnet enabled");
	str += eq;
	str += String(telnetEnable);
	str += delimiter;

#ifdef NTP_TIME_ENABLE
	str += F("NTP enabled");
	str += eq;
	str += String(NTPenable);
	str += delimiter;
#endif

	str += F("WiFi mode");
	str += eq;
	str += wifiModes[currentWiFiMode];
	str += delimiter;

	str += F("WiFi connection");
	str += eq;
	if (WiFi.status() == WL_CONNECTED)
	{
		str += F("connected");
		str += delimiter;
		str += F("AP");
		str += eq;
		str += getStaSsid();
		str += delimiter;
		str += F("IP");
		str += eq;
		str += WiFi.localIP().toString();
		str += delimiter;
	}
	else
	{
		str += F("not connected");
		str += delimiter;
	}

	if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0)
	{
		uint8_t netClientsNum = 0;
		for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
		{
			if (serverClient[i] && serverClient[i].connected()) netClientsNum++;
			yield();
		}
		str += F("Telnet clients connected");
		str += eq;
		str += String(netClientsNum);
		str += F("/");
		str += MAX_SRV_CLIENTS;
		str += delimiter;
	}

	if (WiFi.softAPgetStationNum() > 0)
	{
		str += F("AP Clients connected");
		str += eq;
		str += String(WiFi.softAPgetStationNum());
		str += delimiter;
	}

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM buffer usage");
	str += eq;
	str += String(telegramOutboundBufferPos);
	str += F("/");
	str += String(telegramMessageBufferSize);
	str += delimiter;
#endif

#ifdef EVENTS_ENABLE
	str += F("Events enabled");
	str += eq;
	str += String(eventsEnable);
	str += delimiter;
	for (uint8_t i = 0; i < eventsNumber; i++)
	{
		String eventStr = getEvent(i);
		if (eventStr.length() > 0)
		{
			str += F("Event #");
			str += String(i);
			str += F(" active");
			str += eq;
			str += String(eventsFlags[i]);
			str += delimiter;
		}
		yield();
	}
#endif

#ifdef SCHEDULER_ENABLE
	str += F("Scheduler enabled");
	str += eq;
	str += String(schedulerEnable);
	str += delimiter;
	for (uint8_t i = 0; i < schedulesNumber; i++)
	{
		String scheduleStr = getSchedule(i);
		if (scheduleStr.length() > 0)
		{
			str += F("Schedule #");
			str += String(i);
			str += F(" last run time");
			str += eq;
			uint32_t runTime = readScheduleExecTime(i);
			str += String(year(runTime)) + "." + String(month(runTime)) + "." + String(day(runTime)) + " " + String(hour(runTime)) + ":" + String(minute(runTime)) + ":" + String(second(runTime));
			str += delimiter;
		}
		yield();
	}

#endif

#ifdef LOG_ENABLE
	str += F("Log records collected");
	str += eq;
	str += String(history_record_number);
	str += delimiter;
#endif

#ifdef GSM_ENABLE
	str += F("GSM enabled");
	str += eq;
	str += String(gsmEnable);
	str += delimiter;
#endif

#ifdef SLEEP_ENABLE
	str += F("SLEEP enabled");
	str += eq;
	str += String(sleepEnable);
	str += delimiter;
#endif

	str += F("Free memory");
	str += eq;
	str += String(ESP.getFreeHeap());
	str += delimiter;
	str += F("Heap fragmentation");
	str += eq;
	str += String(ESP.getHeapFragmentation());
	str += delimiter;

	if (toJson) str += F("\"}");

	return str;
}

String printHelp()
{
	return F("Commands:\r\n"
		"help\r\n"
		"help_event\r\n"
		"help_schedule\r\n"
		"help_action\r\n"
		"get_sensor\r\n"
		"get_status\r\n"
		"get_config\r\n"

		"[ADMIN]        set_time=yyyy.mm.dd hh:mm:ss\r\n"

		"[ADMIN][FLASH] set_pin_mode?=OFF/INPUT/OUTPUT/INPUT_PULLUP\r\n"
		"[ADMIN][FLASH] set_init_output?=[on/off, 0..1023]\r\n"
		"[ADMIN][FLASH] set_interrupt_mode?=OFF/FALLING/RISING/CHANGE\r\n"
		"[ADMIN]        set_output?=[on/off, 0..1023]\r\n"

		"[ADMIN][FLASH] autoreport=n (bit[0..7]=UART,TELNET,MQTT,TELEGRAM,GSCRIPT,PUSHINGBOX,SMTP,GSM)\r\n"

#ifdef SSD1306DISPLAY_ENABLE
		"[ADMIN][FLASH] display_refresh=n (sec.)\r\n"
#endif

		"[ADMIN][FLASH] check_period=n (sec.)\r\n"

		"[ADMIN][FLASH] device_name=****\r\n"

		"[ADMIN][FLASH] sta_ssid=****\r\n"
		"[ADMIN][FLASH] sta_pass=****\r\n"
		"[ADMIN][FLASH] ap_ssid=**** (empty for device_name+mac)\r\n"
		"[ADMIN][FLASH] ap_pass=****\r\n"
		"[ADMIN][FLASH] wifi_standart=B/G/N\r\n"
		"[ADMIN][FLASH] wifi_power=n (0..20.5)\r\n"
		"[ADMIN][FLASH] wifi_mode=AUTO/STATION/APSTATION/AP/OFF\r\n"
		"[ADMIN][FLASH] wifi_connect_time=n (sec.)\r\n"
		"[ADMIN][FLASH] wifi_reconnect_period=n (sec.)\r\n"
		"[ADMIN]        wifi_enable=on/off\r\n"

#ifdef LOG_ENABLE		
		"[ADMIN][FLASH] log_period=n (sec.)\r\n"
#endif

#ifdef SLEEP_ENABLE
		"[ADMIN][FLASH] sleep_on=n (sec.)\r\n"
		"[ADMIN][FLASH] sleep_off=n (sec.)\r\n"
		"[ADMIN][FLASH] sleep_enable=on/off\r\n"
#endif

		"[ADMIN][FLASH] telnet_port=n\r\n"
		"[ADMIN][FLASH] telnet_enable=on/off\r\n"

#ifdef HTTP_SERVER_ENABLE
		"[ADMIN][FLASH] http_port=n\r\n"
		"[ADMIN][FLASH] http_enable=on/off\r\n"
#endif

#ifdef NTP_TIME_ENABLE
		"[ADMIN][FLASH] ntp_server=****\r\n"
		"[ADMIN][FLASH] ntp_time_zone=n\r\n"
		"[ADMIN][FLASH] ntp_refresh_delay=n (sec.)\r\n"
		"[ADMIN][FLASH] ntp_enable=on/off\r\n"
#endif

#ifdef MQTT_ENABLE
		"[ADMIN][FLASH] mqtt_server=****\r\n"
		"[ADMIN][FLASH] mqtt_port=n\r\n"
		"[ADMIN][FLASH] mqtt_login=****\r\n"
		"[ADMIN][FLASH] mqtt_pass=****\r\n"
		"[ADMIN][FLASH] mqtt_id=**** (empty for device_name+mac)\r\n"
		"[ADMIN][FLASH] mqtt_topic_in=****\r\n"
		"[ADMIN][FLASH] mqtt_topic_out=****\r\n"
		"[ADMIN][FLASH] mqtt_clean=on/off\r\n"
		"[ADMIN][FLASH] mqtt_enable=on/off\r\n"
#endif

#ifdef GSM_ENABLE
		"[ADMIN][FLASH] gsm_user?=n\r\n"
		"[ADMIN][FLASH] gsm_enable=on/off\r\n"
#endif

#ifdef TELEGRAM_ENABLE
		"[ADMIN][FLASH] telegram_token=****\r\n"
		"[ADMIN][FLASH] telegram_user?=n\r\n"
		"[ADMIN][FLASH] telegram_enable=on/off\r\n"
#endif

#ifdef SMTP_ENABLE
		"[ADMIN][FLASH] smtp_server=****\r\n"
		"[ADMIN][FLASH] smtp_port=n\r\n"
		"[ADMIN][FLASH] smtp_login=****\r\n"
		"[ADMIN][FLASH] smtp_pass=****\r\n"
		"[ADMIN][FLASH] smtp_to=****@***.***\r\n"
		"[ADMIN][FLASH] smtp_enable=on/off\r\n"
#endif

#ifdef GSCRIPT_ENABLE
		"[ADMIN][FLASH] gscript_token=****\r\n"
		"[ADMIN][FLASH] gscript_enable=on/off\r\n"
#endif

#ifdef PUSHINGBOX_ENABLE
		"[ADMIN][FLASH] pushingbox_token=****\r\n"
		"[ADMIN][FLASH] pushingbox_parameter=****\r\n"
		"[ADMIN][FLASH] pushingbox_enable=on/off\r\n"
#endif

#ifdef EVENTS_ENABLE
		"[ADMIN][FLASH] set_event?=condition:action1;action2;...\r\n"
		"[ADMIN][FLASH] events_enable=on/off\r\n"
		"[ADMIN]        set_event_flag?=on/off\r\n"
#endif

#ifdef SCHEDULER_ENABLE
		"[ADMIN][FLASH] set_schedule?=period@time:action1;action2;...\r\n"
		"[ADMIN][FLASH] scheduler_enable=on/off\r\n"
		"[ADMIN]        clear_schedule_exec_time?\r\n"
#endif

		"[ADMIN]        reset");
}

String timeToString(uint32_t time)
{
	// digital clock display of the time
	String tmp = String(hour(time));
	tmp += F(":");
	tmp += String(minute(time));
	tmp += F(":");
	tmp += String(second(time));
	tmp += F(" ");
	tmp += String(year(time));
	tmp += F(".");
	tmp += String(month(time));
	tmp += F(".");
	tmp += String(day(time));
	return tmp;
}

sensorDataCollection collectData()
{
	sensorDataCollection sensorData;
	sensorData.year = year();
	sensorData.month = month();
	sensorData.day = day();
	sensorData.hour = hour();
	sensorData.minute = minute();
	sensorData.second = second();

#ifdef AMS2320_ENABLE
	if (am2320_enable)
	{
		sensorData.ams_humidity = am2320.readHumidity();
		sensorData.ams_temp = am2320.readTemperature();
	}
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable)
	{
		sensorData.htu21d_humidity = htu21d.readHumidity();
		sensorData.htu21d_temp = htu21d.readTemperature();
	}
#endif

#ifdef BME280_ENABLE
	if (bme280_enable)
	{
		sensorData.bme280_humidity = bme280.readHumidity();
		sensorData.bme280_temp = bme280.readTemperature();
		sensorData.bme280_pressure = bme280.readPressure();
	}
#endif

#ifdef BMP180_ENABLE
	if (bmp180_enable)
	{
		sensorData.bmp180_temp = bmp180.readTemperature();
		sensorData.bmp180_pressure = bmp180.readPressure();
	}
#endif

#ifdef DS18B20_ENABLE
	if (ds1820_enable)
	{
		ds1820.requestTemperatures();
		sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
	}
#endif

#ifdef DHT_ENABLE
	sensorData.dht_humidity = dht.readHumidity();
	sensorData.dht_temp = dht.readTemperature();
#endif

#ifdef MH_Z19_UART_ENABLE
	sensorData.mh_uart_co2 = co2SerialRead();
	sensorData.mh_temp = mhtemp_s;
#endif

#ifdef MH_Z19_PPM_ENABLE
	sensorData.mh_ppm_co2 = co2PPMRead();
#endif

#ifdef ADC_ENABLE
	sensorData.adc = getAdc();
#endif

	if (INTERRUPT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(INPUT_PINS, num))
			{
				sensorData.InterruptCounters[num] = InterruptCounter[num];
			}
			yield();
		}
	}

	if (INPUT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(INPUT_PINS, num))
			{
				sensorData.inputs[num] = inputs[num] = digitalRead(pins[num]);
			}
			yield();
		}
	}

	if (OUTPUT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(INPUT_PINS, num))
			{
				sensorData.outputs[num] = digitalRead(pins[num]);
			}
			yield();
		}
	}

	return sensorData;
}

String ParseSensorReport(sensorDataCollection& data, String delimiter, bool toJson = false)
{
	String eq = F("=");
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
	}

	String str;
	str.reserve(512);
	if (toJson) str = F("{\"");
	str += F("DeviceName");
	str += eq;
	str += deviceName;
	str += delimiter;
	str += F("Time");
	str += eq;
	str += String(data.hour);
	str += F(":");
	str += String(data.minute);
	str += F(":");
	str += String(data.second);
	str += F(" ");
	str += String(data.year);
	str += F("/");
	str += String(data.month);
	str += F("/");
	str += String(data.day);

#ifdef MH_Z19_UART_ENABLE
	str += delimiter;
	str += F("UART CO2");
	str += eq;
	str += String(data.mh_uart_co2);
	str += delimiter;
	str += F("MH-Z19 t");
	str += eq;
	str += String(data.mh_temp);
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += delimiter;
	str += F("MH-Z19 CO2");
	str += eq;
	str += String(data.mh_ppm_co2);
#endif

#ifdef AMS2320_ENABLE
	if (am2320_enable)
	{
		if (!isnan(data.ams_temp))
		{
			str += delimiter;
			str += F("AMS2320 t");
			str += eq;
			str += String(data.ams_temp);
		}
		if (!isnan(data.ams_temp))
		{
			str += delimiter;
			str += F("AMS2320 h");
			str += eq;
			str += String(data.ams_humidity);
		}
	}
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable)
	{
		str += delimiter;
		str += F("HTU21D t");
		str += eq;
		str += String(data.htu21d_temp);
		str += delimiter;
		str += F("HTU21D h");
		str += eq;
		str += String(data.htu21d_humidity);
	}
#endif

#ifdef BME280_ENABLE
	if (bme280_enable)
	{
		str += delimiter;
		str += F("BME280 t");
		str += eq;
		str += String(data.bme280_temp);
		str += delimiter;
		str += F("BME280 h");
		str += eq;
		str += String(data.bme280_humidity);
		str += delimiter;
		str += F("BME280 p");
		str += eq;
		str += String(data.bme280_pressure);
	}
#endif

#ifdef BMP180_ENABLE
	if (bmp180_enable)
	{
		str += delimiter;
		str += F("BMP180 t");
		str += eq;
		str += String(data.bmp180_temp);
		str += delimiter;
		str += F("BMP180 p");
		str += eq;
		str += String(data.bmp180_pressure);
	}
#endif

#ifdef DS18B20_ENABLE
	if (ds1820_enable && data.ds1820_temp > -127 && data.ds1820_temp < 127)
	{
		str += delimiter;
		str += F("DS18B20 t");
		str += eq;
		str += String(data.ds1820_temp);
	}
#endif

#ifdef DHT_ENABLE
	str += delimiter;
	str += DHT_ENABLE;
	str += F("DHT t");
	str += eq;
	str += String(data.dht_temp);
	str += delimiter;
	str += DHT_ENABLE;
	str += F("DHT h");
	str += eq;
	str += String(data.dht_humidity);
#endif

#ifdef ADC_ENABLE
	str += delimiter;
	str += F("ADC");
	str += eq;
	str += String(data.adc);
#endif

	if (INTERRUPT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(INTERRUPT_PINS, num))
			{
				str += delimiter;
				str += F("COUNTER");
				str += String(num + 1);
				str += eq;
				str += String(data.InterruptCounters[num]);
			}
			yield();
		}
	}

	if (INPUT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(INPUT_PINS, num))
			{
				str += delimiter;
				str += F("IN");
				str += String(num + 1);
				str += eq;
				str += String(data.inputs[num]);
			}
			yield();
		}
	}

	if (OUTPUT_PINS > 0)
	{
		for (uint8_t num = 0; num < PIN_NUMBER; num++)
		{
			if (bitRead(OUTPUT_PINS, num))
			{
				str += delimiter;
				str += F("OUT");
				str += String(num + 1);
				str += eq;
				str += String(data.outputs[num]);
			}
			yield();
		}
	}
	if (toJson) str += F("\"}");
	return str;
}

String processCommand(String command, uint8_t channel, bool isAdmin)
{
	String tmp = "";
	if (command.indexOf('=') > 0) tmp = command.substring(0, command.indexOf('=') + 1);
	else tmp = command;
	tmp.toLowerCase();
	String str = "";
	str.reserve(2048);
	if (channel == CHANNEL_GSM) str += F("^");
	if (tmp == F("get_sensor"))
	{
		sensorDataCollection sensorData = collectData();
		str += ParseSensorReport(sensorData, EOL);
	}
	else if (tmp == F("get_status"))
	{
		str += printStatus();
	}
	else if (tmp == F("get_config"))
	{
		str += printConfig();
	}
	else if (tmp == F("help"))
	{
		str += printHelp();
	}
#ifdef EVENTS_ENABLE
	else if (tmp == "help_event")
	{
		str += printHelpEvent();
	}
#endif
#ifdef SCHEDULER_ENABLE
	else if (tmp == "help_schedule")
	{
		str += printHelpSchedule();
	}
#endif
#if defined(EVENTS_ENABLE) || defined(SCHEDULER_ENABLE)
	else if (tmp == "help_action")
	{
		str += printHelpAction();
	}
#endif

	else if (isAdmin)
	{
		if (tmp.startsWith(F("set_time=")) && command.length() == 28)
		{
			uint8_t _hr = command.substring(20, 22).toInt();
			uint8_t _min = command.substring(23, 25).toInt();
			uint8_t _sec = command.substring(26, 28).toInt();
			uint8_t _day = command.substring(17, 19).toInt();
			uint8_t _month = command.substring(14, 16).toInt();
			uint16_t _yr = command.substring(9, 13).toInt();
			setTime(_hr, _min, _sec, _day, _month, _yr);
			str += F("New time:");
			str += String(_yr);
			str += F(".");
			str += String(_month);
			str += F(".");
			str += String(_day);
			str += F(" ");
			str += String(_hr);
			str += F(":");
			str += String(_min);
			str += F(":");
			str += String(_sec);
			timeIsSet = true;
#ifdef NTP_TIME_ENABLE
			NTPenable = false;
#endif
		}
		else if (tmp.startsWith(F("check_period=")) && command.length() > 13)
		{
			checkSensorPeriod = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000UL);
			str += F("New sensor check period = \"");
			str += String(uint16_t(checkSensorPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(uint16_t(checkSensorPeriod / 1000UL)));
		}
#ifdef LOG_ENABLE
		else if (tmp.startsWith(F("log_period=")) && command.length() > 11)
		{
			logPeriod = uint32_t(atol(command.substring(command.indexOf('=') + 1).c_str()) * 1000UL);
			str += F("New log save period = \"");
			str += String(uint16_t(logPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(LOG_PERIOD_addr, LOG_PERIOD_size, String(uint16_t(logPeriod / 1000UL)));
		}
		//getLog - to be implemented
		else if (tmp == F("getlog="))
		{
			for (int i = 0; i < LOG_SIZE; i++)
			{
				if (history_log[i].year > 0)
				{
					str += ParseSensorReport(history_log[i], EOL, true);
					switch (channel)
					{
					case CHANNEL_UART:
						Serial.println(str);
						break;
					case CHANNEL_TELNET:

						break;
					case CHANNEL_MQTT:

						break;
					case CHANNEL_TELEGRAM:

						break;
					case CHANNEL_GSCRIPT:

						break;
					case CHANNEL_PUSHINGBOX:

						break;
					case CHANNEL_EMAIL:

						break;
					case CHANNEL_GSM:

						break;
					default:
						break;
					};
				}
			}
		}
#endif
		else if (tmp.startsWith(F("device_name=")) && command.length() > 12)
		{
			deviceName = command.substring(command.indexOf('=') + 1);
			str += F("New device name = \"");
			str += deviceName;
			str += QUOTE;
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
		}
		else if (tmp.startsWith(F("sta_ssid=")) && command.length() > 9)
		{
			String STA_Ssid = command.substring(command.indexOf('=') + 1);
			str += F("New STA SSID = \"");
			str += STA_Ssid;
			str += QUOTE;
			writeConfigString(STA_SSID_addr, STA_SSID_size, STA_Ssid);
		}
		else if (tmp.startsWith(F("sta_pass=")) && command.length() > 9)
		{

			String STA_Password = command.substring(command.indexOf('=') + 1);
			STA_Password.trim();
			str += F("New STA password = \"");
			str += STA_Password;
			str += QUOTE;
			writeConfigString(STA_PASS_addr, STA_PASS_size, STA_Password);
		}
		else if (tmp.startsWith(F("ap_ssid=")) && command.length() > 8)
		{
			String AP_Ssid = command.substring(command.indexOf('=') + 1);
			AP_Ssid.trim();
			str += F("New AP SSID = \"");
			str += AP_Ssid;
			str += QUOTE;
			writeConfigString(AP_SSID_addr, AP_SSID_size, AP_Ssid);
		}
		else if (tmp.startsWith(F("ap_pass=")) && command.length() > 8)
		{
			String AP_Password = command.substring(command.indexOf('=') + 1);
			AP_Password.trim();
			str += F("New AP password = \"");
			str += AP_Password;
			str += QUOTE;
			writeConfigString(AP_PASS_addr, AP_PASS_size, AP_Password);
		}
		else if (tmp.startsWith(F("wifi_standart=")) && command.length() > 14)
		{
			String wifi_mode = command.substring(command.indexOf('=') + 1);
			if (wifi_mode == F("B") || wifi_mode == F("G") || wifi_mode == F("N"))
			{
				str += F("New WiFi standart  = \"");
				str += String(wifi_mode);
				str += QUOTE;
				writeConfigString(WIFI_STANDART_addr, WIFI_STANDART_size, wifi_mode);
			}
			else
			{
				str = F("Incorrect WiFi standart: ");
				str += String(wifi_mode);
			}
		}
		else if (tmp.startsWith(F("wifi_power=")) && command.length() > 11)
		{
			float power = command.substring(command.indexOf('=') + 1).toFloat();
			if (power >= 0 && power <= 20.5)
			{
				str += F("New WiFi power  = \"");
				str += String(power);
				str += QUOTE;
				writeConfigFloat(WIFI_POWER_addr, power);
			}
			else
			{
				str = F("Incorrect WiFi power: ");
				str += String(power);
			}
		}
		else if (tmp.startsWith(F("wifi_mode=")) && command.length() > 10)
		{
			String wifi_modeStr = command.substring(command.indexOf('=') + 1);
			wifi_modeStr.trim();
			wifi_modeStr.toLowerCase();
			uint8_t wifi_mode = 255;
			if (wifi_modeStr == F("auto"))
			{
				wifi_mode = WIFI_MODE_AUTO;
			}
			else if (wifi_modeStr == F("station"))
			{
				wifi_mode = WIFI_MODE_STA;
			}
			else if (wifi_modeStr == F("apstation"))
			{
				wifi_mode = WIFI_MODE_AP_STA;
			}
			else if (wifi_modeStr == F("ap"))
			{
				wifi_mode = WIFI_MODE_AP;
			}
			else if (wifi_modeStr == F("off"))
			{
				wifi_mode = WIFI_MODE_OFF;
			}
			else
			{
				str = F("Incorrect value: ");
				str += wifi_modeStr;
			}
			if (wifi_mode != 255)
			{
				str += F("New WiFi mode  = \"");
				str += wifiModes[wifi_mode];
				str += QUOTE;
				writeConfigString(WIFI_MODE_addr, WIFI_MODE_size, String(wifi_mode));
			}
			else
			{
				str = F("Incorrect WiFi mode: ");
				str += String(wifi_modeStr);

			}
		}
		else if (tmp.startsWith(F("wifi_connect_time=")) && command.length() > 18)
		{
			connectTimeLimit = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000UL);
			str += F("New WiFi connect time limit = \"");
			str += String(uint16_t(connectTimeLimit / 1000UL));
			str += QUOTE;
			writeConfigString(CONNECT_TIME_addr, CONNECT_TIME_size, String(uint16_t(connectTimeLimit / 1000UL)));
		}
		else if (tmp.startsWith(F("wifi_reconnect_period=")) && command.length() > 22)
		{
			reconnectPeriod = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000UL);
			str += F("New WiFi disconnect period = \"");
			str += String(uint16_t(reconnectPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size, String(uint16_t(reconnectPeriod / 1000UL)));
		}
		else if (tmp.startsWith(F("wifi_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				wifiEnable = false;
				Start_OFF_Mode();
				str += F("WiFi disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				wifiEnable = true;
				startWiFi();
				str += F("WiFi enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}

		else if (tmp.startsWith(F("telnet_port=")) && command.length() > 12)
		{
			uint16_t telnetPort = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New telnet port = \"");
			str += String(telnetPort);
			str += QUOTE;
			writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
			//WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith(F("telnet_enable=")) && command.length() > 14)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				telnetEnable = false;
				telnetServer.stop();
				str += F("Telnet disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				telnetEnable = true;
				telnetServer.begin();
				telnetServer.setNoDelay(true);
				str += F("Telnet enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}

		else if (tmp == F("reset"))
		{
			if (channel == CHANNEL_TELEGRAM)
			{
				uartCommand = F("reset");
				uartReady = false;
			}
			else
			{
				//Serial.println(F("Resetting..."));
				//Serial.flush();
				ESP.restart();
			}
		}

		else if (tmp.startsWith(F("autoreport=")) && command.length() > 11)
		{
			autoReport = command.substring(command.indexOf('=') + 1).toInt();
			writeConfigString(AUTOREPORT_addr, AUTOREPORT_size, String(autoReport));
			str += F("New autoreport channels=");
			for (uint8_t b = 0; b < channels->length(); b++)
			{
				if (bitRead(autoReport, b))
				{
					str += channels[b];
					str += F(",");
				}
				yield();
			}
		}

		else if (tmp.startsWith(F("set_pin_mode")) && command.length() > 14)
		{
			int t = command.indexOf('=');
			if (t >= 13 && t < command.length() - 1)
			{
				uint8_t outNum = command.substring(12, t).toInt();
				if (outNum >= 1 && outNum <= PIN_NUMBER && !bitRead(SIGNAL_PINS, outNum - 1))
				{
					String pinMode = command.substring(t + 1);
					pinMode.toUpperCase();
					uint8_t intModeNum = 255;
					if (pinMode == pinModeList[3]) intModeNum = 3; //off
					else if (pinMode == pinModeList[INPUT]) intModeNum = INPUT;
					else if (pinMode == pinModeList[OUTPUT]) intModeNum = OUTPUT;
					else if (pinMode == pinModeList[INPUT_PULLUP]) intModeNum = INPUT_PULLUP;
					else
					{
						str = F("Incorrect pin mode: ");
						str += String(pinMode);
					}
					if (intModeNum != 255)
					{
						uint16_t l = PIN_MODE_size / PIN_NUMBER;
						writeConfigString(PIN_MODE_addr + (outNum - 1) * l, l, String(intModeNum));

						str += F("New pin");
						str += String(outNum);
						str += F(" mode: ");
						str += pinModeList[intModeNum];
					}
				}
				else
				{
					str = F("Incorrect pin: ");
					str += String(outNum);
				}
			}
		}
		else if (tmp.startsWith(F("set_interrupt_mode")) && command.length() > 20)
		{
			int t = command.indexOf('=');
			if (t >= 19 && t < command.length() - 1)
			{
				uint8_t outNum = command.substring(18, t).toInt();
				if (outNum >= 1 && outNum <= PIN_NUMBER && !bitRead(SIGNAL_PINS, outNum - 1))
				{
					String intMode = command.substring(t + 1);
					intMode.trim();
					intMode.toUpperCase();
					uint8_t intModeNum = 255;
					if (intMode == intModeList[0]) intModeNum = 0;
					else if (intMode == intModeList[RISING]) intModeNum = RISING;
					else if (intMode == intModeList[FALLING]) intModeNum = FALLING;
					else if (intMode == intModeList[CHANGE]) intModeNum = CHANGE;
					else
					{
						str = F("Incorrect INT mode: ");
						str += String(intMode);
					}
					if (intModeNum != 255)
					{
						uint16_t l = INTERRUPT_MODE_size / PIN_NUMBER;
						writeConfigString(INTERRUPT_MODE_addr + (outNum - 1) * l, l, String(intModeNum));
						str += F("New INT");
						str += String(outNum);
						str += F(" mode: ");
						str += intModeList[intModeNum];
					}
				}
				else
				{
					str = F("Incorrect INT: ");
					str += String(outNum);
				}
			}
		}
		else if (tmp.startsWith(F("set_init_output")) && command.length() > 17)
		{
			int t = command.indexOf('=');
			if (t >= 16 && t < command.length() - 1)
			{
				uint8_t outNum = command.substring(15, t).toInt();
				if (outNum >= 1 && outNum <= PIN_NUMBER && !bitRead(SIGNAL_PINS, outNum - 1))
				{
					String outStateStr = command.substring(t + 1);
					if (outStateStr != SWITCH_ON && outStateStr != SWITCH_OFF)
					{
						uint16_t outState = 0;
						outState = outStateStr.toInt();
						outStateStr = String(outState);
					}
					uint16_t l = OUTPUT_INIT_size / PIN_NUMBER;
					writeConfigString(OUTPUT_INIT_addr + (outNum - 1) * l, l, outStateStr);
					str += F("New OUT");
					str += String(outNum);
					str += F(" initial state: ");
					str += outStateStr;
				}
				else
				{
					str = F("Incorrect OUT");
					str += String(outNum);
				}
			}
		}
		else if (tmp.startsWith(F("set_output")) && command.length() > 12)
		{
			str += set_output(command);
		}

#ifdef SLEEP_ENABLE
		else if (tmp.startsWith(F("sleep_enable=")) && command.length() > 13)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				//sleepEnable = false;
				writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_OFF_NUMBER);
				str += F("SLEEP mode disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				//sleepEnable = true;
				writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_ON_NUMBER);
				str += F("SLEEP mode enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
		else if (tmp.startsWith(F("sleep_on=")) && command.length() > 9)
		{
			activeTimeOut_ms = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000UL);
			if (activeTimeOut_ms == 0) activeTimeOut_ms = uint32_t(2 * connectTimeLimit);

			str += F("New sleep ON period = \"");
			str += String(uint16_t(activeTimeOut_ms / 1000UL));
			str += QUOTE;
			writeConfigString(SLEEP_ON_addr, SLEEP_ON_size, String(uint16_t(activeTimeOut_ms / 1000UL)));
		}
		else if (tmp.startsWith(F("sleep_off=")) && command.length() > 10)
		{
			sleepTimeOut_us = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000000UL);
			if (sleepTimeOut_us == 0) sleepTimeOut_us = uint32_t(20UL * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)

			str += F("New sleep OFF period = \"");
			str += String(uint16_t(sleepTimeOut_us / 1000000UL));
			str += QUOTE;
			writeConfigString(SLEEP_OFF_addr, SLEEP_OFF_size, String(uint16_t(sleepTimeOut_us / 1000000UL)));
		}
#endif

#ifdef HTTP_SERVER_ENABLE
		else if (tmp.startsWith(F("http_port=")) && command.length() > 10)
		{
			httpPort = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New HTTP port = \"");
			str += String(httpPort);
			str += QUOTE;
			writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
			//if (httpServerEnable) http_server.begin(httpPort);
		}
		else if (tmp.startsWith(F("http_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_OFF_NUMBER);
				//httpServerEnable = false;
				//if (httpServerEnable) http_server.stop();
				str += F("HTTP server disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_ON_NUMBER);
				/*httpServerEnable = true;
				http_server.on("/", handleRoot);
				http_server.onNotFound(handleNotFound);
				http_server.begin(httpPort);*/
				str += F("HTTP server enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef MQTT_ENABLE
		// send_mqtt=topic,message - to be implemented
		else if (tmp.startsWith(F("send_mqtt=")))
		{
			String topic = tmp.substring(10, tmp.indexOf(','));
			if (topic.length() > 0)
			{
				String tmpMessage = command.substring(command.indexOf(',') + 1);
				if (tmpMessage.length() > 0)
				{
					if (mqtt_send(tmpMessage, tmpMessage.length(), topic))
					{
						str += F("Mail sent");
					}
					else
					{
						str += F("Mail not sent");
					}
				}
			}
		}

		else if (tmp.startsWith(F("mqtt_server=")) && command.length() > 12)
		{
			String tmpSrv = (command.substring(command.indexOf('=') + 1));
			tmpSrv.trim();
			IPAddress tmpAddr;
			if (tmpAddr.fromString(tmpSrv))
			{
				IPAddress mqtt_ip_server = tmpAddr;
				str += F("New MQTT server = \"");
				str += mqtt_ip_server.toString();
				str += QUOTE;
				writeConfigString(MQTT_SERVER_addr, MQTT_SERVER_size, mqtt_ip_server.toString());
			}
			else
			{
				str += F("New MQTT server = \"");
				str += tmpSrv;
				str += QUOTE;
				writeConfigString(MQTT_SERVER_addr, MQTT_SERVER_size, tmpSrv);
			}
		}
		else if (tmp.startsWith(F("mqtt_port=")) && command.length() > 10)
		{
			uint16_t mqtt_port = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New MQTT port = \"");
			str += String(mqtt_port);
			str += QUOTE;
			writeConfigString(MQTT_PORT_addr, MQTT_PORT_size, String(mqtt_port));
			//mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else if (tmp.startsWith(F("mqtt_login=")) && command.length() > 11)
		{
			String mqtt_User = command.substring(command.indexOf('=') + 1);
			mqtt_User.trim();
			str += F("New MQTT login = \"");
			str += mqtt_User;
			str += QUOTE;
			writeConfigString(MQTT_USER_addr, MQTT_USER_size, mqtt_User);
		}
		else if (tmp.startsWith(F("mqtt_pass=")) && command.length() > 10)
		{
			String mqtt_Password = command.substring(command.indexOf('=') + 1);
			mqtt_Password.trim();
			str += F("New MQTT password = \"");
			str += mqtt_Password;
			str += QUOTE;
			writeConfigString(MQTT_PASS_addr, MQTT_PASS_size, mqtt_Password);
		}
		else if (tmp.startsWith(F("mqtt_id=")) && command.length() > 8)
		{
			String mqtt_device_id = command.substring(command.indexOf('=') + 1);
			mqtt_device_id.trim();
			str += F("New MQTT ID = \"");
			str += mqtt_device_id;
			str += QUOTE;
			writeConfigString(MQTT_ID_addr, MQTT_ID_size, mqtt_device_id);
		}
		else if (tmp.startsWith(F("mqtt_topic_in=")) && command.length() > 8)
		{
			String mqtt_topic_in = command.substring(command.indexOf('=') + 1);
			mqtt_topic_in.trim();
			str += F("New MQTT SUBSCRIBE topic = \"");
			str += mqtt_topic_in;
			str += QUOTE;
			writeConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size, mqtt_topic_in);
		}
		else if (tmp.startsWith(F("mqtt_topic_out=")) && command.length() > 8)
		{
			String mqtt_topic_out = command.substring(command.indexOf('=') + 1);
			mqtt_topic_out.trim();
			str += F("New MQTT POST topic = \"");
			str += mqtt_topic_out;
			str += QUOTE;
			writeConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size, mqtt_topic_out);
		}
		else if (tmp.startsWith(F("mqtt_clean=")) && command.length() > 11)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			stateStr.trim();
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
				str += F("New MQTT clean session: ");
				str += stateStr;
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size, SWITCH_ON_NUMBER);
				str += F("New MQTT clean session: ");
				str += stateStr;
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
		else if (tmp.startsWith(F("mqtt_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			stateStr.trim();
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
				mqttEnable = false;
				str += F("MQTT disabled");
				mqtt_client.disconnect();
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_ON_NUMBER);
				mqttEnable = true;
				str += F("MQTT enabled");
				mqtt_connect();
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef SMTP_ENABLE
		// send_mail=address,message
		else if (tmp.startsWith(F("send_mail=")))
		{
			String addressTo = tmp.substring(10, tmp.indexOf(','));
			if (addressTo.length() > 0)
			{
				String tmpMessage = command.substring(command.indexOf(',') + 1);
				if (tmpMessage.length() > 0)
				{
					String testMessage = F("Test");
					if (sendMail(testMessage, tmpMessage, addressTo))
					{
						str += F("Mail sent");
					}
					else
					{
						str += F("Mail send failed");
					}
				}
				else str += F("Message empty");
			}
			else str += F("Address empty");
		}

		else if (tmp.startsWith(F("smtp_server=")) && command.length() > 12)
		{
			String smtpServerAddress = command.substring(command.indexOf('=') + 1);
			smtpServerAddress.trim();
			str += F("New SMTP server IP address = \"");
			str += smtpServerAddress;
			str += QUOTE;
			writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, smtpServerAddress);
		}
		else if (tmp.startsWith(F("smtp_port=")) && command.length() > 10)
		{
			uint16_t smtpServerPort = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New SMTP port = \"");
			str += String(smtpServerPort);
			str += QUOTE;
			writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
		}
		else if (tmp.startsWith(F("smtp_login=")) && command.length() > 11)
		{
			String smtpLogin = command.substring(command.indexOf('=') + 1);
			smtpLogin.trim();
			str += F("New SMTP login = \"");
			str += smtpLogin;
			str += QUOTE;
			writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, smtpLogin);
		}
		else if (tmp.startsWith(F("smtp_pass=")) && command.length() > 10)
		{
			String smtpPassword = command.substring(command.indexOf('=') + 1);
			smtpPassword.trim();
			str += F("New SMTP password = \"");
			str += smtpPassword;
			str += QUOTE;
			writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, smtpPassword);
		}
		else if (tmp.startsWith(F("smtp_to=")) && command.length() > 8)
		{
			String smtpTo = command.substring(command.indexOf('=') + 1);
			smtpTo.trim();
			str += F("New SMTP TO address = \"");
			str += smtpTo;
			str += QUOTE;
			writeConfigString(SMTP_TO_addr, SMTP_TO_size, smtpTo);
		}
		else if (tmp.startsWith(F("smtp_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_OFF_NUMBER);
				//smtpEnable = false;
				str += F("SMTP reporting disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_ON_NUMBER);
				//smtpEnable = true;
				str += F("SMTP reporting enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef GSM_ENABLE
		// send_sms=[*/user#],message
		else if (tmp.startsWith(F("send_sms=")))
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (tmp[9] == '*') //if * then all
			{
				i = 0;
				j = gsmUsersNumber;
			}
			else
			{
				i = tmp.substring(9, tmp.indexOf(',')).toInt();
				j = i + 1;
			}

			if (i <= gsmUsersNumber)
			{
				String tmpMessage = command.substring(command.indexOf(',') + 1);
				for (; i < j; i++)
				{
					String gsmUser = getGsmUser(i);
					if (gsmUser.length() > 0)
					{
						if (sendSMS(gsmUser, tmpMessage))
						{
							str += F("SMS sent");
						}
						else
						{
							str += F("SMS not sent");
						}
					}
					yield();
				}
			}
		}

		else if (tmp.startsWith(F("gsm_user")) && command.length() > 10)
		{
			int t = command.indexOf('=');
			if (t > 0 && t < command.length() - 1)
			{
				uint8_t userNum = command.substring(8, t).toInt();
				String newUser = command.substring(t + 1);
				if (userNum >= 0 && userNum < gsmUsersNumber)
				{
					uint16_t m = GSM_USERS_TABLE_size / gsmUsersNumber;
					writeConfigString(GSM_USERS_TABLE_addr + userNum * m, m, newUser);
					str += F("New user");
					str += String(userNum);
					str += F(" is now: ");
					str += newUser;
				}
				else
				{
					str = F("User #");
					str += String(userNum);
					str += F(" is out of range [0,");
					str += String(gsmUsersNumber - 1);
					str += F("]");
				}
			}
		}
		else if (tmp.startsWith(F("gsm_enable=")) && command.length() > 10)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_OFF_NUMBER);
				//telegramEnable = false;
				str += F("GSM disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_ON_NUMBER);
				gsmEnable = true;
				str += F("GSM enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef TELEGRAM_ENABLE
		// send_telegram=[*/user#],message
		else if (tmp.startsWith(F("send_telegram=")))
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (tmp[14] == '*') //if * then all
			{
				i = 0;
				j = gsmUsersNumber;
			}
			else
			{
				i = tmp.substring(14, tmp.indexOf(',')).toInt();
				j = i + 1;
			}

			if (i <= gsmUsersNumber)
			{
				String tmpMessage = command.substring(command.indexOf(',') + 1);
				for (; i < j; i++)
				{
					int64_t gsmUser = getTelegramUser(i);
					if (gsmUser != 0)
					{
						if (sendToTelegramServer(gsmUser, tmpMessage))
						{
							str += F("Message sent");
						}
						else
						{
							str += F("Message not sent");
						}
					}
					yield();
				}
			}
		}

		else if (tmp.startsWith(F("telegram_user")) && command.length() > 10)
		{
			int t = command.indexOf('=');
			if (t > 0 && t < command.length() - 1)
			{
				uint8_t userNum = command.substring(13, t).toInt();
				uint64_t newUser = StringToUint64(command.substring(t + 1));
				if (userNum >= 0 && userNum < telegramUsersNumber)
				{
					uint16_t m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
					writeConfigString(TELEGRAM_USERS_TABLE_addr + userNum * m, m, uint64ToString(newUser));
					str += F("New user");
					str += uint64ToString(userNum);
					str += F(" is now: ");
					str += uint64ToString(newUser);
				}
				else
				{
					str += F("User #");
					str += String(userNum);
					str += F(" is out of range [0,");
					str += String(telegramUsersNumber - 1);
					str += F("]");
				}
			}
		}
		else if (tmp.startsWith(F("telegram_token=")) && command.length() > 15)
		{
			String telegramToken = command.substring(command.indexOf('=') + 1);
			telegramToken.trim();
			str += F("New TELEGRAM token = \"");
			str += telegramToken;
			str += QUOTE;
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, telegramToken);
			//myBot.setTelegramToken(telegramToken);
		}
		else if (tmp.startsWith(F("telegram_enable=")) && command.length() > 16)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_OFF_NUMBER);
				//telegramEnable = false;
				str += F("Telegram disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_ON_NUMBER);
				//telegramEnable = true;
				//myBot.wifiConnect(STA_Ssid, STA_Password);
				//myBot.setTelegramToken(telegramToken);
				//myBot.testConnection();
				str += F("Telegram enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef GSCRIPT_ENABLE
		//send_gscript - to be implemented

		else if (tmp.startsWith(F("gscript_token=")) && command.length() > 14)
		{
			String gScriptId = command.substring(command.indexOf('=') + 1);
			gScriptId.trim();
			str += F("New GScript token = \"");
			str += gScriptId;
			str += QUOTE;
			writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, gScriptId);
		}
		else if (tmp.startsWith(F("gscript_enable=")) && command.length() > 15)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_OFF_NUMBER);
				//gScriptEnable = false;
				str += F("GScript disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_ON_NUMBER);
				//gScriptEnable = true;
				str += F("GScript enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef PUSHINGBOX_ENABLE
		//send_pushingbox - to be implemented

		else if (tmp.startsWith(F("pushingbox_token=")) && command.length() > 17)
		{
			String pushingBoxId = command.substring(command.indexOf('=') + 1);
			pushingBoxId.trim();
			str += F("New PushingBox token = \"");
			str += pushingBoxId;
			str += QUOTE;
			writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, pushingBoxId);
		}
		else if (tmp.startsWith(F("pushingbox_parameter=")) && command.length() > 21)
		{
			String pushingBoxParameter = command.substring(command.indexOf('=') + 1);
			pushingBoxParameter.trim();
			str += F("New PUSHINGBOX_ENABLE parameter name = \"");
			str += pushingBoxParameter;
			str += QUOTE;
			writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, pushingBoxParameter);
		}
		else if (tmp.startsWith(F("pushingbox_enable=")) && command.length() > 18)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_OFF_NUMBER);
				//pushingBoxEnable = false;
				str += F("PushingBox disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_ON_NUMBER);
				//pushingBoxEnable = true;
				str += F("PushingBox enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef NTP_TIME_ENABLE
		else if (tmp.startsWith(F("ntp_server=")) && command.length() > 11)
		{
			String ntpServer = command.substring(command.indexOf('=') + 1);
			ntpServer.trim();
			str += F("New NTP server = \"");
			str += ntpServer;
			str += QUOTE;
			writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, ntpServer);
		}
		else if (tmp.startsWith(F("ntp_time_zone=")) && command.length() > 14)
		{
			int timeZone = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New NTP time zone = \"");
			str += String(timeZone);
			str += QUOTE;
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
		}
		else if (tmp.startsWith(F("ntp_refresh_delay=")) && command.length() > 18)
		{
			ntpRefreshDelay = command.substring(command.indexOf('=') + 1).toInt();
			str += F("New NTP refresh delay = \"");
			str += String(ntpRefreshDelay);
			str += QUOTE;
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);
		}
		else if (tmp.startsWith(F("ntp_enable=")) && command.length() > 11)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_OFF_NUMBER);
				NTPenable = false;
				setSyncProvider(nullptr);
				Udp.stop();
				str += F("NTP disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_ON_NUMBER);
				NTPenable = true;
				Udp.begin(UDP_LOCAL_PORT);
				setSyncProvider(getNtpTime);
				setSyncInterval(ntpRefreshDelay);
				str += F("NTP enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
#endif

#ifdef EVENTS_ENABLE		
		else if (tmp.startsWith(F("set_event")) && command.length() > 11)
		{
			int t = command.indexOf('=');
			if (t >= 10 && t < command.length() - 1)
			{
				uint8_t eventNum = command.substring(9, t).toInt();
				String newEvent = command.substring(t + 1);
				newEvent.trim();
				if (eventNum >= 0 && eventNum < eventsNumber)
				{
					uint16_t m = EVENTS_TABLE_size / eventsNumber;
					writeConfigString(EVENTS_TABLE_addr + eventNum * m, m, newEvent);
					str += F("New event");
					str += String(eventNum);
					str += F(" is now: ");
					str += newEvent;
				}
				else
				{
					str = F("Event #");
					str += String(eventNum);
					str += F(" is out of range [0,");
					str += String(eventsNumber - 1);
					str += F("]");
				}
			}
		}
		else if (tmp.startsWith(F("events_enable=")) && command.length() > 14)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
				eventsEnable = false;
				str += F("Events disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
				eventsEnable = true;
				str += F("Events enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
		else if (tmp.startsWith(F("set_event_flag")) && command.length() > 16)
		{
			int t = tmp.indexOf('=');
			if (t >= 15 && t < tmp.length() - 1)
			{
				uint8_t eventNum = tmp.substring(14, t).toInt();
				if (eventNum >= 0 && eventNum < eventsNumber)
				{
					String stateStr = tmp.substring(t + 1);
					stateStr.trim();
					if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
					{
						writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
						eventsFlags[eventNum] = false;
					}
					else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
					{
						writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
						eventsFlags[eventNum] = true;
					}
				}
			}
		}
#endif

#ifdef SCHEDULER_ENABLE
		else if (tmp.startsWith(F("set_schedule")) && command.length() > 14)
		{
			int t = command.indexOf('=');
			if (t >= 13 && t < command.length() - 1)
			{
				uint8_t scheduleNum = command.substring(12, t).toInt();
				String newSchedule = command.substring(t + 1);
				newSchedule.trim();
				if (scheduleNum >= 0 && scheduleNum < schedulesNumber)
				{
					uint16_t m = SCHEDULER_TABLE_size / schedulesNumber;
					writeConfigString(SCHEDULER_TABLE_addr + scheduleNum * m, m, newSchedule);
					writeScheduleExecTime(scheduleNum, 0);
					str += F("New schedule");
					str += String(scheduleNum);
					str += F(" is now: ");
					str += newSchedule;
				}
				else
				{
					str = F("Schedule #");
					str += String(scheduleNum);
					str += F(" is out of range [0,");
					str += String(schedulesNumber - 1);
					str += F("]");
				}
			}
		}
		else if (tmp.startsWith(F("scheduler_enable=")) && command.length() > 17)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_OFF_NUMBER);
				schedulerEnable = false;
				str += F("Scheduler disabled");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_ON_NUMBER);
				schedulerEnable = true;
				str += F("Scheduler enabled");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
			}
		}
		else if (tmp.startsWith(F("clear_schedule_exec_time")) && command.length() > 24)
		{
			uint8_t scheduleNum = tmp.substring(24).toInt();
			if (scheduleNum >= 0 && scheduleNum < schedulesNumber)
			{
				writeScheduleExecTime(scheduleNum, 0);
			}
		}
#endif

#ifdef DISPLAY_ENABLED
		else if (tmp.startsWith(F("display_refresh=")) && command.length() > 16)
		{
			displaySwitchPeriod = uint32_t(command.substring(command.indexOf('=') + 1).toInt() * 1000UL);
			str += F("New display refresh period = \"");
			str += String(uint16_t(displaySwitchPeriod));
			str += QUOTE;
			writeConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size, String(uint16_t(displaySwitchPeriod / 1000UL)));
		}
#endif
	}
	if (str == "")
	{
		str = F("Incorrect command: \"");
		str += command;
		str += QUOTE;
	}
	str += EOL;
	return str;
}

#if defined(EVENTS_ENABLE) || defined(SCHEDULER_ENABLE)
String printHelpAction()
{
	return F("Actions:\r\n"
		"command=[any command]\r\n"
		"set_output?=[on, off, 0..1023]\r\n"
		"set_counter ? = x\r\n"
		"reset_counter?\r\n"
		"set_event_flag?=0/1\r\n"
		"reset_flag?\r\n"
		"set_flag?\r\n"
		"clear_schedule_exec_time?\r\n"
		"send_telegram=[*, user#],message\r\n"
		"send_pushingbox=message\r\n"
		"send_mail=address,message\r\n"
		"send_gscript=message\r\n"
		"send_mqtt=topic,message\r\n"
		"send_sms=[*, user#],message\r\n"
		"save_log\r\n");
}

void ProcessAction(String& action, uint8_t eventNum, bool isEvent)
{
	if (isEvent)
	{
#ifdef EVENTS_ENABLE
		eventsFlags[eventNum] = true;
#endif
	}
	else
	{
#ifdef SCHEDULER_ENABLE
		writeScheduleExecTime(eventNum, now());
#endif
	}

	do
	{
		int t = action.indexOf(CSV_DIVIDER);
		String tmpAction;
		if (t > 0)
		{
			tmpAction = action.substring(0, t);
			action = action.substring(t + 1);
		}
		else
		{
			tmpAction = action;
			action = "";
		}
		if (tmpAction.startsWith(F("command=")) && tmpAction.length() > 9)
		{
			processCommand(tmpAction.substring(8), CHANNEL_UART, true);
		}
		//set_output?=x
		else if (tmpAction.startsWith(F("set_output")) && tmpAction.length() > 11)
		{
			set_output(tmpAction);
		}

		// deprecated
		//reset_counter?
		else if (tmpAction.startsWith(F("reset_counter")) && tmpAction.length() > 13)
		{
			uint8_t counterNum = tmpAction.substring(13).toInt();
			if (counterNum >= 0 && counterNum < PIN_NUMBER)
			{
				InterruptCounter[counterNum - 1] = 0;
			}
		}

		//set_counter?=x
		else if (tmpAction.startsWith(F("set_counter")) && tmpAction.length() > 12)
		{
			int t = tmpAction.indexOf('=');
			if (t >= 11 && t < tmpAction.length() - 1)
			{
				uint8_t counterNum = tmpAction.substring(11, t).toInt();
				if (counterNum >= 0 && counterNum < PIN_NUMBER)
				{
					String stateStr = tmpAction.substring(t + 1);
					stateStr.trim();
					InterruptCounter[counterNum - 1] = stateStr.toInt();
				}
			}
		}

#ifdef EVENTS_ENABLE
		//reset_flag?
		else if (tmpAction.startsWith(F("reset_flag")) && tmpAction.length() > 10)
		{
			uint8_t eventNum = tmpAction.substring(10).toInt();
			if (eventNum >= 0 && eventNum < eventsNumber)
			{
				eventsFlags[eventNum] = false;
			}
		}
		//set_flag?
		else if (tmpAction.startsWith(F("set_flag")) && tmpAction.length() > 8)
		{
			int eventNum = tmpAction.substring(8).toInt();
			if (eventNum >= 0 && eventNum < eventsNumber)
			{
				eventsFlags[eventNum] = true;
			}
		}
		//set_event_flag?=0/1/off/on
		else if (tmpAction.startsWith(F("set_event_flag")) && tmpAction.length() > 16)
		{
			int t = tmpAction.indexOf('=');
			if (t >= 15 && t < tmpAction.length() - 1)
			{
				uint8_t eventNum = tmpAction.substring(14, t).toInt();
				if (eventNum >= 0 && eventNum < eventsNumber)
				{
					String stateStr = tmpAction.substring(t + 1);
					stateStr.trim();
					if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
					{
						writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
						eventsFlags[eventNum] = false;
					}
					else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
					{
						writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
						eventsFlags[eventNum] = true;
					}
				}
			}
		}

#endif
#ifdef SCHEDULER_ENABLE
		//clear_schedule_exec_time?
		else if (tmpAction.startsWith(F("clear_schedule_exec_time")) && tmpAction.length() > 24)
		{
			uint8_t scheduleNum = tmpAction.substring(24).toInt();
			if (scheduleNum >= 0 && scheduleNum < schedulesNumber)
			{
				writeScheduleExecTime(scheduleNum, 0);
			}
		}
#endif
#ifdef GSM_ENABLE
		//send_sms=* / user#,message
		else if (gsmEnable && tmpAction.startsWith(F("send_sms=")))
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (tmpAction[9] == '*') //if * then all
			{
				i = 0;
				j = gsmUsersNumber;
			}
			else
			{
				i = tmpAction.substring(9, tmpAction.indexOf(',')).toInt();
				j = i + 1;
			}
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			sensorDataCollection sensorData = collectData();
			for (; i < j; i++)
			{
				String gsmUser = getGsmUser(i);
				if (gsmUser.length() > 0)
				{
					String tmpStr = deviceName;
					tmpStr += F(":");
					tmpStr += CSV_DIVIDER;
					tmpStr += tmpAction;
					tmpStr += EOL;
					tmpStr += ParseSensorReport(sensorData, EOL);
					bool sendOk = sendSMS(gsmUser, tmpStr);
					if (!sendOk)
					{
						/*#ifdef EVENTS_ENABLE
												eventsFlags[eventNum] = false;
						#endif*/
					}
				}
				yield();
			}
		}
#endif
#ifdef TELEGRAM_ENABLE
		//send_telegram=* / user#,message
		else if (telegramEnable && tmpAction.startsWith(F("send_telegram=")))
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (tmpAction[14] == '*') //if * then all
			{
				i = 0;
				j = telegramUsersNumber;
			}
			else
			{
				i = tmpAction.substring(14, tmpAction.indexOf(',')).toInt();
				j = i + 1;
			}
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			sensorDataCollection sensorData = collectData();
			for (; i < j; i++)
			{
				uint64_t user = getTelegramUser(i);
				if (user > 0)
				{
					String tmpStr = deviceName;
					tmpStr += F(":");
					tmpStr += CSV_DIVIDER;
					tmpStr += tmpAction;
					tmpStr += EOL;
					tmpStr += ParseSensorReport(sensorData, EOL);
					bool sendOk = sendToTelegramServer(user, tmpStr);
					if (!sendOk)
					{
						/*#ifdef EVENTS_ENABLE
												eventsFlags[eventNum] = false;
						#endif*/
					}
				}
				yield();
			}
		}
#endif
#ifdef PUSHINGBOX_ENABLE
		//send_PushingBox=message
		else if (pushingBoxEnable && tmpAction.startsWith(F("send_pushingbox")))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			sensorDataCollection sensorData = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += tmpAction;
			tmpStr += EOL;
			tmpStr += ParseSensorReport(sensorData, EOL);
			bool sendOk = sendToPushingBox(tmpStr);
			if (!sendOk)
			{
				/*#ifdef EVENTS_ENABLE
								eventsFlags[eventNum] = false;
				#endif*/
			}
		}
#endif
#ifdef SMTP_ENABLE
		//send_mail=address_to,message
		else if (smtpEnable && tmpAction.startsWith(F("send_mail")))
		{
			String address = tmpAction.substring(10, tmpAction.indexOf(','));
			address.trim();
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			sensorDataCollection sensorData = collectData();
			String tmpStr = tmpAction;
			tmpStr += EOL;
			tmpStr += ParseSensorReport(sensorData, EOL);
			String subj = deviceName + " alert";
			bool sendOk = sendMail(subj, tmpStr, address);
			if (!sendOk)
			{
				/*#ifdef EVENTS_ENABLE
								eventsFlags[eventNum] = false;
				#endif*/
			}
		}
#endif
#ifdef GSCRIPT_ENABLE
		//send_GScript=message
		else if (gScriptEnable && tmpAction.startsWith(F("send_gscript")))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			sensorDataCollection sensorData = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += tmpAction;
			tmpStr += EOL;
			tmpStr += ParseSensorReport(sensorData, EOL);
			bool sendOk = sendValueToGoogle(tmpStr);
			if (!sendOk)
			{
				/*#ifdef EVENTS_ENABLE
								eventsFlags[eventNum] = false;
				#endif*/
			}
		}
#endif
#ifdef MQTT_ENABLE
		//send_MQTT=message
		if (mqttEnable && tmpAction.startsWith(F("send_mqtt")))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			String topicTo = tmpAction.substring(10, tmpAction.indexOf(','));
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			String delimiter = F("\",\r\n\"");
			String eq = F("\":\"");
			String tmpStr = F("{\"");
			tmpStr += F("Device name");
			tmpStr += eq;
			tmpStr += deviceName;
			tmpStr += delimiter;
			tmpStr += F("Message");
			tmpStr += eq;
			tmpStr += tmpAction;
			tmpStr += delimiter;
			tmpStr += F("\"}");

			bool sendOk = false;
			sendOk = mqtt_send(tmpStr, tmpStr.length(), topicTo);
			if (!sendOk)
			{
				/*#ifdef EVENTS_ENABLE
								eventsFlags[eventNum] = false;
				#endif*/
			}
		}
#endif
#ifdef LOG_ENABLE
		//save_log
		else if (tmpAction.startsWith(F("save_log")))
		{
			sensorDataCollection sensorData = collectData();
			addToLog(sensorData);
		}
#endif
		else
		{
			//Serial.print(F("Incorrect action: \""));
			//Serial.print(tmpAction);
			//Serial.println(quote);
		}
		yield();
	} while (action.length() > 0);
}
#endif

#ifdef TEMPERATURE_SENSOR
float getTemperature(sensorDataCollection& sensorData)
{
	float temp = -1000;
#ifdef MH_Z19_UART_ENABLE
	temp = sensorData.mh_temp;
#endif

#ifdef DHT_ENABLE
	temp = sensorData.dht_temp;
#endif

#ifdef BMP180_ENABLE
	if (bmp180_enable) temp = sensorData.bmp180_temp;
#endif

#ifdef BME280_ENABLE
	if (bme280_enable) temp = sensorData.bme280_temp;
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable) temp = sensorData.htu21d_temp;
#endif

#ifdef AMS2320_ENABLE
	if (am2320_enable) temp = sensorData.ams_temp;
#endif

#ifdef DS18B20_ENABLE
	if (ds1820_enable) temp = sensorData.ds1820_temp;
#endif
	return temp;
}
#endif

#ifdef HUMIDITY_SENSOR
float getHumidity(sensorDataCollection& sensorData)
{
	float humidity = -1000;
#ifdef DHT_ENABLE
	humidity = sensorData.dht_humidity;
#endif

#ifdef AMS2320_ENABLE
	if (am2320_enable) humidity = round(sensorData.ams_humidity);
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable) humidity = sensorData.htu21d_humidity;
#endif

#ifdef BME280_ENABLE
	if (bme280_enable) humidity = sensorData.bme280_humidity;
#endif

	return humidity;
}
#endif

#ifdef CO2_SENSOR
int getCo2(sensorDataCollection& sensorData)
{
	int co2_avg = -1000;
#if defined(MH_Z19_PPM_ENABLE)
	if (sensorData.mh_ppm_co2 > 0)
	{
		co2_ppm_avg[0] = co2_ppm_avg[1];
		co2_ppm_avg[1] = co2_ppm_avg[2];
		co2_ppm_avg[2] = sensorData.mh_ppm_co2;
		co2_avg = (co2_ppm_avg[0] + co2_ppm_avg[1] + co2_ppm_avg[2]) / 3;
	}
#endif

#ifdef MH_Z19_UART_ENABLE
	if (sensorData.mh_uart_co2 > 0)
	{
		co2_uart_avg[0] = co2_uart_avg[1];
		co2_uart_avg[1] = co2_uart_avg[2];
		co2_uart_avg[2] = sensorData.mh_uart_co2;
		co2_avg = (co2_uart_avg[0] + co2_uart_avg[1] + co2_uart_avg[2]) / 3;
	}
#endif

	return co2_avg;
}
#endif

String getStaSsid()
{
	return readConfigString(STA_SSID_addr, STA_SSID_size);
}

String getStaPassword()
{
	return readConfigString(STA_PASS_addr, STA_PASS_size);
}

String getApSsid()
{
	String AP_Ssid = readConfigString(AP_SSID_addr, AP_SSID_size);
	if (AP_Ssid.length() <= 0) AP_Ssid = deviceName + F("_") + WiFi.macAddress();
	return AP_Ssid;
}

String getApPassword()
{
	return readConfigString(AP_PASS_addr, AP_PASS_size);
}

WiFiPhyMode getWifiStandard()
{
	WiFiPhyMode wifi_standard = WIFI_PHY_MODE_11B; //WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	String wifi_phy = readConfigString(WIFI_STANDART_addr, WIFI_STANDART_size);
	//WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	if (wifi_phy == F("G"))
	{
		wifi_standard = WIFI_PHY_MODE_11G;
	}
	else if (wifi_phy == F("N"))
	{
		wifi_standard = WIFI_PHY_MODE_11N;
	}
	else
	{
		wifi_standard = WIFI_PHY_MODE_11B;
	}
	return wifi_standard;
}

float getWiFiPower()
{
	float power = readConfigFloat(WIFI_POWER_addr);
	if (power < 0 || power > 20.5) power = 20.5f;
	return power;
}

void startWiFi()
{
	wifi_mode = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (wifi_mode < 0 || wifi_mode > 4) wifi_mode = WIFI_MODE_AUTO;

	if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AUTO)
	{
		Start_STA_Mode();
		wiFiIntendedStatus = RADIO_CONNECTING;
	}
	else if (wifi_mode == WIFI_MODE_AP)
	{
		Start_AP_Mode();
		wiFiIntendedStatus = RADIO_WAITING;
	}
	else if (wifi_mode == WIFI_MODE_AP_STA)
	{
		Start_AP_STA_Mode();
		wiFiIntendedStatus = RADIO_CONNECTING;
	}
	else
	{
		Start_OFF_Mode();
		wiFiIntendedStatus = RADIO_STOP;
	}
}

void Start_OFF_Mode()
{
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.persistent(true);
	WiFi.mode(WIFI_OFF);
	currentWiFiMode = WIFI_MODE_OFF;
	delay(1000UL);
}

void Start_STA_Mode()
{
	String STA_Password = getStaPassword();
	String STA_Ssid = getStaSsid();
	WiFiPhyMode wifi_standard = getWifiStandard(); //WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	float wifi_OutputPower = getWiFiPower(); //[0 - 20.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setPhyMode(wifi_standard);
	WiFi.setOutputPower(wifi_OutputPower);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.mode(WIFI_STA);
	WiFi.hostname(deviceName);
	WiFi.begin(STA_Ssid.c_str(), STA_Password.c_str());
	ConnectionStarted = millis();
	currentWiFiMode = WIFI_MODE_STA;
}

void Start_AP_Mode()
{
	String AP_Ssid = getApSsid();
	String AP_Password = getApPassword();
	WiFiPhyMode wifi_standard = getWifiStandard(); //WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	float wifi_OutputPower = getWiFiPower(); //[0 - 20.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setPhyMode(wifi_standard);
	WiFi.setOutputPower(wifi_OutputPower);
	WiFi.mode(WIFI_AP);
	WiFi.hostname(deviceName);
	WiFi.softAP(AP_Ssid.c_str(), AP_Password.c_str());
	//WiFi.softAPConfig(apIP, apIPgate, IPAddress(192, 168, 4, 1));
	currentWiFiMode = WIFI_MODE_AP;
}

void Start_AP_STA_Mode()
{
	String STA_Password = getStaPassword();
	String STA_Ssid = getStaSsid();
	String AP_Ssid = getApSsid();
	String AP_Password = getApPassword();
	WiFiPhyMode wifi_standard = getWifiStandard(); //WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	float wifi_OutputPower = getWiFiPower(); //[0 - 20.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setPhyMode(wifi_standard);
	WiFi.setOutputPower(wifi_OutputPower);
	WiFi.mode(WIFI_AP_STA);
	WiFi.hostname(deviceName);
	WiFi.softAP(AP_Ssid.c_str(), AP_Password.c_str());
	WiFi.begin(STA_Ssid.c_str(), STA_Password.c_str());
	ConnectionStarted = millis();
	//WiFi.softAPConfig(apIP, apIPgate, IPAddress(192, 168, 4, 1));
	currentWiFiMode = WIFI_MODE_AP_STA;
}

void int1count()
{
	InterruptCounter[0]++;
}
void int2count()
{
	InterruptCounter[1]++;
}
void int3count()
{
	InterruptCounter[2]++;
}
void int4count()
{
	InterruptCounter[3]++;
}
void int5count()
{
	InterruptCounter[4]++;
}
void int6count()
{
	InterruptCounter[5]++;
}
void int7count()
{
	InterruptCounter[6]++;
}
void int8count()
{
	InterruptCounter[7]++;
}

/*
String MacToStr(const uint8_t mac[6])
{
	return String(mac[0])
		+ String(mac[1])
		+ String(mac[2])
		+ String(mac[3])
		+ String(mac[4])
		+ String(mac[5]);
}

uint16_t countOf(String& str, char c)
{
	uint16_t count = 0;
	for (int i = 0; i < str.length(); i++)
	{
		if (str[i] == c) count++;
		yield();
	}
	return count;
}*/
