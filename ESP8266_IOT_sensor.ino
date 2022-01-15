/* Untested feature:
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

/* Planned features:
- SSL/TLS MQTT, SMTP
- LLMNR responder
- TIMERS service (start_timer?=n; stop_timer? commands running actions)
- TELEGRAM
	- set TELEGRAM_URL "api.telegram.org"
	- set TELEGRAM_IP "149.154.167.198"
	- set TELEGRAM_PORT 443
	- setFingerprint
	- show simple keyboard
- temperature/humidity offset setting
- Amazon Alexa integration
- HTTP - settings page
- EVENT service - option to check action successful execution.
- GoogleScript service - memory allocation problem
- DEBUG mode verbose serial output
- option to use UART for GPRS/sensors
- GSM_MODEM status (connection state, network name)
- SMS message paging
- improve SMS management
- make common outbound message queue for all channels (SMS/Telegram/)
- write default settings to EEPROM if CRC is not correct
*/

/* Sensors to be supported:
 - BMP280
 - ACS712 (A0)
*/

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <EEPROM.h>
#include "configuration.h"
#include "datastructures.h"
#include "command_strings.h"
#include "action_strings.h"
#include "event_strings.h"
#include "schedule_strings.h"
#include "ESP8266_IOT_sensor.h"
#include <TimeLib.h>

bool inputs[PIN_NUMBER];
uint16_t outputs[PIN_NUMBER];
uint32_t InterruptCounter[PIN_NUMBER];

String deviceName = "";
uint8_t wifi_mode = WIFI_MODE_OFF;
uint32_t connectTimeLimit = 30000; //time for connection await
uint32_t reconnectPeriod = 600000; //time for sleep between connection retries
uint8_t APclientsConnected = 0;
uint8_t currentWiFiMode = 0;
uint8_t wifiIntendedStatus = RADIO_STOP;
uint32_t wifiReconnectLastTime = 0;
uint32_t ConnectionStarted = 0;
uint32_t WaitingStarted = 0;
bool wifiEnable = true;

uint32_t checkSensorLastTime = 0;
uint32_t checkSensorPeriod = 60000;

bool uartReady = false;
String uartCommand = "";
bool timeIsSet = false;

uint8_t autoReport = 0;

uint16_t SIGNAL_PINS = 0;
uint16_t INPUT_PINS = 0;
uint16_t OUTPUT_PINS = 0;
uint16_t INTERRUPT_PINS = 0;

// DATA BUSES

#ifdef I2C_ENABLE
#include <Wire.h>

TwoWire i2c;
#endif

#ifdef ONEWIRE_ENABLE
#include <OneWire.h>

OneWire oneWire(ONEWIRE_DATA);
#endif

// SENSORS

//AM2320 I2C temperature + humidity sensor
#ifdef AMS2320_ENABLE
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

Adafruit_AM2320 am2320;
bool am2320_enable;
#endif

// HTU21D I2C temperature + humidity sensor
#ifdef HTU21D_ENABLE
#include <Adafruit_HTU21DF.h>

Adafruit_HTU21DF htu21d;
bool htu21d_enable;
#endif

// BME280 I2C temperature + humidity + pressure sensor
#ifdef BME280_ENABLE
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme280;
bool bme280_enable;
#endif

// BMP180 I2C temperature + pressure sensor
#ifdef BMP180_ENABLE
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp180;
bool bmp180_enable;
#endif

// HTU21D I2C temperature + humidity sensor
#ifdef AHTx0_ENABLE
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 ahtx0;
bool ahtx0_enable;
#endif

// DS18B20 OneWire temperature sensor
#ifdef DS18B20_ENABLE
#include <DallasTemperature.h>

DallasTemperature ds1820(&oneWire);
bool ds1820_enable;
#endif

// DHT11/DHT21(AM2301)/DHT22(AM2302, AM2321) temperature + humidity sensor
#ifdef DHT_ENABLE
#include <DHT.h>

DHT dht(DHT_PIN, DHT_ENABLE);
bool dht_enable;
#endif

// MH-Z19 CO2 sensor via UART
#ifdef MH_Z19_UART_ENABLE
#include <SoftwareSerial.h>

SoftwareSerial uart2(SOFT_UART_TX, SOFT_UART_RX);

uint32_t mhz19UartSpeed = 9600;
uint16_t co2_uart_avg[3] = { 0, 0, 0 };
int mhtemp_s = 0;

int co2SerialRead()
{
	const int STATUS_NO_RESPONSE = -2;
	const int STATUS_CHECKSUM_MISMATCH = -3;
	const int STATUS_INCOMPLETE = -4;
	//const int STATUS_NOT_READY = -5;
	//const int STATUS_PWM_NOT_CONFIGURED = -6;
	//const int STATUS_SERIAL_NOT_CONFIGURED = -7;

	//unsigned long lastRequest = 0;
	uart2.begin(mhz19UartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
	uart2.flush();
	delay(50);

	byte cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
	byte response[9]; // for answer

	uart2.write(cmd, 9); // request PPM CO2
	//lastRequest = millis();

	// clear the buffer
	memset(response, 0, 9);

	int waited = 0;
	while (uart2.available() == 0)
	{
		delay(100); // wait a short moment to avoid false reading
		if (waited++ > 10)
		{
			uart2.flush();
			uart2.end();
			return STATUS_NO_RESPONSE;
		}
	}

	boolean skip = false;
	while (uart2.available() > 0 && (unsigned char)uart2.peek() != 0xFF)
	{
		if (!skip)
		{
#ifdef DEBUG_MODE
			Serial.print(F("MHZ: - skipping unexpected readings:"));
#endif
			skip = true;
		}
#ifdef DEBUG_MODE
		Serial.print(SPACE);
		Serial.print(uart2.peek(), HEX);
#endif
		uart2.read();
	}
	if (skip)
	{
#ifdef DEBUG_MODE
#endif
	}

	if (uart2.available() > 0)
	{
		int count = uart2.readBytes(response, 9);
		if (count < 9)
		{
			uart2.flush();
			uart2.end();
			return STATUS_INCOMPLETE;
		}
	}
	else
	{
		uart2.flush();
		uart2.end();
		return STATUS_INCOMPLETE;
	}

	// checksum
	byte check = getCheckSum(response);
	if (response[8] != check)
	{
		mhtemp_s = STATUS_CHECKSUM_MISMATCH;
		uart2.flush();
		uart2.end();
		return STATUS_CHECKSUM_MISMATCH;
	}
	uart2.flush();
	uart2.end();

	int ppm_uart = 256 * (int)response[2] + response[3];
	mhtemp_s = response[4] - 44; // - 40;
	//byte status = response[5];

	return ppm_uart;
}

byte getCheckSum(byte* packet)
{
	unsigned char checksum = 0;
	for (byte i = 1; i < 8; i++)
	{
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
#ifdef DEBUG_MODE
			Serial.println(F("Failed to read PPM"));
#endif
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
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

const uint8_t* font = fixed_bold10x15; // 10x15 pix
//const uint8_t* font = cp437font8x8;  // 8*8 pix
//const uint8_t* font = Adafruit5x7;  // 5*7 pix
SSD1306AsciiWire oled;
#endif
#endif

#ifdef ADC_ENABLE
uint16_t getAdc()
{
	uint16_t averageValue = analogRead(A0);
	delay(10);
	/*const uint8_t measurementsToAverage = 16;
	for (uint8_t i = 0; i < measurementsToAverage; ++i)
	{
		averageValue = uint16_t((analogRead(A0) + averageValue) / 2);
		//delay(1);
		yield();
	}
	return averageValue;*/
	return averageValue;
}
#endif

// SERVICES

// SLEEP
#ifdef SLEEP_ENABLE
uint32_t activeTimeOut_ms = uint32_t(2UL * connectTimeLimit);
uint32_t sleepTimeOut_us = uint32_t(20UL * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)
uint32_t activeStart;
bool sleepEnable = false;
#endif

// OTA UPDATE
#ifdef OTA_UPDATE
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool otaEnable = false;

/*
void otaStartCallback()
{
	String type;
	if (ArduinoOTA.getCommand() == U_FLASH)
	{
		Serial.println(F("Start updating sketch"));
	}
	else  // U_FS
	{
		// NOTE: if updating FS this would be the place to unmount FS using FS.end()
		Serial.println(F("Start updating filesystem"));
	}
}

void otaErrorCallback(ota_error_t error)
{
	//Serial.printf("Error[%u]: ", error);
	if (error == OTA_AUTH_ERROR) {
		Serial.println("Auth Failed");
	}
	else if (error == OTA_BEGIN_ERROR) {
		Serial.println("Begin Failed");
	}
	else if (error == OTA_CONNECT_ERROR) {
		Serial.println("Connect Failed");
	}
	else if (error == OTA_RECEIVE_ERROR) {
		Serial.println("Receive Failed");
	}
	else if (error == OTA_END_ERROR) {
		Serial.println("End Failed");
	}
}

void otaProgressCallback(unsigned int progress, unsigned int total)
{
	//Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void otaEndCallback()
{
	Serial.println(F("\nEnd"));
}
*/
#endif

// NTP
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>

WiFiUDP Udp;
int ntpRefreshDelay = 180;
uint32_t lastTimeRefersh = 0;
uint32_t timeRefershPeriod = 24 * 60 * 60 * 1000; //once a day
bool NTPenable = false;

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
	uint8_t packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011; // LI, Version, Mode
	packetBuffer[1] = 0;		  // Stratum, or type of clock
	packetBuffer[2] = 6;		  // Polling Interval
	packetBuffer[3] = 0xEC;		  // Peer Clock Precision
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
	if (!NTPenable || WiFi.status() != WL_CONNECTED)
		return 0;

	while (Udp.parsePacket() > 0)
		yield(); // discard any previously received packets

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
			uint8_t packetBuffer[NTP_PACKET_SIZE];	 //buffer to hold incoming & outgoing packets
			Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
			timeIsSet = true;
			// convert four bytes starting at location 40 to a long integer
			uint32_t secsSince1900 = (uint32_t)packetBuffer[40] << 24;
			secsSince1900 |= (uint32_t)packetBuffer[41] << 16;
			secsSince1900 |= (uint32_t)packetBuffer[42] << 8;
			secsSince1900 |= (uint32_t)packetBuffer[43];
			//NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			lastTimeRefersh = millis();
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
		yield();
	}
	return 0; // return 0 if unable to get the time
}

void restartNTP()
{
	ntpRefreshDelay = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt();
	if (readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size) == SWITCH_ON_NUMBER)
		NTPenable = true;

	if (NTPenable)
	{
		timeRefershPeriod = readConfigString(TELNET_PORT_addr, TELNET_PORT_size).toInt() * 1000;
		Udp.begin(UDP_LOCAL_PORT);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
	}
}

#endif

// EVENTS
#ifdef EVENTS_ENABLE
bool eventsFlags[EVENTS_NUMBER];
bool eventsEnable = false;

String getEvent(uint8_t n)
{
	uint16_t singleEventLength = EVENTS_TABLE_size / EVENTS_NUMBER;
	String eventStr = readConfigString((EVENTS_TABLE_addr + n * singleEventLength), singleEventLength);
	eventStr.trim();
	return eventStr;
}

void processEvent(uint8_t eventNum)
{
	if (eventsFlags[eventNum])
		return;

	String event = getEvent(eventNum);
	commandTokens cmd = parseCommand(event, ':', 1, false);
	event.clear();
	cmd.command.toLowerCase();

	// input?=0/1/c;
	if (cmd.command.startsWith(EVENT_INPUT))
	{
		if (INPUT_PINS > 0)
		{
			int t = cmd.command.indexOf('=');
			if (t >= 6 && t < cmd.command.length() - 1)
			{
				uint8_t outNum = cmd.command.substring(5, t).toInt();
				String outStateStr = cmd.command.substring(t + 1);
				bool outState = OFF;
				const String cTxt = F("c");
				if (outStateStr != cTxt)
				{
					if (outStateStr == SWITCH_ON)
						outState = ON;
					else if (outStateStr == SWITCH_OFF)
						outState = OFF;
					else
						outState = outStateStr.toInt();
				}
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (outStateStr == cTxt)
					{
						if (inputs[outNum - 1] != digitalRead(pins[outNum - 1]))
						{
							inputs[outNum - 1] = digitalRead(pins[outNum - 1]);
							ProcessAction(cmd.arguments[0], eventNum, true);
						}
					}
					else if (outState == digitalRead(pins[outNum - 1]))
						ProcessAction(cmd.arguments[0], eventNum, true);
				}
				else
				{
#ifdef DEBUG_MODE
					Serial.print(F("Incorrect input #"));
					Serial.println(String(outNum));
#endif
				}
			}
		}
	}
	// output?=0/1/c;
	else if (cmd.command.startsWith(EVENT_OUTPUT))
	{
		if (OUTPUT_PINS > 0)
		{
			int t = cmd.command.indexOf('=');
			if (t >= 7 && t < cmd.command.length() - 1)
			{
				uint8_t outNum = cmd.command.substring(6, t).toInt();
				String outStateStr = cmd.command.substring(t + 1);
				uint16_t outState = OFF;
				const String cTxt = F("c");
				if (outStateStr != cTxt)
				{
					if (outStateStr == SWITCH_ON)
						outState = ON;
					else if (outStateStr == SWITCH_OFF)
						outState = OFF;
					else
						outState = outStateStr.toInt();
				}
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (outStateStr == cTxt)
					{
						if (outputs[outNum - 1] != digitalRead(pins[outNum - 1]))
						{
							ProcessAction(cmd.arguments[0], eventNum, true);
						}
					}
					else if (outState == digitalRead(pins[outNum - 1]))
						ProcessAction(cmd.arguments[0], eventNum, true);
				}
				else
				{
#ifdef DEBUG_MODE
					Serial.print(F("Incorrect output #"));
					Serial.println(String(outNum));
#endif
				}
			}
		}
	}
	// counter?>x;
	else if (cmd.command.startsWith(EVENT_COUNTER))
	{
		if (INTERRUPT_PINS > 0)
		{
			int t = cmd.command.indexOf('>');
			if (t >= 8 && t < cmd.command.length() - 1)
			{
				uint8_t outNum = cmd.command.substring(7, t).toInt();
				String outStateStr = cmd.command.substring(t + 1);
				uint32_t outState = outStateStr.toInt();
				if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
				{
					if (InterruptCounter[outNum - 1] > outState)
						ProcessAction(cmd.arguments[0], eventNum, true);
				}
				else
				{
#ifdef DEBUG_MODE
					Serial.print(F("Incorrect output #"));
					Serial.println(String(outNum));
#endif
				}
			}
		}
	}

#ifdef ADC_ENABLE
	// adc<>x;
	else if (cmd.command.startsWith(EVENT_ADC) && cmd.command.length() > 4)
	{
		uint16_t adcValue = getAdc();
		char operation = cmd.command[3];
		uint16_t value = cmd.command.substring(4).toInt();
		if ((operation == '>' && adcValue > value) || (operation == '<' && adcValue < value))
		{
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#ifdef TEMPERATURE_SENSOR
	// temperature<>x;
	else if (cmd.command.startsWith(EVENT_TEMPERATURE) && cmd.command.length() > 12)
	{
		char oreration = cmd.command[11];
		int value = cmd.command.substring(12).toInt();
		sensorDataCollection sensorData = collectData();
		int temperatureNow = (int)getTemperature(sensorData);
		if (oreration == '>' && temperatureNow > value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
		}
		else if (oreration == '<' && temperatureNow < value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
		}
	}
#endif

#ifdef HUMIDITY_SENSOR
	// humidity<>x;
	else if (cmd.command.startsWith(EVENT_HUMIDITY) && cmd.command.length() > 9)
	{
		char oreration = cmd.command[8];
		int value = cmd.command.substring(9).toInt();
		sensorDataCollection sensorData = collectData();
		int humidityNow = (int)getTemperature(sensorData);
		if (oreration == '>' && humidityNow > value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
		}
		else if (oreration == '<' && humidityNow < value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
		}
	}
#endif

#ifdef CO2_SENSOR
	// co2<>x;
	else if (cmd.command.startsWith(EVENT_CO2) && cmd.command.length() > 4)
	{
		char oreration = cmd.command[3];
		int value = cmd.command.substring(4).toInt();
		sensorDataCollection sensorData = collectData();
		int co2Now = getTemperature(sensorData);
		if (oreration == '>' && co2Now > value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
		}
		else if (oreration == '<' && co2Now < value)
		{
			ProcessAction(cmd.arguments[0], eventNum, true);
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

// SCHEDULES
#ifdef SCHEDULER_ENABLE
bool schedulerEnable = false;

String getSchedule(uint8_t n)
{
	uint16_t singleScheduleLength = SCHEDULER_TABLE_size / SCHEDULES_NUMBER;
	String scheduleStr = readConfigString(SCHEDULER_TABLE_addr + n * singleScheduleLength, singleScheduleLength);
	scheduleStr.trim();
	return scheduleStr;
}

void writeScheduleExecTime(uint8_t scheduleNum, uint32_t execTime)
{
	uint16_t singleScheduleLength = SCHEDULER_LASTRUN_TABLE_size / SCHEDULES_NUMBER;
	writeConfigLong(SCHEDULER_LASTRUN_TABLE_addr + scheduleNum * singleScheduleLength, execTime);
}

uint32_t readScheduleExecTime(uint8_t scheduleNum)
{
	uint16_t singleScheduleLength = SCHEDULER_LASTRUN_TABLE_size / SCHEDULES_NUMBER;
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
	String schedule = getSchedule(scheduleNum);

	if (schedule.length() == 0)
		return;
	int t = schedule.indexOf('@');
	int t1 = schedule.indexOf(';');

	if (t <= 0 || t1 <= 0 || t1 < t)
		return;

	String condition = schedule.substring(0, t + 1);
	condition.trim();
	condition.toLowerCase();

	String time = schedule.substring(t + 1, t1);
	time.trim();
	time.toLowerCase();
	schedule = schedule.substring(t1 + 1);

	uint32_t execTime = readScheduleExecTime(scheduleNum);

	//	start daily - daily@hh:mm;action1;action2;...
	if (condition.startsWith(SHEDULE_DAILY) && time.length() == 5)
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
			if (readScheduleExecTime(scheduleNum) < plannedTime && hour() >= _hr && minute() >= _min)
				ProcessAction(schedule, scheduleNum, false);
		}
	}
	//	start weekly - weekly@week_day hh:mm;action1;action2;...
	else if (condition.startsWith(SHEDULE_WEEKLY) && time.length() == 7)
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
			if (readScheduleExecTime(scheduleNum) < plannedTime && weekday() == _weekDay && hour() >= _hr && minute() >= _min)
				ProcessAction(schedule, scheduleNum, false);
		}
	}
	//	monthly - monthly@month_day hh:mm;action1;action2;...
	else if (condition.startsWith(SHEDULE_MONTHLY) && time.length() == 8)
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
			if (readScheduleExecTime(scheduleNum) < plannedTime && day() >= _day && hour() >= _hr && minute() >= _min)
				ProcessAction(schedule, scheduleNum, false);
		}
	}
	//	start date - once@yyyy.mm.dd hh:mm;action1;action2;...
	else if (condition.startsWith(SHEDULE_ONCE) && time.length() == 16 && execTime == 0)
	{
		uint8_t _yr = time.substring(0, 4).toInt();
		uint8_t _month = time.substring(5, 7).toInt();
		uint8_t _day = time.substring(8, 10).toInt();
		uint8_t _hr = time.substring(11, 13).toInt();
		uint8_t _min = time.substring(14, 16).toInt();

		if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60 && _day >= 1 && _day <= 31 && _month >= 1 && _month <= 12)
		{
			if (year() >= _yr && month() >= _month && day() >= _day && hour() >= _hr && minute() >= _min)
				ProcessAction(schedule, scheduleNum, false);
		}
	}
}
#endif

// TELNET
#ifdef TELNET_ENABLE
WiFiServer telnetServer(23);
WiFiClient serverClient[TELNET_ENABLE];
bool telnetEnable = true;

void sendToTelnet(String& str, uint8_t clientN)
{
	if (serverClient[clientN] && serverClient[clientN].connected())
	{
		serverClient[clientN].print(str);
		delay(1);
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
	if (addressTo.length() <= 0)
		addressTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);

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
	if (readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size) == SWITCH_ON_NUMBER)
		mqttClean = true;
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
			mqtt_device_id += F("_");
			mqtt_device_id += CompactMac();
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

bool mqtt_send(String& message, int dataLength, String& topic)
{
	if (!mqtt_client.connected())
	{
		mqtt_connect();
	}

	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		if (topic.length() <= 0)
			topic = getMqttTopicOut();

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

// TELEGRAM
#ifdef TELEGRAM_ENABLE
#include <ArduinoJson.h>
#include <CTBot.h>

CTBot myBot;
telegramMessage telegramOutboundBuffer[TELEGRAM_MESSAGE_BUFFER_SIZE];
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
		if (c < 10)
			c += '0';
		else
			c += 'A' - 10;
		result = c + result;
		yield();
	} while (input);
	return result;
}

uint64_t StringToUint64(String& value)
{
	for (uint8_t i = 0; i < value.length(); i++)
	{
		if (value[i] < '0' || value[i] > '9')
			return 0;
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

bool sendToTelegramServer(int64_t user, String& message)
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
	if (telegramOutboundBufferPos >= TELEGRAM_MESSAGE_BUFFER_SIZE)
	{
		for (uint8_t i = 0; i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
			yield();
		}
		telegramOutboundBufferPos = TELEGRAM_MESSAGE_BUFFER_SIZE - 1;
	}
	telegramOutboundBuffer[telegramOutboundBufferPos].user = user;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = message;
	telegramOutboundBuffer[telegramOutboundBufferPos].retries = retries;
	telegramOutboundBufferPos++;
}

void removeMessageFromTelegramOutboundBuffer()
{
	if (telegramOutboundBufferPos <= 0)
		return;
	for (uint8_t i = 0; i < telegramOutboundBufferPos; i++)
	{
		if (i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1)
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		else
		{
			telegramOutboundBuffer[i].message.clear();
			telegramOutboundBuffer[i].user = 0;
		}
		yield();
	}
	telegramOutboundBufferPos--;
}

void sendToTelegram(int64_t user, String& message, uint8_t retries)
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
			message.clear();
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
			yield();
			//consider 1st sender an admin if admin is empty
			if (getTelegramUser(0) != 0)
			{
				//check if it's registered user
				for (uint8_t i = 0; i < TELEGRAM_USERS_NUMBER; i++)
				{
					yield();
					if (getTelegramUser(i) == msg.sender.id)
					{
						isRegisteredUser = true;
						if (i == 0)
							isAdmin = true;
						break;
					}
				}
			}
			yield();
			//process data from TELEGRAM
			if (isRegisteredUser && msg.text.length() > 0)
			{
				//sendToTelegram(msg.sender.id, deviceName + ": " + msg.text, telegramRetries);
				String tmpCmd = msg.text;
				String str = processCommand(tmpCmd, CHANNEL_TELEGRAM, isAdmin);
				String message = deviceName;
				message += F(": ");
				message += str;
				sendToTelegram(msg.sender.id, message, TELEGRAM_RETRIES);
			}
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
	uint16_t singleUserLength = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
	String userStr = readConfigString((TELEGRAM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return StringToUint64(userStr);
}

#endif

// GSM
#ifdef GSM_ENABLE
#include <SoftwareSerial.h>

uint32_t gsmTimeOut = 0;
uint32_t gsmUartSpeed = 19200;
bool gsmEnable = false;

SoftwareSerial uart2(SOFT_UART_TX, SOFT_UART_RX);

String getGsmUser(uint8_t n)
{
	const uint8_t singleUserLength = GSM_USERS_TABLE_size / GSM_USERS_NUMBER;
	String userStr = readConfigString((GSM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}

String sendATCommand(String cmd, bool waiting)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	uart2.flush();
	String _resp = "";
	uart2.println(cmd);
#ifdef DEBUG_MODE
	Serial.print(F(">>"));
	Serial.println(cmd);
#endif
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
#ifdef DEBUG_MODE
		Serial.println("<<" + _resp);
#endif
		if (_resp.length() <= 0)
		{
#ifdef DEBUG_MODE
			Serial.println("Timeout...");
#endif
		}

		if (stopSerial)
			uart2.end();
		return _resp;
	}
	return _resp;
}

bool initModem()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	bool resp = true;
	String response = sendATCommand(AT_COMMAND_INIT1, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT2, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT3, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT4, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	if (stopSerial)
		uart2.end();

	return resp;
}

bool sendSMS(String& phone, String& smsText)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	String cmdTmp;
	cmdTmp.reserve(175);
	while (smsText.length() > 0)
	{
		yield();
		initModem();
		cmdTmp = AT_COMMAND_SEND;
		cmdTmp.reserve(175);
		cmdTmp += phone;
		cmdTmp += F("\"");
		sendATCommand(cmdTmp, true);
		cmdTmp.clear();
		if (smsText.length() > 150)
		{
			cmdTmp += smsText.substring(0, 150);
			smsText = smsText.substring(151, smsText.length());
		}
		else
		{
			cmdTmp += smsText;
			smsText.clear();
		}

#ifdef DEBUG_MODE
		Serial.print(F("Sending SMS to "));
		Serial.print(phone);
		Serial.print(F(":\'"));
		Serial.print(cmdTmp);
		Serial.println(F("\'"));
		Serial.println(String(cmdTmp.length()));
#endif
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
		cmdTmp.clear();
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
#ifdef DEBUG_MODE
			//Serial.println("Timeout...");
#endif
		}

		if (cmdTmp.lastIndexOf(F("OK")) > 0)
		{
#ifdef DEBUG_MODE
			//Serial.println(F("SMS sent..."));
#endif
		}
		else
		{
#ifdef DEBUG_MODE
			//Serial.println(F("SMS not sent..."));
#endif
		}
	}

	if (stopSerial)
		uart2.end();

	if (cmdTmp.lastIndexOf(F("OK")) > 0)
		return true;
	return false;
}

smsMessage parseSMS(String& msg)
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
	if (msgphone.length() <= 0)
		msgphone = SPACE;

	smsMessage response;
	response.PhoneNumber = msgphone;
	for (uint8_t i = 0; i < GSM_USERS_NUMBER; i++)
	{
		String gsmUser = getGsmUser(i);
		if (msgphone == gsmUser)
		{
			if (i == 0)
				response.IsAdmin = true;
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
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
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
#ifdef DEBUG_MODE
	//Serial.println("Sms: " + sms.Message);
	//Serial.println("From: " + sms.PhoneNumber);
#endif
	if (stopSerial)
		uart2.end();

	return sms;
}

bool deleteSMS(int n)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	const String cmd = String(AT_COMMAND_DELETE_SMS) + String(n);
	const String response = sendATCommand(cmd, true);
	if (stopSerial)
		uart2.end();
	if (response.lastIndexOf(F("OK")) > 0)
		return true;
	else
		return false;
}

String getModemStatus()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(gsmUartSpeed, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	String response = sendATCommand(AT_COMMAND_STATUS1, true);
	response += sendATCommand(AT_COMMAND_STATUS2, true);
	response += sendATCommand(AT_COMMAND_STATUS3, true);
	response += sendATCommand(AT_COMMAND_STATUS4, true);

	if (stopSerial)
		uart2.end();

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
		const String host = F("script.google.com");
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
			int retval = gScriptClient->connect(host.c_str(), httpsPort);
			if (retval == 1)
			{
				flag = true;
				break;
			}
#ifdef DEBUG_MODE
			//else Serial.println(F("Connection failed. Retrying..."));
#endif
			yield();
		}
		if (flag)
		{
			String urlFinal = F("/macros/s/");
			urlFinal += gScriptId;
			urlFinal += F("/exec");
			urlFinal += F("?value=");
			urlFinal += value;
			flag = gScriptClient->GET(urlFinal, host.c_str(), true);
			if (flag)
			{
				gScriptClient->stopAll();
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

bool pushingBoxEnable = false;

bool sendToPushingBoxServer(String message)
{
	bool flag = false;
	if (pushingBoxEnable && WiFi.status() == WL_CONNECTED)
	{
		WiFiClient client;

		const String pushingBoxServer = String(F("api.pushingbox.com"));
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
			client.print(pushingBoxServer);
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
	const uint16_t pushingBoxMessageMaxSize = 1000;
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
			message.clear();
		}
		yield();
	}
	return result;
}
#endif

// HTTP server
#ifdef HTTP_ENABLE
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
	str += parseSensorReport(sensorData, F("<br>"), false);
	str += F("<br />\r\n</html>\r\n");
	http_server.sendContent(str);
}

/*void handleRoot()
{
	sensorDataCollection sensorData = collectData();
	String str = F("Connection: close\r\nRefresh: ");
	str += String(uint16_t(checkSensorPeriod / 1000UL));
	str += F("\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
	str += parseSensorReport(sensorData, F("<br>"), false);
	str += F("<br />\r\n</html>\r\n");
	http_server.send(200, F("text/html"), str);
}*/

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
		message += SPACE;
		message += http_server.argName(i);
		message += F(": ");
		message += http_server.arg(i);
		message += EOL;
		yield();
	}
	http_server.send(404, F("text/plain"), message);
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
	deviceName.reserve(DEVICE_NAME_size + 1);

#ifdef HARD_UART_ENABLE
	if (!add_signal_pin(HARD_UART_TX))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting HardUART TX pin to "));
		Serial.println(String(HARD_UART_TX));
#endif
	}
	if (!add_signal_pin(HARD_UART_RX))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting HardUART RX pin to "));
		Serial.println(String(HARD_UART_RX));
#endif
	}
	uartCommand.reserve(100);
	Serial.begin(115200);
#endif
	yield();

#ifdef MQTT_ENABLE
	mqttCommand.reserve(256);
#endif
	yield();
#ifdef TELEGRAM_ENABLE
	for (uint8_t b = 0; b < TELEGRAM_MESSAGE_BUFFER_SIZE; b++)
	{
		telegramOutboundBuffer[0].message.reserve(TELEGRAM_MESSAGE_MAX_SIZE);
		yield();
	}
#endif
	yield();
	//mark signal pins to avoid using them as discrete I/O
#ifdef I2C_ENABLE
	if (!add_signal_pin(PIN_WIRE_SDA))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting I2C SDA pin to "));
		Serial.println(String(PIN_WIRE_SDA));
#endif
	}
	if (!add_signal_pin(PIN_WIRE_SCL))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting I2C SCL pin to "));
		Serial.println(String(PIN_WIRE_SCL));
#endif
	}

	Wire.begin(PIN_WIRE_SDA, PIN_WIRE_SCL);
#endif
	yield();
#ifdef SOFT_UART_ENABLE
	if (!add_signal_pin(SOFT_UART_TX))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting SoftUART TX pin to "));
		Serial.println(String(SOFT_UART_TX));
#endif
	}
	if (!add_signal_pin(SOFT_UART_RX))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting SoftUART RX pin to "));
		Serial.println(String(SOFT_UART_RX));
#endif
	}
#endif
	yield();
#ifdef ONEWIRE_ENABLE
	if (!add_signal_pin(ONEWIRE_DATA))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting ONEWIRE pin to "));
		Serial.println(String(ONEWIRE_DATA));
#endif
	}
#endif
	yield();
#ifdef DHT_ENABLE
	if (!add_signal_pin(DHT_PIN))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting DHT pin to "));
		Serial.println(String(DHT_PIN));
#endif
	}
#endif
	yield();
#ifdef TM1637DISPLAY_ENABLE
	if (!add_signal_pin(TM1637_CLK))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting TM1637 CLK pin to "));
		Serial.println(String(TM1637_CLK));
#endif
	}
	if (!add_signal_pin(TM1637_DIO))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting TM1637 DIO pin to "));
		Serial.println(String(TM1637_DIO));
#endif
	}
#endif
	yield();
#ifdef MH_Z19_PPM_ENABLE
	if (!add_signal_pin(MH_Z19_PPM_ENABLE))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting MH-Z19 PPM pin to "));
		Serial.println(String(MH_Z19_PPM_ENABLE));
#endif
	}
#endif
	yield();
#ifdef BUZZER_ENABLE
	if (!add_signal_pin(BUZZER_ENABLE))
	{
#ifdef DEBUG_MODE
		Serial.println(F("Error setting BUZZER pin to "));
		Serial.println(String(BUZZER_ENABLE));
#endif
	}

#endif
	yield();
	EEPROM.begin(collectEepromSize());
	//clear EEPROM if it's just initialized
	if (!checkEepromCrc())
	{
		clearEeprom();
	}
	yield();
	connectTimeLimit = uint32_t(readConfigString(CONNECT_TIME_addr, CONNECT_TIME_size).toInt() * 1000UL);
	if (connectTimeLimit == 0)
		connectTimeLimit = 60000UL;
	reconnectPeriod = uint32_t(readConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size).toInt() * 1000UL);
	if (reconnectPeriod == 0)
		reconnectPeriod = 600000UL;

	deviceName = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);

	checkSensorPeriod = readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt() * 1000UL;
	autoReport = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();
	yield();
#ifdef LOG_ENABLE
	logPeriod = readConfigString(LOG_PERIOD_addr, LOG_PERIOD_size).toInt() * 1000UL;
#endif
	yield();
	// init WiFi
	startWiFi();
	yield();
#ifdef TELNET_ENABLE
	uint16_t telnetPort = readConfigString(TELNET_PORT_addr, TELNET_PORT_size).toInt();
	telnetServer.begin(telnetPort);
	telnetServer.setNoDelay(true);
#endif
	yield();
#ifdef EVENTS_ENABLE
	if (readConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size) == SWITCH_ON_NUMBER)
		eventsEnable = true;
#endif
	yield();
#ifdef SCHEDULER_ENABLE
	if (readConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size) == SWITCH_ON_NUMBER)
		schedulerEnable = true;
#endif
	yield();
	String tmpStr;
	tmpStr.reserve(101);
#ifdef OTA_UPDATE
	if (readConfigString(OTA_ENABLE_addr, OTA_ENABLE_size) == SWITCH_ON_NUMBER)
		otaEnable = true;

	uint16_t otaPort = readConfigString(OTA_PORT_addr, OTA_PORT_size).toInt();
	ArduinoOTA.setPort(otaPort);

	tmpStr = readConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size);
	ArduinoOTA.setPassword(tmpStr.c_str());
	tmpStr = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	tmpStr += F("_");
	tmpStr += CompactMac();
	ArduinoOTA.setHostname(tmpStr.c_str());

	//ArduinoOTA.onStart(otaStartCallback);
	//ArduinoOTA.onEnd(otaEndCallback);
	//ArduinoOTA.onProgress(otaProgressCallback);
	//ArduinoOTA.onError(otaErrorCallback);

	if (otaEnable) ArduinoOTA.begin();
#endif
	yield();
#ifdef MQTT_ENABLE
	if (readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size) == SWITCH_ON_NUMBER)
		mqttEnable = true;
#endif
	yield();
#ifdef SMTP_ENABLE
	if (readConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size) == SWITCH_ON_NUMBER)
		smtpEnable = true;
#endif
	yield();
#ifdef GSM_ENABLE
	if (readConfigString(GSM_ENABLE_addr, GSM_ENABLE_size) == SWITCH_ON_NUMBER)
		gsmEnable = true;
	if (gsmEnable)
	{
		initModem();
	}
#endif
	yield();
#ifdef TELEGRAM_ENABLE
	String telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	if (readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size) == SWITCH_ON_NUMBER)
		telegramEnable = true;
	if (telegramEnable)
	{
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif
	yield();
#ifdef GSCRIPT_ENABLE
	if (readConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size) == SWITCH_ON_NUMBER)
		gScriptEnable = true;
#endif
	yield();
#ifdef PUSHINGBOX_ENABLE
	if (readConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size) == SWITCH_ON_NUMBER)
		pushingBoxEnable = true;
#endif
	yield();
#ifdef HTTP_ENABLE
	httpPort = readConfigString(HTTP_PORT_addr, HTTP_PORT_size).toInt();

	if (readConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size) == SWITCH_ON_NUMBER)
		httpServerEnable = true;

	http_server.on(F("/"), handleRoot);
	/*http_server.on(F("/inline"), []()
		{
		http_server.send(200, F("text/plain"), F("this works as well"));
		});*/
		//http_server.onNotFound(handleNotFound);

	if (httpServerEnable) http_server.begin(httpPort);
#endif
	yield();
#ifdef NTP_TIME_ENABLE
	restartNTP();
#endif
	yield();
#ifdef MH_Z19_UART_ENABLE
	co2_uart_avg[0] = co2_uart_avg[1] = co2_uart_avg[2] = co2SerialRead();
#endif
	yield();
#ifdef MH_Z19_PPM_ENABLE
	pinMode(MH_Z19_PPM_ENABLE, INPUT);
	co2_ppm_avg[0] = co2_ppm_avg[1] = co2_ppm_avg[2] = co2PPMRead();
#endif
	yield();
#ifdef DISPLAY_ENABLED
	displaySwitchPeriod = uint32_t(readConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size).toInt() * 1000UL);

#ifdef TM1637DISPLAY_ENABLE
	display.setBrightness(1);
	display.showNumberDec(0, false);
#endif

#endif
	yield();
	//setting pin modes
	for (uint8_t num = 0; num < PIN_NUMBER; num++)
	{
#ifdef DEBUG_MODE
		Serial.print(F("Setting pin: "));
		Serial.println(String(num));
#endif

		if (bitRead(SIGNAL_PINS, num))
			continue; //pin reserved to data bus

		uint16_t l = PIN_MODE_size / PIN_NUMBER;
		uint8_t pinModeCfg = readConfigString(PIN_MODE_addr + num * l, l).toInt();

#ifdef DEBUG_MODE
		Serial.println(F("mode: "));
		Serial.println(String(pinModeCfg));
#endif

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
				if (num == 0)
					attachInterrupt(pins[num], int1count, intModeCfg);
				else if (num == 1)
					attachInterrupt(pins[num], int2count, intModeCfg);
				else if (num == 2)
					attachInterrupt(pins[num], int3count, intModeCfg);
				else if (num == 3)
					attachInterrupt(pins[num], int4count, intModeCfg);
				else if (num == 4)
					attachInterrupt(pins[num], int5count, intModeCfg);
				else if (num == 5)
					attachInterrupt(pins[num], int6count, intModeCfg);
				else if (num == 6)
					attachInterrupt(pins[num], int7count, intModeCfg);
				else if (num == 7)
					attachInterrupt(pins[num], int8count, intModeCfg);
			}
		}
		else if (pinModeCfg == OUTPUT) // pin is output
		{
			bitSet(OUTPUT_PINS, num);
			pinMode(pins[num], pinModeCfg);
			l = OUTPUT_INIT_size / PIN_NUMBER;
			tmpStr = readConfigString(OUTPUT_INIT_addr + num * l, l);
			set_output(num + 1, tmpStr);
			outputs[num] = digitalRead(pins[num]);
		}

		yield();
	}

#ifdef SLEEP_ENABLE
	if (readConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size) == SWITCH_ON_NUMBER)
		sleepEnable = true;

	activeTimeOut_ms = uint32_t(readConfigString(SLEEP_ON_addr, SLEEP_ON_size).toInt() * 1000UL);
	if (activeTimeOut_ms == 0)
		activeTimeOut_ms = uint32_t(2 * connectTimeLimit);

	sleepTimeOut_us = uint32_t(readConfigString(SLEEP_OFF_addr, SLEEP_OFF_size).toInt() * 1000000UL);
	if (sleepTimeOut_us == 0)
		sleepTimeOut_us = uint32_t(20 * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)

	if (sleepEnable)
		activeStart = millis();
#endif
	yield();
#ifdef DEBUG_MODE
	Serial.println(F("Setup end"));
#endif
	yield();
}

void loop()
{

#ifdef NTP_TIME_ENABLE
	//refersh NTP time if time has come
	/*if (NTPenable && millis() - lastTimeRefersh > timeRefershPeriod)
	{
		restartNTP();
	}*/
#endif
	//check if connection is present and reconnect
	if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_STA || wifi_mode == WIFI_MODE_AUTO)
	{
		//check if connection is lost
		if (wifiIntendedStatus == RADIO_CONNECTED && WiFi.status() != WL_CONNECTED)
		{
			wifiIntendedStatus = RADIO_CONNECTING;
			ConnectionStarted = millis();
		}

		//check if connection is established
		if (wifiIntendedStatus == RADIO_CONNECTING && WiFi.status() == WL_CONNECTED)
		{
#ifdef DEBUG_MODE
			Serial.print(F("STA connected to \""));
			Serial.print(getStaSsid());
			Serial.println(F("\" AP."));
			Serial.print(F("IP: "));
			Serial.println(WiFi.localIP());
#endif
			wifiIntendedStatus = RADIO_CONNECTED;
		}
	}
	yield();
	//check if connecting is too long and switch AP/STA
	if (wifi_mode == WIFI_MODE_AUTO)
	{
		//check connect to AP time limit and switch to AP if AUTO mode
		if (wifiIntendedStatus == RADIO_CONNECTING && ConnectionStarted + connectTimeLimit < millis())
		{
			Start_AP_Mode();
			wifiIntendedStatus = RADIO_WAITING;
			WaitingStarted = millis();
		}

		//check if waiting time finished and no clients connected to AP
		if (wifiIntendedStatus == RADIO_WAITING && WaitingStarted + reconnectPeriod < millis() && WiFi.softAPgetStationNum() <= 0)
		{
			Start_STA_Mode();
			wifiIntendedStatus = RADIO_CONNECTING;
			ConnectionStarted = millis();
		}
	}
	yield();
	//check if clients connected/disconnected to AP
	if ((wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA) && APclientsConnected != WiFi.softAPgetStationNum())
	{
		if (APclientsConnected < WiFi.softAPgetStationNum())
		{
#ifdef DEBUG_MODE
			Serial.println(F("AP client connected: "));
#endif
		}
		else
		{
#ifdef DEBUG_MODE
			Serial.println(F("AP Client disconnected: "));
#endif
		}
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
				String str = parseSensorReport(sensorData, EOL, false);
#ifdef DEBUG_MODE
				Serial.println(str);
#endif
			}
#ifdef TELNET_ENABLE
			if ((WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) && telnetEnable && bitRead(autoReport, CHANNEL_TELNET))
			{
				String str = parseSensorReport(sensorData, EOL, false);
				for (uint8_t i = 0; i < TELNET_ENABLE; i++)
				{
					if (serverClient[i] && serverClient[i].connected())
					{
						sendToTelnet(str, i);
					}
					yield();
				}
			}
#endif
			if (WiFi.status() == WL_CONNECTED)
			{
#ifdef MQTT_ENABLE
				if (mqttEnable && bitRead(autoReport, CHANNEL_MQTT))
				{
					String str = parseSensorReport(sensorData, F(","), true);
					String topic = "";
					topic.reserve(100);
					mqtt_send(str, str.length(), topic);
				}
#endif

#ifdef TELEGRAM_ENABLE
				if (telegramEnable && bitRead(autoReport, CHANNEL_TELEGRAM))
				{
					String str = parseSensorReport(sensorData, EOL, false);
					for (uint8_t i = 0; i < TELEGRAM_USERS_NUMBER; i++)
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
					String str = parseSensorReport(sensorData, EOL, false);
					sendValueToGoogle(str);
				}
#endif

#ifdef PUSHINGBOX_ENABLE
				if (pushingBoxEnable && bitRead(autoReport, CHANNEL_PUSHINGBOX))
				{
					String str = parseSensorReport(sensorData, EOL, false);
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
				String tmpStr = parseSensorReport(history_log[n], CSV_DIVIDER, false);
				tmpStr += EOL;
				n++;
				while (n < history_record_number)
				{
					uint32_t freeMem = ESP.getFreeHeap() / 3;
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number)
							break;
						tmpStr += parseSensorReport(history_log[n], CSV_DIVIDER, false);
						tmpStr += EOL;
						yield();
					}
					String subj = deviceName;
					subj += F(" log");
					String addrTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);
					sendOk = sendMail(subj, tmpStr, addrTo);
					tmpStr.clear();
					yield();
					if (sendOk)
					{
						//history_record_number = 0;
					}
					else
					{
#ifdef DEBUG_MODE
						Serial.print(F("Error sending log to E-mail "));
						Serial.print(String(history_record_number));
						Serial.println(F(" records."));
#endif
					}
					yield();
				}
			}
		}
#endif
	}
#endif
	yield();
#ifdef HARD_UART_ENABLE
	//check UART for data
	while (Serial.available() && !uartReady)
	{
		char c = Serial.read();
		if (c == '\r' || c == '\n')
			uartReady = true;
		else
			uartCommand += (char)c;
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
		uartCommand.clear();
	}
#endif
	yield();
	//process data from HTTP/Telnet/MQTT/TELEGRAM if available
	if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0)
	{
#ifdef OTA_UPDATE
		if (otaEnable) ArduinoOTA.handle();
#endif
		yield();
#ifdef HTTP_ENABLE
		if (httpServerEnable) http_server.handleClient();
#endif
		yield();
#ifdef TELNET_ENABLE
		if (telnetEnable)
		{
			//check if there are any new clients
			if (telnetServer.hasClient())
			{
				yield();
				uint8_t i;
				//find free/disconnected spot
				for (i = 0; i < TELNET_ENABLE; i++)
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
				}
				//no free/disconnected spot so reject
				if (i >= TELNET_ENABLE)
				{
					WiFiClient serverClient = telnetServer.available();
					serverClient.stop();
#ifdef DEBUG_MODE
					Serial.print(F("No more slots. Client rejected."));
#endif
				}
			}

			//check network clients for incoming data and process commands
			for (uint8_t i = 0; i < TELNET_ENABLE; i++)
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
									telnetCommand.clear();
									yield();
									sendToTelnet(str, i);
								}
							}
							else
								telnetCommand += (char)c;
							yield();
						}
					}
				}
			}
		}
#endif
		yield();
#ifdef MQTT_ENABLE
		if (mqttEnable)
		{
			if (!mqtt_client.connected())
				mqtt_connect();
			else
				mqtt_client.loop();
			if (mqttCommand.length() > 0)
			{
				String str = processCommand(mqttCommand, CHANNEL_MQTT, true);
				mqttCommand.clear();
				String topic = "";
				topic.reserve(100);
				mqtt_send(str, str.length(), topic);
			}
			yield();
		}
#endif
		yield();
#ifdef TELEGRAM_ENABLE
		//check TELEGRAM for data
		if (telegramEnable && currentWiFiMode != WIFI_MODE_AP && millis() - telegramLastTime > TELEGRAM_MESSAGE_DELAY)
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
		if (newSms.Message.length() > 0)
		{
#ifdef DEBUG_MODE
			Serial.print(F("New SMS from "));
			Serial.print(newSms.PhoneNumber);
			Serial.print(F(":\""));
			Serial.print(newSms.Message);
			Serial.println(F("\""));
#endif
			//process data from GSM
			String str = processCommand(newSms.Message, CHANNEL_GSM, newSms.IsAdmin);
			str.trim();
			if (str.startsWith(F("^")))
			{
				bool f = sendSMS(newSms.PhoneNumber, str);
				if (!f)
				{
#ifdef DEBUG_MODE
					Serial.println(F("SMS not sent"));
#endif
				}
			}
		}
	}
#endif
	yield();
#ifdef EVENTS_ENABLE
	if (eventsEnable)
	{
		for (uint8_t i = 0; i < EVENTS_NUMBER; i++)
		{
			processEvent(i);
			yield();
		}
	}
#endif
	yield();
#ifdef SCHEDULER_ENABLE
	if (schedulerEnable && timeIsSet)
	{
		for (uint8_t i = 0; i < SCHEDULES_NUMBER; i++)
		{
			processSchedule(i);
			yield();
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
				if (co2_avg > -1000)
					display.showNumberDec(co2_avg, false);
				else
					displayState++;
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
				else
					displayState++;
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
				else
					displayState++;
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
				else
					displayState++;
			}
			displayState++;
			if (displayState > 3)
				displayState = 0;
#endif

#ifdef SSD1306DISPLAY_ENABLE
			i2c.setClock(400000L);
			oled.begin(&Adafruit128x64, SSD1306DISPLAY_ENABLE);
			oled.setFont(font);

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
				if (_hr < 10)
					tmpStr += F("0");
				tmpStr += String(_hr);
				tmpStr += COLON;
				if (_min < 10)
					tmpStr += F("0");
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
			if (n < 3)
				oled.set2X();
			else
				oled.set1X();
			oled.print(tmpStr);
#endif
		}
	}
#endif
	yield();
#ifdef SLEEP_ENABLE
	if (sleepEnable && millis() > uint32_t(activeStart + activeTimeOut_ms))
	{
		ESP.deepSleep(sleepTimeOut_us);
	}
#endif
}

uint32_t collectEepromSize()
{
	return FINAL_CRC_addr + FINAL_CRC_size + 100; // add 100 bytes as a reserve
}

String readConfigString(uint16_t startAt, uint16_t maxlen)
{
	String str = "";
	str.reserve(maxlen);
	for (uint16_t i = 0; i < maxlen; i++)
	{
		char c = (char)EEPROM.read(startAt + i);
		if (c == 0)
			i = maxlen;
		else
			str += c;
		yield();
	}

	return str;
}

void readConfigString(uint16_t startAt, uint16_t maxlen, char* array)
{
	for (uint16_t i = 0; i < maxlen; i++)
	{
		array[i] = (char)EEPROM.read(startAt + i);
		if (array[i] == 0)
			i = maxlen;
		yield();
	}
}

uint32_t readConfigLong(uint16_t startAt)
{
	union Convert
	{
		uint32_t number = 0;
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
		float number = 0;
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
		if (i < str.length())
			EEPROM.write(startAt + i, (uint8_t)str[i]);
		else
			EEPROM.write(startAt + i, 0);
		yield();
	}
	refreshEepromCrc();
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
	refreshEepromCrc();
	EEPROM.commit();
}

void writeConfigLong(uint16_t startAt, uint32_t data)
{
	union Convert
	{
		uint32_t number = 0;
		uint8_t byteNum[4];
	} p;
	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	refreshEepromCrc();
	EEPROM.commit();
}

void writeConfigFloat(uint16_t startAt, float data)
{
	union Convert
	{
		float number = 0;
		uint8_t byteNum[4];
	} p;

	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	refreshEepromCrc();
	EEPROM.commit();
}

uint16_t calculateEepromCrc()
{
	uint16_t crc = 0xffff;
	for (int i = 0; i < FINAL_CRC_addr + FINAL_CRC_size; i++)
	{
		uint8_t b = EEPROM.read(i);
		crc ^= b;
		for (int x = 0; x < 8; x++)
		{
			if (crc & 0x0001)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
			{
				crc >>= 1;
			}
			yield();
		}
		yield();
	}

	return crc;
}

bool checkEepromCrc()
{
	union Convert
	{
		uint32_t number = 0;
		uint8_t byteNum[4];
	} savedCrc;

	for (uint8_t i = 0; i < 2; i++)
	{
		savedCrc.byteNum[i] = EEPROM.read(FINAL_CRC_addr + i);
		yield();
	}
	uint16_t realCrc = calculateEepromCrc();

	return savedCrc.number == realCrc;
}

void refreshEepromCrc()
{
	union Convert
	{
		uint16_t number = 0;
		uint8_t byteNum[4];
	} realCrc;
	realCrc.number = calculateEepromCrc();
	for (uint16_t i = 0; i < 2; i++)
	{
		EEPROM.write(FINAL_CRC_addr + i, realCrc.byteNum[i]);
		yield();
	}
	EEPROM.commit();
}

void clearEeprom()
{
	/*
	for (uint32_t i = 0; i < EEPROM.length(); i++)
	{
		EEPROM.write(i, 0);
		yield();
	}
	EEPROM.commit();
	refreshEepromCrc();*/
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

// input string format: ****nn="****",nnn,"****",nnn
commandTokens parseCommand(String& commandString, char cmd_divider, char arg_divider, bool findIndex)
{
	commandTokens params;
	params.index = -1;
	params.tokensNumber = 0;

	if (commandString.length() <= 0)
		return params;

	int equalSignPos = commandString.indexOf(cmd_divider);
	if (equalSignPos > 0)
	{
		int lastIndex = equalSignPos + 1;
		do // get argument tokens
		{
			int t = commandString.indexOf(arg_divider, lastIndex);
			if (t > equalSignPos)
			{
				params.arguments[params.tokensNumber] = commandString.substring(lastIndex, t);
			}
			else
			{
				params.arguments[params.tokensNumber] = commandString.substring(lastIndex);
			}

			params.arguments[params.tokensNumber].trim();
			params.tokensNumber++;
			lastIndex = t + 1;
			yield();
		} while (lastIndex > equalSignPos);
	}
	else
	{
		equalSignPos = commandString.length();
	}

	int cmdEnd = equalSignPos - 1;
	if (findIndex)
	{
		for (cmdEnd; cmdEnd >= 0; cmdEnd--)
		{
			if (commandString[cmdEnd] < '0' || commandString[cmdEnd] > '9')
			{
				cmdEnd++;
				break;
			}
			yield();
		}
		// get command index
		if (cmdEnd < equalSignPos)
			params.index = commandString.substring(cmdEnd, equalSignPos).toInt();
	}

	// get command name
	if (cmdEnd > 0)
		params.command = commandString.substring(0, cmdEnd);

	return params;
}

String set_output(uint8_t outNum, String& outStateStr)
{
	String str = "";
	str.reserve(30);

	uint16_t outState;
	bool pwm_mode = false;
	if (outStateStr == SWITCH_ON)
		outState = uint16_t(ON);
	else if (outStateStr == SWITCH_OFF)
		outState = uint16_t(OFF);
	else
	{
		pwm_mode = true;
		outState = outStateStr.toInt();
	}

	if (outNum >= 1 && outNum <= PIN_NUMBER && bitRead(OUTPUT_PINS, outNum - 1))
	{
		outputs[outNum - 1] = outState;
		if (pwm_mode)
			analogWrite(pins[outNum - 1], outputs[outNum - 1]);
		else
			digitalWrite(pins[outNum - 1], outputs[outNum - 1]);
		str = REPLY_OUTPUT_SET;
		str += String(outNum);
		str += F("=");
		str += String((outState));
	}
	else
	{
		str = REPLY_INCORRECT;
		str += F("output #: ");
		str += String(outNum);
		str += EOL;
	}

	return str;
}

String printConfig(bool toJson = false)
{
	String eq = F(": ");
	String delimiter = F("\r\n");
	String str = "";
	//str.reserve(3000);
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
		str = F("{\"");
	}

	str += PROPERTY_DEVICE_NAME;
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;

	str += PROPERTY_DEVICE_MAC;
	str += eq;
	str += WiFi.macAddress();
	str += delimiter;

	str += PROPERTY_FW_VERSION;
	str += eq;
	str += FW_VERSION;
	str += delimiter;

	str += REPLY_STA_SSID;
	str += eq;
	str += readConfigString(STA_SSID_addr, STA_SSID_size);
	str += delimiter;

	str += REPLY_STA_PASS;
	str += eq;
	str += readConfigString(STA_PASS_addr, STA_PASS_size);
	str += delimiter;

	str += REPLY_AP_SSID;
	str += eq;
	str += readConfigString(AP_SSID_addr, AP_SSID_size);
	str += delimiter;

	str += REPLY_AP_PASS;
	str += eq;
	str += readConfigString(AP_PASS_addr, AP_PASS_size);
	str += delimiter;

	str += REPLY_WIFI_CONNECT_TIME;
	str += eq;
	str += readConfigString(CONNECT_TIME_addr, CONNECT_TIME_size);
	str += delimiter;

	str += REPLY_WIFI_RECONNECT_PERIOD;
	str += eq;
	str += readConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size);
	str += delimiter;

	str += REPLY_WIFI_POWER;
	str += eq;
	str += String(readConfigFloat(WIFI_POWER_addr));
	str += delimiter;

	str += REPLY_WIFI_STANDART;
	str += eq;
	str += F("802.11");
	str += readConfigString(WIFI_STANDART_addr, WIFI_STANDART_size);
	str += delimiter;

	str += REPLY_WIFI_MODE;
	str += eq;
	long m = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (m < 0 || m >= wifiModes->length())
		m = WIFI_MODE_AUTO;
	str += wifiModes[m];
	str += delimiter;

	str += REPLY_CHECK_PERIOD;
	str += eq;
	str += String(readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt());
	str += delimiter;

	str += REPLY_AUTOREPORT;
	str += eq;
	m = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();
	for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
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
#ifdef HARD_UART_ENABLE
			case HARD_UART_TX:
				str += F("HardUART TX");
				break;
			case HARD_UART_RX:
				str += F("HardUART RX");
				break;
#endif
#ifdef SOFT_UART_ENABLE
			case SOFT_UART_TX:
				str += F("SoftUART TX");
				break;
			case SOFT_UART_RX:
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
#ifdef BUZZER_ENABLE
			case BUZZER_ENABLE:
				str += F("BUZZER");
				break;
#endif
			}
		}
		else if (pinModeCfg == INPUT || pinModeCfg == INPUT_PULLUP) // pin is input or input_pullup
		{
			if (pinModeCfg == INPUT)
				str += F("INPUT");
			else
				str += F("INPUT_PULLUP");
			str += F(", interrupt mode ");
			uint16_t l = INTERRUPT_MODE_size / PIN_NUMBER;
			uint8_t n = readConfigString(INTERRUPT_MODE_addr + m * l, l).toInt();
			if (n < 0 || n >= intModeList->length())
				n = 0;
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

	str += CFG_LOG_PERIOD;
	str += eq;
	str += String(readConfigString(LOG_PERIOD_addr, LOG_PERIOD_size).toInt());
	str += delimiter;
#endif
	yield();
#ifdef SLEEP_ENABLE
	str += REPLY_SLEEP_ON;
	str += eq;
	str += readConfigString(SLEEP_ON_addr, SLEEP_ON_size);
	str += delimiter;

	str += REPLY_SLEEP_OFF;
	str += eq;
	str += readConfigString(SLEEP_OFF_addr, SLEEP_OFF_size);
	str += delimiter;

	str += REPLY_SLEEP_ENABLE;
	str += eq;
	str += readConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef OTA_UPDATE
	str += REPLY_OTA_PORT;
	str += eq;
	str += readConfigString(OTA_PORT_addr, OTA_PORT_size);
	str += delimiter;

	str += REPLY_OTA_PASSWORD;
	str += eq;
	str += readConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size);
	str += delimiter;

	str += REPLY_OTA_ENABLE;
	str += eq;
	str += String(readConfigString(OTA_ENABLE_addr, OTA_ENABLE_size));
	str += delimiter;
#endif
	yield();
#ifdef TELNET_ENABLE
	str += REPLY_TELNET_PORT;
	str += eq;
	str += readConfigString(TELNET_PORT_addr, TELNET_PORT_size);
	str += delimiter;

	str += F("Telnet clients limit");
	str += eq;
	str += String(TELNET_ENABLE);
	str += delimiter;

	str += REPLY_TELNET_ENABLE;
	str += eq;
	str += readConfigString(TELNET_ENABLE_addr, TELNET_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef HTTP_ENABLE
	str += REPLY_HTTP_PORT;
	str += eq;
	str += readConfigString(HTTP_PORT_addr, HTTP_PORT_size);
	str += delimiter;

	str += REPLY_HTTP_ENABLE;
	str += eq;
	str += readConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef MQTT_ENABLE
	str += REPLY_MQTT_SERVER;
	str += eq;
	str += readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
	str += delimiter;

	str += REPLY_MQTT_PORT;
	str += eq;
	str += readConfigString(MQTT_PORT_addr, MQTT_PORT_size);
	str += delimiter;

	str += REPLY_MQTT_LOGIN;
	str += eq;
	str += readConfigString(MQTT_USER_addr, MQTT_USER_size);
	str += delimiter;

	str += REPLY_MQTT_PASS;
	str += eq;
	str += readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
	str += delimiter;

	str += REPLY_MQTT_ID;
	str += eq;
	str += readConfigString(MQTT_ID_addr, MQTT_ID_size);
	str += delimiter;

	str += REPLY_MQTT_TOPIC_IN;
	str += eq;
	str += readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
	str += delimiter;

	str += REPLY_MQTT_TOPIC_OUT;
	str += eq;
	str += readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
	str += delimiter;

	str += REPLY_MQTT_CLEAN;
	str += eq;
	str += readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size);
	str += delimiter;

	str += REPLY_MQTT_ENABLE;
	str += eq;
	str += readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef SMTP_ENABLE
	str += CFG_SMTP_SERVER;
	str += eq;
	str += readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	str += delimiter;

	str += CFG_SMTP_PORT;
	str += eq;
	str += readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size);
	str += delimiter;

	str += CFG_SMTP_LOGIN;
	str += eq;
	str += readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	str += delimiter;

	str += CFG_SMTP_PASS;
	str += eq;
	str += readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	str += delimiter;

	str += CFG_SMTP_TO;
	str += eq;
	str += readConfigString(SMTP_TO_addr, SMTP_TO_size);
	str += delimiter;

	str += CFG_SMTP_ENABLE;
	str += eq;
	str += readConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef TELEGRAM_ENABLE
	str += REPLY_TELEGRAM_TOKEN;
	str += eq;
	str += readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	str += delimiter;

	str += F("Admin #0");
	str += eq;
	str += uint64ToString(getTelegramUser(0));
	str += delimiter;

	for (m = 1; m < TELEGRAM_USERS_NUMBER; m++)
	{
		str += REPLY_TELEGRAM_USER;
		str += String(m);
		str += eq;
		str += uint64ToString(getTelegramUser(m));
		str += delimiter;
		yield();
	}

	str += REPLY_TELEGRAM_ENABLE;
	str += eq;
	str += readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef GSCRIPT_ENABLE
	str += CFG_GSCRIPT_TOKEN;
	str += eq;
	str += readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	str += delimiter;

	str += CFG_GSCRIPT_ENABLE;
	str += eq;
	str += readConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef PUSHINGBOX_ENABLE
	str += CFG_PUSHINGBOX_TOKEN;
	str += eq;
	str += readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
	str += delimiter;

	str += CFG_PUSHINGBOX_PARAMETER;
	str += eq;
	str += readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	str += delimiter;

	str += CFG_PUSHINGBOX_ENABLE;
	str += eq;
	str += readConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef NTP_TIME_ENABLE
	str += REPLY_NTP_SERVER;
	str += eq;
	str += readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	str += delimiter;

	str += REPLY_NTP_TIME_ZONE;
	str += eq;
	str += readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size);
	str += delimiter;

	str += REPLY_NTP_REFRESH_DELAY;
	str += eq;
	str += readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size);
	str += delimiter;

	str += REPLY_NTP_REFRESH_PERIOD;
	str += eq;
	str += readConfigString(NTP_REFRESH_PERIOD_addr, NTP_REFRESH_PERIOD_size);
	str += delimiter;

	str += REPLY_NTP_ENABLE;
	str += eq;
	str += readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef EVENTS_ENABLE
	for (m = 0; m < EVENTS_NUMBER; m++)
	{
		str += REPLY_EVENT_SET;
		str += String(m);
		str += eq;
		str += getEvent(m);
		str += delimiter;
		yield();
	}
	str += REPLY_EVENTS_ENABLE;
	str += eq;
	str += readConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef SCHEDULER_ENABLE
	for (m = 0; m < SCHEDULES_NUMBER; m++)
	{
		str += REPLY_SCHEDULE_SET;
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
	str += REPLY_SCHEDULER_ENABLE;
	str += eq;
	str += readConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef GSM_M590_ENABLE
	str += F("M590 GSM modem");
	str += eq;
#if GSM_M590_ENABLE == HARD_UART
	str += F("HardUART");
#elif GSM_M590_ENABLE == SOFT_UART
	str += F("SoftUART");
#endif
	str += delimiter;
#endif
	yield();
#ifdef GSM_SIM800_ENABLE
	str += F("SIM800 GSM modem");
	str += eq;
#if GSM_SIM800_ENABLE == HARD_UART
	str += F("HardUART");
#elif GSM_SIM800_ENABLE == SOFT_UART
	str += F("SoftUART");
#endif
	str += delimiter;
#endif
	yield();
#ifdef GSM_ENABLE
	for (m = 0; m < GSM_USERS_NUMBER; m++)
	{
		str += CFG_GSM_USER;
		str += String(m);
		str += eq;
		str += getGsmUser(m);
		str += delimiter;
		yield();
	}
	str += CFG_GSM_ENABLE;
	str += eq;
	str += readConfigString(GSM_ENABLE_addr, GSM_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef TM1637DISPLAY_ENABLE
	str += F("TM1637 CLK/DIO pin");
	str += eq;
	str += TM1637_CLK;
	str += F("/");
	str += TM1637_DIO;
	str += delimiter;
#endif
	yield();
#ifdef SSD1306DISPLAY_ENABLE
	str += F("SSD1306 display");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef DISPLAY_ENABLED
	str += CFG_DISPLAY_REFRESH;
	str += eq;
	str += readConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size);
	str += delimiter;
#endif
	yield();
#ifdef AMS2320_ENABLE
	str += F("AMS2320");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef HTU21D_ENABLE
	str += F("HTU21D");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef BME280_ENABLE
	str += F("BME280");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef BMP180_ENABLE
	str += F("BMP180");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef AHTx0_ENABLE
	str += F("AHTx0");
	str += eq;
	str += F("I2C");
	str += delimiter;
#endif
	yield();
#ifdef DS18B20_ENABLE
	str += F("DS18B20");
	str += eq;
	str += F("OneWire");
	str += delimiter;
#endif
	yield();
#ifdef DHT_ENABLE
	str += F("DHT");
	str += DHT_ENABLE;
	str += F(" pin");
	str += eq;
	str += DHT_PIN;
	str += delimiter;
#endif
	yield();
#ifdef MH_Z19_UART_ENABLE
	str += F("MH-Z19 CO2");
	str += eq;
#if MH_Z19_UART_ENABLE == HARD_UART
	str += F("HardUART");
#elif MH_Z19_UART_ENABLE == SOFT_UART
	str += F("SoftUART");
#endif
	str += delimiter;
#endif
	yield();
#ifdef MH_Z19_PPM_ENABLE
	str += F("MH-Z19 CO2 PPM pin");
	str += eq;
	str += MH_Z19_PPM_ENABLE;
	str += delimiter;
#endif
	yield();
#ifdef ADC_ENABLE
	str += F("ADC pin");
	str += eq;
	str += F("A0");
	str += delimiter;
#endif
	yield();
	str += F("EEPROM size");
	str += eq;
	str += String(collectEepromSize());
	str += delimiter;
	yield();
	if (toJson)
		str += F("\"}");

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
	if (toJson)
		str = F("{\"");

	str += PROPERTY_DEVICE_NAME;
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;

	str += PROPERTY_DEVICE_MAC;
	str += eq;
	str += WiFi.macAddress();
	str += delimiter;

	str += PROPERTY_FW_VERSION;
	str += eq;
	str += FW_VERSION;
	str += delimiter;

	str += F("Time");
	str += eq;
	if (timeIsSet)
		str += timeToString(now());
	else
		str += F("not set");
	str += delimiter;

	str += F("WiFi enabled");
	str += eq;
	str += String(wifiEnable);
	str += delimiter;

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

	if (WiFi.softAPgetStationNum() > 0)
	{
		str += F("AP Clients connected");
		str += eq;
		str += String(WiFi.softAPgetStationNum());
		str += delimiter;
	}

#ifdef TELNET_ENABLE
	if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0)
	{
		uint8_t netClientsNum = 0;
		for (uint8_t i = 0; i < TELNET_ENABLE; i++)
		{
			if (serverClient[i] && serverClient[i].connected())
				netClientsNum++;
			yield();
		}
		str += F("Telnet clients connected");
		str += eq;
		str += String(netClientsNum);
		str += F("/");
		str += TELNET_ENABLE;
		str += delimiter;
	}
#endif

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM buffer usage");
	str += eq;
	str += String(telegramOutboundBufferPos);
	str += F("/");
	str += String(TELEGRAM_MESSAGE_BUFFER_SIZE);
	str += delimiter;
#endif

#ifdef EVENTS_ENABLE
	str += F("Events enabled");
	str += eq;
	str += String(eventsEnable);
	str += delimiter;
	for (uint8_t i = 0; i < EVENTS_NUMBER; i++)
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
	for (uint8_t i = 0; i < SCHEDULES_NUMBER; i++)
	{
		String scheduleStr = getSchedule(i);
		if (scheduleStr.length() > 0)
		{
			str += F("Schedule #");
			str += String(i);
			str += F(" last run time");
			str += eq;
			uint32_t runTime = readScheduleExecTime(i);
			str += String(year(runTime));
			str += F(".");
			str += String(month(runTime));
			str += F(".");
			str += String(day(runTime));
			str += SPACE;
			str += String(hour(runTime));
			str += COLON;
			str += String(minute(runTime));
			str += COLON;
			str += String(second(runTime));
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

#ifdef OTA_UPDATE
	str += F("OTA enabled");
	str += eq;
	str += String(otaEnable);
	str += delimiter;
#endif

#ifdef AMS2320_ENABLE
	str += F("AMS2320 found");
	str += eq;
	str += String(am2320_enable);
	str += delimiter;
#endif

#ifdef HTU21D_ENABLE
	str += F("HTU21D found");
	str += eq;
	str += String(htu21d_enable);
	str += delimiter;
#endif

#ifdef BME280_ENABLE
	str += F("BME280 found");
	str += eq;
	str += String(bme280_enable);
	str += delimiter;
#endif

#ifdef BMP180_ENABLE
	str += F("BMP180 found");
	str += eq;
	str += String(bmp180_enable);
	str += delimiter;
#endif

#ifdef AHTx0_ENABLE
	str += F("AHTx0 found");
	str += eq;
	str += String(ahtx0_enable);
	str += delimiter;
#endif

#ifdef DS18B20_ENABLE
	str += F("DS18B20 found");
	str += eq;
	str += String(ds1820_enable);
	str += delimiter;
#endif

#ifdef DHT_ENABLE
	str += F("DHT found");
	str += eq;
	str += String(dht_enable);
	str += delimiter;
#endif

	str += F("Free memory");
	str += eq;
	str += String(ESP.getFreeHeap());
	str += delimiter;

	str += F("Heap fragmentation");
	str += eq;
	str += String(ESP.getHeapFragmentation());

	if (toJson)
		str += F("\"}");

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

		"[ADMIN][FLASH] autoreport=n (bit[0..7] 1=UART,2=TELNET,4=MQTT,8=TELEGRAM,16=GSCRIPT,32=PUSHINGBOX,64=SMTP,128=GSM)\r\n"

#ifdef SSD1306DISPLAY_ENABLE
		"[ADMIN][FLASH] display_refresh=n (sec.)\r\n"
#endif

		"[ADMIN][FLASH] check_period=n (sec.)\r\n"

		"[ADMIN][FLASH] device_name=****\r\n"

		"[ADMIN][FLASH] sta_ssid=****\r\n"
		"[ADMIN][FLASH] sta_pass=****\r\n"
		"[ADMIN][FLASH] ap_ssid=**** (empty for device_name+MAC)\r\n"
		"[ADMIN][FLASH] ap_pass=****\r\n"
		"[ADMIN][FLASH] wifi_standart=B/G/N\r\n"
		"[ADMIN][FLASH] wifi_power=n (0..20.5)\r\n"
		"[ADMIN][FLASH] wifi_mode=AUTO/STATION/APSTATION/AP/OFF\r\n"
		"[ADMIN][FLASH] wifi_connect_time=n (sec.)\r\n"
		"[ADMIN][FLASH] wifi_reconnect_period=n (sec.)\r\n"
		"[ADMIN]        wifi_enable=on/off\r\n"

#ifdef LOG_ENABLE
		"[ADMIN][FLASH] log_period=n (sec., no less than 'check_period' in fact)\r\n"
		"[ADMIN][FLASH] getlog\r\n"
#endif

#ifdef SLEEP_ENABLE
		"[ADMIN][FLASH] sleep_on=n (sec.)\r\n"
		"[ADMIN][FLASH] sleep_off=n (sec., max 4 294 967 sec ~ 71 min.)\r\n" // max 4 294 967 295 ms, which is about ~71 minutes
		"[ADMIN][FLASH] sleep_enable=on/off\r\n"
#endif

#ifdef OTA_UPDATE
		"[ADMIN][FLASH] ota_port=n\r\n"
		"[ADMIN][FLASH] ota_pass=****\r\n"
		"[ADMIN][FLASH] ota_enable=on/off\r\n"
#endif

#ifdef TELNET_ENABLE
		"[ADMIN][FLASH] telnet_port=n\r\n"
		"[ADMIN][FLASH] telnet_enable=on/off\r\n"
#endif

#ifdef HTTP_ENABLE
		"[ADMIN][FLASH] http_port=n\r\n"
		"[ADMIN][FLASH] http_enable=on/off\r\n"
#endif

#ifdef NTP_TIME_ENABLE
		"[ADMIN][FLASH] ntp_server=****\r\n"
		"[ADMIN][FLASH] ntp_time_zone=n\r\n"
		"[ADMIN][FLASH] ntp_refresh_delay=n (sec.)\r\n"
		"[ADMIN][FLASH] ntp_refresh_period=n (sec.)\r\n"
		"[ADMIN][FLASH] ntp_enable=on/off\r\n"
#endif

#ifdef MQTT_ENABLE
		"[ADMIN][FLASH] mqtt_server=****\r\n"
		"[ADMIN][FLASH] mqtt_port=n\r\n"
		"[ADMIN][FLASH] mqtt_login=****\r\n"
		"[ADMIN][FLASH] mqtt_pass=****\r\n"
		"[ADMIN][FLASH] mqtt_id=**** (empty for device_name+MAC)\r\n"
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

#ifdef BUZZER_ENABLE
		"[ADMIN]        buzz=n,m (n=frequency, msec.; m=delay, msec.)\r\n"
#endif
		"[ADMIN]        reset");
}

String timeToString(uint32_t time)
{
	// digital clock display of the time
	String tmp = String(hour(time));
	tmp += COLON;
	tmp += String(minute(time));
	tmp += COLON;
	tmp += String(second(time));
	tmp += SPACE;
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
	am2320 = Adafruit_AM2320(&Wire);
	if (!am2320.begin())
	{
		am2320_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find AMS2320 sensor!"));
#endif
	}
	else
	{
		am2320_enable = true;
		sensorData.ams_humidity = am2320.readHumidity();
		sensorData.ams_temp = am2320.readTemperature();
		if (isnan(sensorData.ams_temp))
			am2320_enable = false;
	}
#endif

#ifdef HTU21D_ENABLE
	htu21d = Adafruit_HTU21DF();
	if (!htu21d.begin())
	{
		htu21d_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find HTU21D sensor!"));
#endif
	}
	else
	{
		htu21d_enable = true;
		sensorData.htu21d_humidity = htu21d.readHumidity();
		sensorData.htu21d_temp = htu21d.readTemperature();
	}
#endif

#ifdef BME280_ENABLE
	if (!bme280.begin(BME280_ENABLE))
	{
		bme280_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find BME280 sensor!"));
#endif
	}
	else
	{
		bme280_enable = true;
		sensorData.bme280_humidity = bme280.readHumidity();
		sensorData.bme280_temp = bme280.readTemperature();
		sensorData.bme280_pressure = bme280.readPressure();
	}
#endif

#ifdef BMP180_ENABLE
	if (!bmp180.begin())
	{
		bmp180_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find BMP180 sensor!"));
#endif
	}
	else
	{
		bmp180_enable = true;
		sensorData.bmp180_temp = bmp180.readTemperature();
		sensorData.bmp180_pressure = bmp180.readPressure();
	}
#endif

#ifdef AHTx0_ENABLE
	if (!ahtx0.begin())
	{
		ahtx0_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find AHTx0 sensor!"));
#endif
	}
	else
	{
		ahtx0_enable = true;
		sensors_event_t humidity, temp;
		ahtx0.getEvent(&humidity, &temp);
		sensorData.ahtx0_humidity = humidity.relative_humidity;
		sensorData.ahtx0_temp = temp.temperature;
	}
#endif

#ifdef DS18B20_ENABLE
	ds1820.begin();
	if (ds1820.getDS18Count() <= 0)
	{
		ds1820_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find DS18B20 sensor!"));
#endif
	}
	else
	{
		ds1820_enable = true;
		ds1820.requestTemperatures();
		delay(5);
		sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
		if (sensorData.ds1820_temp > 100)
		{
			ds1820.requestTemperatures();
			delay(5);
			sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
		}
	}
#endif

#ifdef DHT_ENABLE
	dht.begin();
	if (!dht.read())
	{
		dht_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find DHT sensor!"));
#endif
	}
	else
	{
		dht_enable = true;
		sensorData.dht_humidity = dht.readHumidity();
		sensorData.dht_temp = dht.readTemperature();
	}
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

String parseSensorReport(sensorDataCollection& data, String delimiter, bool toJson = false)
{
	String eq = F("=");
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
	}

	String str;
	str.reserve(512);
	if (toJson)
		str = F("{\"");
	str += PROPERTY_DEVICE_NAME;
	str += eq;
	str += deviceName;
	str += delimiter;

	str += PROPERTY_DEVICE_MAC;
	str += eq;
	str += WiFi.macAddress();
	str += delimiter;

	str += F("Time");
	str += eq;
	str += String(data.hour);
	str += COLON;
	str += String(data.minute);
	str += COLON;
	str += String(data.second);
	str += SPACE;
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

#ifdef AHTx0_ENABLE
	if (ahtx0_enable)
	{
		str += delimiter;
		str += F("AHTx0 t");
		str += eq;
		str += String(data.ahtx0_temp);
		str += delimiter;
		str += F("AHTx0 h");
		str += eq;
		str += String(data.ahtx0_humidity);
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
	if (dht_enable)
	{
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
	}
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
	if (toJson)
		str += F("\"}");
	return str;
}

String processCommand(String& command, uint8_t channel, bool isAdmin)
{
	commandTokens cmd = parseCommand(command, '=', ',', true);
	cmd.command.toLowerCase();
	String eq = F("=\"");
	String str = "";
	str.reserve(2048);
	if (channel == CHANNEL_GSM)
		str += F("^");

	if (cmd.command == CMD_GET_SENSOR)
	{
		sensorDataCollection sensorData = collectData();
		str += parseSensorReport(sensorData, EOL);
	}
	else if (cmd.command == CMD_GET_STATUS)
	{
		str += printStatus();
	}
	else if (cmd.command == CMD_HELP)
	{
		str += printHelp();
	}
	else if (isAdmin)
	{
		if (cmd.command == CMD_GET_CONFIG)
		{
			str += printConfig();
		}
		else if (cmd.command == CMD_TIME_SET)
		{
			uint8_t _hr = cmd.arguments[0].substring(11, 13).toInt();
			uint8_t _min = cmd.arguments[0].substring(14, 16).toInt();
			uint8_t _sec = cmd.arguments[0].substring(17, 19).toInt();
			uint8_t _day = cmd.arguments[0].substring(8, 10).toInt();
			uint8_t _month = cmd.arguments[0].substring(5, 7).toInt();
			uint16_t _yr = cmd.arguments[0].substring(0, 4).toInt();
			setTime(_hr, _min, _sec, _day, _month, _yr);
			str += REPLY_TIME_SET;
			str += String(_yr);
			str += F(".");
			str += String(_month);
			str += F(".");
			str += String(_day);
			str += SPACE;
			str += String(_hr);
			str += COLON;
			str += String(_min);
			str += COLON;
			str += String(_sec);
			timeIsSet = true;
#ifdef NTP_TIME_ENABLE
			NTPenable = false;
#endif
		}
		else if (cmd.command == CMD_CHECK_PERIOD)
		{
			checkSensorPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			str += REPLY_CHECK_PERIOD;
			str += eq;
			str += String(uint16_t(checkSensorPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(uint16_t(checkSensorPeriod / 1000UL)));
		}
		else if (cmd.command == CMD_DEVICE_NAME)
		{
			deviceName = cmd.arguments[0];
			str += REPLY_DEVICE_NAME;
			str += eq;
			str += deviceName;
			str += QUOTE;
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
		}
		else if (cmd.command == CMD_STA_SSID)
		{
			str += REPLY_STA_SSID;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(STA_SSID_addr, STA_SSID_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_STA_PASS)
		{
			str += REPLY_STA_PASS;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(STA_PASS_addr, STA_PASS_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_AP_SSID)
		{
			str += REPLY_AP_SSID;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(AP_SSID_addr, AP_SSID_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_AP_PASS)
		{
			str += REPLY_AP_PASS;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(AP_PASS_addr, AP_PASS_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_WIFI_STANDART)
		{
			if (cmd.arguments[0] == F("B") || cmd.arguments[0] == F("G") || cmd.arguments[0] == F("N"))
			{
				str += REPLY_WIFI_STANDART;
				str += eq;
				str += String(cmd.arguments[0]);
				str += QUOTE;
				writeConfigString(WIFI_STANDART_addr, WIFI_STANDART_size, cmd.arguments[0]);
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("WiFi standart: ");
				str += String(cmd.arguments[0]);
			}
		}
		else if (cmd.command == CMD_WIFI_POWER)
		{
			float power = cmd.arguments[0].toFloat();
			if (power >= 0 && power <= 20.5)
			{
				str += REPLY_WIFI_POWER;
				str += eq;
				str += String(power);
				str += QUOTE;
				writeConfigFloat(WIFI_POWER_addr, power);
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("WiFi power: ");
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_WIFI_MODE)
		{
			cmd.arguments[0].toUpperCase();
			uint8_t wifi_mode = 255;
			if (cmd.arguments[0] == F("AUTO"))
			{
				wifi_mode = WIFI_MODE_AUTO;
			}
			else if (cmd.arguments[0] == F("STATION"))
			{
				wifi_mode = WIFI_MODE_STA;
			}
			else if (cmd.arguments[0] == F("APSTATION"))
			{
				wifi_mode = WIFI_MODE_AP_STA;
			}
			else if (cmd.arguments[0] == F("AP"))
			{
				wifi_mode = WIFI_MODE_AP;
			}
			else if (cmd.arguments[0] == F("OFF"))
			{
				wifi_mode = WIFI_MODE_OFF;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
			if (wifi_mode != 255)
			{
				str += REPLY_WIFI_MODE;
				str += eq;
				str += wifiModes[wifi_mode];
				str += QUOTE;
				writeConfigString(WIFI_MODE_addr, WIFI_MODE_size, String(wifi_mode));
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("WiFi mode: ");
				str += String(cmd.arguments[0]);
			}
		}
		else if (cmd.command == CMD_WIFI_CONNECT_TIME)
		{
			connectTimeLimit = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			str += REPLY_WIFI_CONNECT_TIME;
			str += eq;
			str += String(uint16_t(connectTimeLimit / 1000UL));
			str += QUOTE;
			writeConfigString(CONNECT_TIME_addr, CONNECT_TIME_size, String(uint16_t(connectTimeLimit / 1000UL)));
		}
		else if (cmd.command == CMD_WIFI_RECONNECT_PERIOD)
		{
			reconnectPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			str += REPLY_WIFI_RECONNECT_PERIOD;
			str += eq;
			str += String(uint16_t(reconnectPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(CONNECT_PERIOD_addr, CONNECT_PERIOD_size, String(uint16_t(reconnectPeriod / 1000UL)));
		}
		else if (cmd.command == CMD_WIFI_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				wifiEnable = false;
				Start_OFF_Mode();
				str += REPLY_WIFI_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				wifiEnable = true;
				startWiFi();
				str += REPLY_WIFI_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_RESET)
		{
			if (channel == CHANNEL_TELEGRAM)
			{
				uartCommand = F("reset");
				uartReady = true;
			}
			else
			{
#ifdef DEBUG_MODE
				//Serial.println(F("Resetting..."));
				//Serial.flush();
#endif
				ESP.restart();
			}
		}
		else if (cmd.command == CMD_AUTOREPORT)
		{
			autoReport = cmd.arguments[0].toInt();
			writeConfigString(AUTOREPORT_addr, AUTOREPORT_size, String(autoReport));
			str += REPLY_AUTOREPORT;
			str += eq;
			for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
			{
				if (bitRead(autoReport, b))
				{
					str += channels[b];
					str += F(",");
				}
				yield();
			}
			str += QUOTE;
		}
		else if (cmd.command == CMD_PIN_MODE_SET)
		{
			if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
			{
				cmd.arguments[0].toUpperCase();
				uint8_t intModeNum = 255;
				if (cmd.arguments[0] == pinModeList[3])
					intModeNum = 3; //off
				else if (cmd.arguments[0] == pinModeList[INPUT])
					intModeNum = INPUT;
				else if (cmd.arguments[0] == pinModeList[OUTPUT])
					intModeNum = OUTPUT;
				else if (cmd.arguments[0] == pinModeList[INPUT_PULLUP])
					intModeNum = INPUT_PULLUP;
				else
				{
					str = REPLY_INCORRECT;
					str += F("pin mode: ");
					str += cmd.arguments[0];
				}
				if (intModeNum != 255)
				{
					uint16_t l = PIN_MODE_size / PIN_NUMBER;
					writeConfigString(PIN_MODE_addr + (cmd.index - 1) * l, l, String(intModeNum));

					str += REPLY_PIN_MODE_SET;
					str += String(cmd.index);
					str += F(" mode: ");
					str += pinModeList[intModeNum];
				}
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("pin: ");
				str += String(cmd.index);
			}
		}
		else if (cmd.command == CMD_INTERRUPT_MODE_SET)
		{
			if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
			{
				cmd.arguments[0].toUpperCase();
				uint8_t intModeNum = 255;
				if (cmd.arguments[0] == intModeList[0])
					intModeNum = 0;
				else if (cmd.arguments[0] == intModeList[RISING])
					intModeNum = RISING;
				else if (cmd.arguments[0] == intModeList[FALLING])
					intModeNum = FALLING;
				else if (cmd.arguments[0] == intModeList[CHANGE])
					intModeNum = CHANGE;
				else
				{
					str = REPLY_INCORRECT;
					str += F("INT mode: ");
					str += String(cmd.arguments[0]);
				}
				if (intModeNum != 255)
				{
					uint16_t l = INTERRUPT_MODE_size / PIN_NUMBER;
					writeConfigString(INTERRUPT_MODE_addr + (cmd.index - 1) * l, l, String(intModeNum));
					str += REPLY_INTERRUPT_MODE_SET;
					str += String(cmd.index);
					str += F(" mode: ");
					str += intModeList[intModeNum];
				}
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("INT: ");
				str += String(cmd.index);
			}
		}
		else if (cmd.command == CMD_OUTPUT_INIT_SET)
		{
			if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
			{
				if (cmd.arguments[0] != SWITCH_ON && cmd.arguments[0] != SWITCH_OFF)
				{
					uint16_t outState = 0;
					outState = cmd.arguments[0].toInt();
					cmd.arguments[0] = String(outState);
				}
				uint16_t l = OUTPUT_INIT_size / PIN_NUMBER;
				writeConfigString(OUTPUT_INIT_addr + (cmd.index - 1) * l, l, cmd.arguments[0]);
				str += REPLY_OUTPUT_INIT_SET;
				str += String(cmd.index);
				str += F(" initial state: ");
				str += cmd.arguments[0];
			}
			else
			{
				str = REPLY_INCORRECT;
				str += F("OUT");
				str += String(cmd.index);
			}
		}
		else if (cmd.command == CMD_OUTPUT_SET)
		{
			str += set_output(cmd.index, cmd.arguments[0]);
		}

#ifdef LOG_ENABLE
		else if (cmd.command == CMD_LOG_PERIOD)
		{
			logPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			str += REPLY_LOG_PERIOD;
			str += eq;
			str += String(uint16_t(logPeriod / 1000UL));
			str += QUOTE;
			writeConfigString(LOG_PERIOD_addr, LOG_PERIOD_size, String(uint16_t(logPeriod / 1000UL)));
		}
		//getLog - to be implemented
		else if (cmd.command == CMD_GETLOG)
		{
			for (int i = 0; i < LOG_SIZE; i++)
			{
				if (history_log[i].year > 0)
				{
					str += parseSensorReport(history_log[i], EOL, true);
					switch (channel)
					{
#ifdef DEBUG_MODE
					case CHANNEL_UART:
						Serial.println(str);
						break;
#endif
					case CHANNEL_TELNET:

						break;
#ifdef MQTT_ENABLE
					case CHANNEL_MQTT:

						break;
#endif
#ifdef TELEGRAM_ENABLE
					case CHANNEL_TELEGRAM:

						break;
#endif
#ifdef GSCRIPT_ENABLE
					case CHANNEL_GSCRIPT:

						break;
#endif
#ifdef PUSHINGBOX_ENABLE
					case CHANNEL_PUSHINGBOX:

						break;
#endif
#ifdef SMTP_ENABLE
					case CHANNEL_SMTP:

						break;
#endif
#ifdef GSM_ENABLE
					case CHANNEL_GSM:

						break;
#endif
					default:
						break;
					};
				}
			}
		}
#endif

#ifdef TELNET_ENABLE
		else if (cmd.command == CMD_TELNET_PORT)
		{
			uint16_t telnetPort = cmd.arguments[0].toInt();
			str += REPLY_TELNET_PORT;
			str += eq;
			str += String(telnetPort);
			str += QUOTE;
			writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
			//WiFiServer server(telnetPort);
		}
		else if (cmd.command == CMD_TELNET_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				telnetEnable = false;
				telnetServer.stop();
				str += REPLY_TELNET_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				telnetEnable = true;
				telnetServer.begin();
				telnetServer.setNoDelay(true);
				str += REPLY_TELNET_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef BUZZER_ENABLE
		else if (cmd.command == CMD_BUZZ)
		{
			int f = uint32_t(cmd.arguments[0].toInt());
			long d = cmd.arguments[1].toInt();
			tone(BUZZER_ENABLE, f, d);
		}
#endif

#ifdef SLEEP_ENABLE
		else if (cmd.command == CMD_SLEEP_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				//sleepEnable = false;
				writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_OFF_NUMBER);
				str += REPLY_SLEEP_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				//sleepEnable = true;
				writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_ON_NUMBER);
				str += REPLY_SLEEP_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_SLEEP_ON)
		{
			activeTimeOut_ms = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			if (activeTimeOut_ms == 0)
				activeTimeOut_ms = uint32_t(2 * connectTimeLimit);

			str += REPLY_SLEEP_ON;
			str += eq;
			str += String(uint16_t(activeTimeOut_ms / 1000UL));
			str += QUOTE;
			writeConfigString(SLEEP_ON_addr, SLEEP_ON_size, String(uint16_t(activeTimeOut_ms / 1000UL)));
		}
		else if (cmd.command == CMD_SLEEP_OFF)
		{
			sleepTimeOut_us = uint32_t(cmd.arguments[0].toInt() * 1000000UL);
			if (sleepTimeOut_us == 0)
				sleepTimeOut_us = uint32_t(20UL * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)

			str += REPLY_SLEEP_OFF;
			str += eq;
			str += String(uint16_t(sleepTimeOut_us / 1000000UL));
			str += QUOTE;
			writeConfigString(SLEEP_OFF_addr, SLEEP_OFF_size, String(uint16_t(sleepTimeOut_us / 1000000UL)));
		}
#endif

#ifdef HTTP_ENABLE
		else if (cmd.command == CMD_HTTP_PORT)
		{
			httpPort = cmd.arguments[0].toInt();
			str += REPLY_HTTP_PORT;
			str += eq;
			str += String(httpPort);
			str += QUOTE;
			writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
			//if (httpServerEnable) http_server.begin(httpPort);
		}
		else if (cmd.command == CMD_HTTP_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_OFF_NUMBER);
				//httpServerEnable = false;
				//if (httpServerEnable) http_server.stop();
				str += REPLY_HTTP_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_ON_NUMBER);
				/*httpServerEnable = true;
				http_server.on("/", handleRoot);
				http_server.onNotFound(handleNotFound);
				http_server.begin(httpPort);*/
				str += REPLY_HTTP_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef MQTT_ENABLE
		// send_mqtt=topic,message - to be implemented
		else if (cmd.command == CMD_SEND_MQTT)
		{
			if (mqtt_send(cmd.arguments[1], cmd.arguments[1].length(), cmd.arguments[0]))
			{
				str += F("Message sent");
			}
			else
			{
				str += F("Message not sent");
			}
		}
		else if (cmd.command == CMD_MQTT_SERVER)
		{
			IPAddress tmpAddr;
			str += REPLY_MQTT_SERVER;
			str += eq;
			if (tmpAddr.fromString(cmd.arguments[0]))
			{
				IPAddress mqtt_ip_server = tmpAddr;
				str += mqtt_ip_server.toString();
			}
			else
			{
				str += cmd.arguments[0];
			}
			str += QUOTE;
			writeConfigString(MQTT_SERVER_addr, MQTT_SERVER_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_PORT)
		{
			uint16_t mqtt_port = cmd.arguments[0].toInt();
			str += REPLY_MQTT_PORT;
			str += eq;
			str += String(mqtt_port);
			str += QUOTE;
			writeConfigString(MQTT_PORT_addr, MQTT_PORT_size, String(mqtt_port));
			//mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else if (cmd.command == CMD_MQTT_LOGIN)
		{
			str += REPLY_MQTT_LOGIN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(MQTT_USER_addr, MQTT_USER_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_PASS)
		{
			str += REPLY_MQTT_PASS;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(MQTT_PASS_addr, MQTT_PASS_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_ID)
		{
			str += REPLY_MQTT_ID;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(MQTT_ID_addr, MQTT_ID_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_TOPIC_IN)
		{
			str += REPLY_MQTT_TOPIC_IN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_TOPIC_OUT)
		{
			str += REPLY_MQTT_TOPIC_OUT;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_MQTT_CLEAN)
		{
			str += REPLY_MQTT_CLEAN;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size, SWITCH_ON_NUMBER);
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
			}
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
				mqttEnable = false;
				str += REPLY_MQTT_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
				mqtt_client.disconnect();
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_ON_NUMBER);
				mqttEnable = true;
				str += REPLY_MQTT_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
				mqtt_connect();
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef SMTP_ENABLE
		// send_mail=address,subject,message
		else if (cmd.command == CMD_SEND_MAIL)
		{
			if (cmd.arguments[0].length() > 0)
			{
				if (cmd.arguments[1].length() > 0)
				{
					if (sendMail(cmd.arguments[1], cmd.arguments[2], cmd.arguments[0]))
					{
						str += F("Mail sent");
					}
					else
					{
						str += F("Mail send failed");
					}
				}
				else
					str += F("Message empty");
			}
			else
				str += F("Address empty");
		}
		else if (cmd.command == CMD_SMTP_SERVER)
		{
			str += REPLY_SMTP_SERVER;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_SMTP_PORT)
		{
			uint16_t smtpServerPort = cmd.arguments[0].toInt();
			str += REPLY_SMTP_PORT;
			str += eq;
			str += String(smtpServerPort);
			str += QUOTE;
			writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
		}
		else if (cmd.command == CMD_SMTP_LOGIN)
		{
			str += REPLY_SMTP_LOGIN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_SMTP_PASS)
		{
			str += REPLY_SMTP_PASS;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_SMTP_TO)
		{
			str += REPLY_SMTP_TO;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(SMTP_TO_addr, SMTP_TO_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_SMTP_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_OFF_NUMBER);
				//smtpEnable = false;
				str += REPLY_SMTP_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_ON_NUMBER);
				//smtpEnable = true;
				str += REPLY_SMTP_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef GSM_ENABLE
		// send_sms=[*/user#],message
		else if (cmd.command == CMD_SEND_SMS)
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (cmd.arguments[0] == STAR) //if * then all
			{
				i = 0;
				j = GSM_USERS_NUMBER;
			}
			else
			{
				i = cmd.arguments[0].toInt();
				j = i + 1;
			}

			if (i <= GSM_USERS_NUMBER)
			{
				for (; i < j; i++)
				{
					String gsmUser = getGsmUser(i);
					if (gsmUser.length() > 0)
					{
						if (sendSMS(gsmUser, cmd.arguments[1]))
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
		else if (cmd.command == CMD_GSM_USER)
		{
			if (cmd.index >= 0 && cmd.index < GSM_USERS_NUMBER)
			{
				uint16_t m = GSM_USERS_TABLE_size / GSM_USERS_NUMBER;
				writeConfigString(GSM_USERS_TABLE_addr + cmd.index * m, m, cmd.arguments[0]);
				str += REPLY_GSM_USER;
				str += String(cmd.index);
				str += F(" is now: ");
				str += cmd.arguments[0];
			}
			else
			{
				str = F("User #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(GSM_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_GSM_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_OFF_NUMBER);
				gsmEnable = false;
				str += REPLY_GSM_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_ON_NUMBER);
				gsmEnable = true;
				str += REPLY_GSM_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef TELEGRAM_ENABLE
		// send_telegram=[*/user#],message
		else if (cmd.command == CMD_SEND_TELEGRAM)
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (cmd.arguments[0] == STAR) //if * then all
			{
				i = 0;
				j = TELEGRAM_USERS_NUMBER;
			}
			else
			{
				i = cmd.arguments[0].toInt();
				j = i + 1;
			}

			if (i <= TELEGRAM_USERS_NUMBER)
			{
				for (; i < j; i++)
				{
					int64_t gsmUser = getTelegramUser(i);
					if (gsmUser != 0)
					{
						if (sendToTelegramServer(gsmUser, cmd.arguments[1]))
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
		else if (cmd.command == CMD_TELEGRAM_USER)
		{
			uint64_t newUser = StringToUint64(cmd.arguments[0]);
			if (cmd.index >= 0 && cmd.index < TELEGRAM_USERS_NUMBER)
			{
				uint16_t m = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
				writeConfigString(TELEGRAM_USERS_TABLE_addr + cmd.index * m, m, uint64ToString(newUser));
				str += REPLY_TELEGRAM_USER;
				str += uint64ToString(cmd.index);
				str += F(" is now: ");
				str += uint64ToString(newUser);
			}
			else
			{
				str += F("Telegram user #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(TELEGRAM_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_TELEGRAM_TOKEN)
		{
			str += REPLY_TELEGRAM_TOKEN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, cmd.arguments[0]);
			//myBot.setTelegramToken(telegramToken);
		}
		else if (cmd.command == CMD_TELEGRAM_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_OFF_NUMBER);
				//telegramEnable = false;
				str += REPLY_TELEGRAM_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_ON_NUMBER);
				//telegramEnable = true;
				//myBot.wifiConnect(STA_Ssid, STA_Password);
				//myBot.setTelegramToken(telegramToken);
				//myBot.testConnection();
				str += REPLY_TELEGRAM_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef GSCRIPT_ENABLE
		//send_gscript - to be implemented
		else if (cmd.command == CMD_GSCRIPT_TOKEN)
		{
			str += REPLY_GSCRIPT_TOKEN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_GSCRIPT_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_OFF_NUMBER);
				//gScriptEnable = false;
				str += REPLY_GSCRIPT_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_ON_NUMBER);
				//gScriptEnable = true;
				str += REPLY_GSCRIPT_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef PUSHINGBOX_ENABLE
		//send_pushingbox - to be implemented
		else if (cmd.command == CMD_PUSHINGBOX_TOKEN)
		{
			str += REPLY_PUSHINGBOX_TOKEN;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_PUSHINGBOX_PARAMETER)
		{
			str += REPLY_PUSHINGBOX_PARAMETER;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_PUSHINGBOX_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_OFF_NUMBER);
				//pushingBoxEnable = false;
				str += REPLY_PUSHINGBOX_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_ON_NUMBER);
				//pushingBoxEnable = true;
				str += REPLY_PUSHINGBOX_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef NTP_TIME_ENABLE
		else if (cmd.command == CMD_NTP_SERVER)
		{
			str += REPLY_NTP_SERVER;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_NTP_TIME_ZONE)
		{
			int timeZone = cmd.arguments[0].toInt();
			str += REPLY_NTP_TIME_ZONE;
			str += eq;
			str += String(timeZone);
			str += QUOTE;
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
		}
		else if (cmd.command == CMD_NTP_REFRESH_DELAY)
		{
			ntpRefreshDelay = cmd.arguments[0].toInt();
			str += REPLY_NTP_REFRESH_DELAY;
			str += eq;
			str += String(ntpRefreshDelay);
			str += QUOTE;
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);
		}
		else if (cmd.command == CMD_NTP_REFRESH_PERIOD)
		{
			timeRefershPeriod = cmd.arguments[0].toInt();
			str += REPLY_NTP_REFRESH_PERIOD;
			str += eq;
			str += String(timeRefershPeriod);
			str += QUOTE;
			writeConfigString(NTP_REFRESH_PERIOD_addr, NTP_REFRESH_PERIOD_size, String(timeRefershPeriod));
		}
		else if (cmd.command == CMD_NTP_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_OFF_NUMBER);
				NTPenable = false;
				setSyncProvider(nullptr);
				Udp.stop();
				str += REPLY_NTP_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_ON_NUMBER);
				NTPenable = true;
				Udp.begin(UDP_LOCAL_PORT);
				setSyncProvider(getNtpTime);
				setSyncInterval(ntpRefreshDelay);
				str += REPLY_NTP_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef EVENTS_ENABLE
		else if (cmd.command == CMD_EVENT_SET)
		{
			if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
			{
				int t = command.indexOf('=');
				command = command.substring(t + 1);
				uint16_t m = EVENTS_TABLE_size / EVENTS_NUMBER;
				writeConfigString(EVENTS_TABLE_addr + cmd.index * m, m, command);
				str += REPLY_EVENT_SET;
				str += String(cmd.index);
				str += F(" is now: ");
				str += cmd.arguments[0];
			}
			else
			{
				str = F("Event #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(EVENTS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_EVENTS_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
				eventsEnable = false;
				str += REPLY_EVENTS_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
				eventsEnable = true;
				str += REPLY_EVENTS_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_EVENT_FLAG_SET)
		{
			if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
			{
				if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
				{
					writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
					eventsFlags[cmd.index] = false;
					str += REPLY_EVENT_FLAG_SET;
					str += String(cmd.index);
					str += F(" cleared");
				}
				else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
				{
					writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
					eventsFlags[cmd.index] = true;
					str += REPLY_EVENT_FLAG_SET;
					str += String(cmd.index);
					str += F(" set");
				}
			}
			else
			{
				str = F("Event #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(EVENTS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_HELP_EVENT)
		{
			str += printHelpEvent();
		}
#endif

#ifdef SCHEDULER_ENABLE
		else if (cmd.command == CMD_SCHEDULE_SET)
		{
			if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
			{
				int t = command.indexOf('=');
				command = command.substring(t + 1);
				uint16_t m = SCHEDULER_TABLE_size / SCHEDULES_NUMBER;
				writeConfigString(SCHEDULER_TABLE_addr + cmd.index * m, m, command);
				writeScheduleExecTime(cmd.index, 0);
				str += REPLY_SCHEDULE_SET;
				str += String(cmd.index);
				str += F(" is now: ");
				str += cmd.arguments[0];
			}
			else
			{
				str = F("Schedule #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(SCHEDULES_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_SCHEDULER_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_OFF_NUMBER);
				schedulerEnable = false;
				str += REPLY_SCHEDULER_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_ON_NUMBER);
				schedulerEnable = true;
				str += REPLY_SCHEDULER_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_CLEAR_SCHEDULE_EXEC_TIME)
		{
			if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
			{
				writeScheduleExecTime(cmd.index, 0);
				str += REPLY_CLEAR_SCHEDULE_EXEC_TIME;
				str += String(cmd.index);
				str += F(" reset");
			}
			else
			{
				str = F("Schedule #");
				str += String(cmd.index);
				str += F(" is out of range [0,");
				str += String(SCHEDULES_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_HELP_SCHEDULE)
		{
			str += printHelpSchedule();
		}
#endif

#if defined(EVENTS_ENABLE) || defined(SCHEDULER_ENABLE)
		else if (cmd.command == CMD_HELP_ACTION)
		{
			str += printHelpAction();
		}
#endif

#ifdef DISPLAY_ENABLED
		else if (cmd.command == CMD_DISPLAY_REFRESH)
		{
			displaySwitchPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
			str += REPLY_DISPLAY_REFRESH;
			str += eq;
			str += String(uint16_t(displaySwitchPeriod));
			str += QUOTE;
			writeConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size, String(uint16_t(displaySwitchPeriod / 1000UL)));
		}
#endif

#ifdef OTA_UPDATE
		else if (cmd.command == CMD_OTA_PORT)
		{
			uint16_t ota_port = cmd.arguments[0].toInt();
			str += REPLY_OTA_PORT;
			str += eq;
			str += String(ota_port);
			str += QUOTE;
			writeConfigString(OTA_PORT_addr, OTA_PORT_size, String(ota_port));
		}
		else if (cmd.command == CMD_OTA_PASSWORD)
		{
			str += REPLY_OTA_PASSWORD;
			str += eq;
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size, cmd.arguments[0]);
		}
		else if (cmd.command == CMD_OTA_ENABLE)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_OFF_NUMBER);
				otaEnable = false;
				str += REPLY_OTA_ENABLE;
				str += eq;
				str += REPLY_DISABLED;
				ArduinoOTA.begin();
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_ON_NUMBER);
				otaEnable = true;
				str += REPLY_OTA_ENABLE;
				str += eq;
				str += REPLY_ENABLED;
			}
			else
			{
				str = REPLY_INCORRECT_VALUE;
				str += cmd.arguments[0];
			}
		}
#endif
	}
	if (str.length() == 0)
	{
		str = REPLY_INCORRECT;
		str += F("command: \"");
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
		"send_mail=address,subject,message\r\n"
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
			action.clear();
		}
		commandTokens cmd = parseCommand(tmpAction, '=', ',', true);
		//command=****
		if (cmd.command == ACTION_COMMAND)
		{
			String tmpCmd = tmpAction.substring(sizeof(ACTION_COMMAND));
			processCommand(tmpCmd, CHANNEL_UART, true);
		}
		//set_output?=x
		else if (cmd.command == ACTION_SET_OUTPUT)
		{
			set_output(cmd.index, cmd.arguments[0]);
		}

		//reset_counter? - deprecated
		else if (cmd.command == ACTION_RESET_COUNTER)
		{
			if (cmd.index >= 0 && cmd.index < PIN_NUMBER)
			{
				InterruptCounter[cmd.index - 1] = 0;
			}
		}

		//set_counter?=x
		else if (cmd.command == ACTION_SET_COUNTER)
		{
			if (cmd.index >= 0 && cmd.index < PIN_NUMBER)
			{
				InterruptCounter[cmd.index - 1] = cmd.arguments[0].toInt();
			}
		}

#ifdef EVENTS_ENABLE
		//reset_flag?
		else if (cmd.command == ACTION_RESET_FLAG)
		{
			if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
			{
				eventsFlags[cmd.index] = false;
			}
		}
		//set_flag?
		else if (cmd.command == ACTION_SET_FLAG)
		{
			if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
			{
				eventsFlags[cmd.index] = true;
			}
		}
		//set_event_flag?=0/1/off/on
		else if (cmd.command == ACTION_SET_EVENT_FLAG)
		{
			if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
			{
				if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
				{
					writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
					eventsFlags[cmd.index] = false;
				}
				else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
				{
					writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
					eventsFlags[cmd.index] = true;
				}
			}
		}

#endif
#ifdef SCHEDULER_ENABLE
		//clear_schedule_exec_time?
		else if (cmd.command == ACTION_CLEAR_SCHEDULE_EXEC_TIME)
		{
			if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
			{
				writeScheduleExecTime(cmd.index, 0);
			}
		}
#endif
#ifdef GSM_ENABLE
		//send_sms=* / user#,message
		else if (gsmEnable && cmd.command == ACTION_SEND_SMS)
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (cmd.arguments[0] == STAR) //if * then all
			{
				i = 0;
				j = GSM_USERS_NUMBER;
			}
			else
			{
				i = cmd.arguments[0].toInt();
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
					tmpStr += COLON;
					tmpStr += cmd.arguments[1];
					tmpStr += EOL;
					tmpStr += parseSensorReport(sensorData, EOL);
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
		else if (telegramEnable && cmd.command == ACTION_SEND_TELEGRAM)
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (cmd.arguments[0] == STAR) //if * then all
			{
				i = 0;
				j = TELEGRAM_USERS_NUMBER;
			}
			else
			{
				i = cmd.arguments[0].toInt();
				j = i + 1;
			}
			sensorDataCollection sensorData = collectData();
			for (; i < j; i++)
			{
				uint64_t user = getTelegramUser(i);
				if (user > 0)
				{
					String tmpStr = deviceName;
					tmpStr += COLON;
					tmpStr += cmd.arguments[1];
					tmpStr += EOL;
					tmpStr += parseSensorReport(sensorData, EOL);
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
		else if (pushingBoxEnable && cmd.command == ACTION_SEND_PUSHINGBOX)
		{
			sensorDataCollection sensorData = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += cmd.arguments[0];
			tmpStr += EOL;
			tmpStr += parseSensorReport(sensorData, EOL);
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
		//send_mail=address_to,subject,message
		else if (smtpEnable && cmd.command == ACTION_SEND_MAIL)
		{
			sensorDataCollection sensorData = collectData();
			String tmpStr = cmd.arguments[2];
			tmpStr += EOL;
			tmpStr += parseSensorReport(sensorData, EOL);
			String subj = deviceName;
			subj += F(" alert");
			bool sendOk = sendMail(cmd.arguments[1], tmpStr, cmd.arguments[0]);
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
		else if (gScriptEnable && cmd.command == ACTION_SEND_GSCRIPT)
		{
			sensorDataCollection sensorData = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += cmd.arguments[0];
			tmpStr += EOL;
			tmpStr += parseSensorReport(sensorData, EOL);
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
		//send_MQTT=topic_to,message
		if (mqttEnable && cmd.command == ACTION_SEND_MQTT)
		{
			String delimiter = F("\",\r\n\"");
			String eq = F("\":\"");
			String tmpStr = F("{\"");
			tmpStr += F("Device name");
			tmpStr += eq;
			tmpStr += deviceName;
			tmpStr += delimiter;
			tmpStr += F("Message");
			tmpStr += eq;
			tmpStr += cmd.arguments[1];
			tmpStr += delimiter;
			tmpStr += F("\"}");

			bool sendOk = false;
			sendOk = mqtt_send(tmpStr, tmpStr.length(), cmd.arguments[0]);
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
		else if (cmd.command == ACTION_SAVE_LOG)
		{
			sensorDataCollection sensorData = collectData();
			addToLog(sensorData);
		}
#endif
		else
		{
#ifdef DEBUG_MODE
			Serial.print(F("Incorrect action: \""));
			Serial.print(tmpAction);
			Serial.println(F("\""));
#endif
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
	if (dht_enable)
		temp = sensorData.dht_temp;
#endif

#ifdef BMP180_ENABLE
	if (bmp180_enable)
		temp = sensorData.bmp180_temp;
#endif

#ifdef BME280_ENABLE
	if (bme280_enable)
		temp = sensorData.bme280_temp;
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable)
		temp = sensorData.htu21d_temp;
#endif

#ifdef AHTx0_ENABLE
	if (ahtx0_enable)
		temp = sensorData.ahtx0_temp;
#endif

#ifdef AMS2320_ENABLE
	if (am2320_enable)
		temp = sensorData.ams_temp;
#endif

#ifdef DS18B20_ENABLE
	if (ds1820_enable)
		temp = sensorData.ds1820_temp;
#endif
	return temp;
}
#endif

#ifdef HUMIDITY_SENSOR
float getHumidity(sensorDataCollection& sensorData)
{
	float humidity = -1000;
#ifdef DHT_ENABLE
	if (dht_enable)
		humidity = sensorData.dht_humidity;
#endif

#ifdef AMS2320_ENABLE
	if (am2320_enable)
		humidity = round(sensorData.ams_humidity);
#endif

#ifdef HTU21D_ENABLE
	if (htu21d_enable)
		humidity = sensorData.htu21d_humidity;
#endif

#ifdef AHTx0_ENABLE
	if (ahtx0_enable)
		humidity = sensorData.ahtx0_humidity;
#endif

#ifdef BME280_ENABLE
	if (bme280_enable)
		humidity = sensorData.bme280_humidity;
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
	if (AP_Ssid.length() <= 0)
		AP_Ssid = deviceName + F("_") + CompactMac();
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
	if (power < 0 || power > 20.5)
		power = 20.5f;
	return power;
}

void startWiFi()
{
	wifi_mode = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (wifi_mode < 0 || wifi_mode > 4)
		wifi_mode = WIFI_MODE_AUTO;

	if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AUTO)
	{
		Start_STA_Mode();
		wifiIntendedStatus = RADIO_CONNECTING;
	}
	else if (wifi_mode == WIFI_MODE_AP)
	{
		Start_AP_Mode();
		wifiIntendedStatus = RADIO_WAITING;
	}
	else if (wifi_mode == WIFI_MODE_AP_STA)
	{
		Start_AP_STA_Mode();
		wifiIntendedStatus = RADIO_CONNECTING;
	}
	else
	{
		Start_OFF_Mode();
		wifiIntendedStatus = RADIO_STOP;
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
	float wifi_OutputPower = getWiFiPower();	   //[0 - 20.5]dBm

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
	float wifi_OutputPower = getWiFiPower();	   //[0 - 20.5]dBm

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
	float wifi_OutputPower = getWiFiPower();	   //[0 - 20.5]dBm

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

String MacToStr(const uint8_t mac[6])
{
	return String(mac[0]) + String(mac[1]) + String(mac[2]) + String(mac[3]) + String(mac[4]) + String(mac[5]);
}

String CompactMac()
{
	String mac = WiFi.macAddress();
	mac.replace(COLON, F(""));
	return mac;
}

/*uint16_t countOf(String &str, char c)
{
	uint16_t count = 0;
	for (int i = 0; i < str.length(); i++)
	{
		if (str[i] == c)
			count++;
		yield();
	}
	return count;
}*/
