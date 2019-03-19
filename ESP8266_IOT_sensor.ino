/*
GPIO 16		 D0 - WAKE, USER button
GPIO 5		 D1 - I2C_SCL
GPIO 4		~D2 - I2C_SDA
GPIO 0		 D3 - pullup, FLASH, TM1637_CLK
GPIO 2		 D4 - pullup, Serial1_TX, TM1637_DIO
GPIO 14		~D5 - SoftUART_TX
GPIO 12		~D6 - SoftUART_RX
GPIO 13		 D7 -
GPIO 15		~D8 - pulldown
GPIO 13		 A7 - analog in
*/

/* Untested feature:
 - keep initial output values in FLASH and restore at startup
 - don't keep events/schedules in RAM
 - set pwm output
 - select interrupt mode (FALLING/RISING/CHANGE)
 - set_time=yyyy.mm.dd hh:mm:ss
 - event action:
	conditions:
		input?=on,off,x,c
		output?=on,off,x,c
		counter>x
		adc<>x
		temperature<>x
		humidity<>x
		co2<>x
	actions:
		set_output?=on,off,x
		send_telegram=* / user#;message
		send_PushingBox=message
		send_mail=address;message
		send_GScript
		save_log;
*/

/*
Planned features:
 - TELEGRAM/PushingBox/E-Mail message limit management
 - SCHEDULE service
 - GoogleDocs service
 - efficient sleep till next event planned with network connectivity restoration after wake up and parameter / timers restore to fire.
 - compile time pin arrangement control
*/

/*
Sensors to be supported:
 - BME280
 - ACS712(A0)
 - GPRS module via UART/SoftUART(TX_pin, RX_pin, baud_rate); send SMS, make call
*/

#define SLEEP_ENABLE //connect D0 and EN pins to start contoller after sleep
#define NTP_TIME_ENABLE
#define HTTP_SERVER_ENABLE
#define EVENTS_ENABLE
#define SCHEDULER_ENABLE

#define AMS2320_ENABLE
#define HTU21D_ENABLE
#define DS18B20_ENABLE
#define DHT_ENABLE
#define MH_Z19_UART_ENABLE
#define MH_Z19_PPM_ENABLE
#define TM1637DISPLAY_ENABLE

#define TELEGRAM_ENABLE
#define PUSHINGBOX
#define EMAIL_ENABLE
#define GSCRIPT

#define ADC_ENABLE

#define INTERRUPT_COUNTER1_ENABLE 5		// D1 - I2C_SCL
#define INTERRUPT_COUNTER2_ENABLE 4		//~D2 - I2C_SDA
#define INTERRUPT_COUNTER3_ENABLE 0		// D3 - FLASH, TM1637_CLK
#define INTERRUPT_COUNTER4_ENABLE 2		// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
#define INTERRUPT_COUNTER5_ENABLE 14	//~D5 - SoftUART_TX
#define INTERRUPT_COUNTER6_ENABLE 12	//~D6 - SoftUART_RX
#define INTERRUPT_COUNTER7_ENABLE 13	// D7
#define INTERRUPT_COUNTER8_ENABLE 15	//~D8 - keep LOW on boot

#define INPUT1_ENABLE 5					// D1 - I2C_SCL
#define INPUT2_ENABLE 4					//~D2 - I2C_SDA
#define INPUT3_ENABLE 0					// D3 - FLASH, TM1637_CLK
#define INPUT4_ENABLE 2					// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
#define INPUT5_ENABLE 14				//~D5 - SoftUART_TX
#define INPUT6_ENABLE 12				//~D6 - SoftUART_RX
#define INPUT7_ENABLE 13				// D7
#define INPUT8_ENABLE 15				//~D8 - keep LOW on boot

#define OUTPUT1_ENABLE 5				// D1 - I2C_SCL
#define OUTPUT2_ENABLE 4				//~D2 - I2C_SDA
#define OUTPUT3_ENABLE 0				// D3 - FLASH, TM1637_CLK
#define OUTPUT4_ENABLE 2				// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
#define OUTPUT5_ENABLE 14				//~D5 - SoftUART_TX
#define OUTPUT6_ENABLE 12				//~D6 - SoftUART_RX
#define OUTPUT7_ENABLE 13				// D7
#define OUTPUT8_ENABLE 15				//~D8 - keep LOW on boot

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include "datastructures.h"

// WiFi settings
#define MAX_SRV_CLIENTS 5 //how many clients should be able to telnet to this ESP8266
String ssid;
String WiFiPass;
unsigned int telnetPort = 23;
String netInput[MAX_SRV_CLIENTS];
byte WiFiIntendedStatus = WIFI_STOP;
WiFiServer telnetServer(telnetPort);
WiFiClient serverClient[MAX_SRV_CLIENTS];
bool wifiEnable = true;
bool telnetEnable = true;
String deviceName = "";

unsigned long checkSensorLastTime = 0;
unsigned long checkSensorPeriod = 5000;

String uartCommand;
bool uartReady = false;

// Log settings for 32 hours of storage
#define LOG_SIZE 192 // Save sensors value each 10 minutes: ( 32hours * 60minutes/hour ) / 10minutes/pcs = 192 pcs
sensorDataCollection history_log[LOG_SIZE];
unsigned int history_record_number = 0;
unsigned long logLastTime = 0;
unsigned long logPeriod = ((60 / (LOG_SIZE / 32)) * 60 * 1000) / 5; // 60minutes/hour / ( 192pcs / 32hours ) * 60seconds/minute * 1000 msec/second
sensorDataCollection sensor;

bool isTimeSet = false;
bool autoReport = false;

// not implemented yet
#ifdef SLEEP_ENABLE
unsigned long reconnectTimeOut = 5000;
bool SleepEnable = false;
#endif

// not implemented yet
const byte schedulesNumber = 10;
#ifdef SCHEDULER_ENABLE
//String schedulesActions[schedulesNumber];
bool schedulesFlags[schedulesNumber];
bool SchedulerEnable = false;
#endif

// not tested
const byte eventsNumber = 10;
#ifdef EVENTS_ENABLE
//String eventsActions[eventsNumber];
bool eventsFlags[eventsNumber];
bool EventsEnable = false;
#endif

// E-mail config
// Need smtp parameters, credentials
#ifdef EMAIL_ENABLE
#include <sendemail.h>
String smtpServerAddress = "";
unsigned int smtpServerPort = 2525;
String smtpLogin = "";
String smtpPassword = "";
String smtpTo = "";
bool EMailEnable = false;

bool sendMail(String address, String subject, String message)
{
	SendEmail e(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 5000, false);
	return e.send(smtpLogin, address, subject, message);
}

#endif

// Telegram config
// Need token
const byte telegramUsersNumber = 10;
#ifdef TELEGRAM_ENABLE
//#include <WiFiClientSecure.h>
#include "CTBot.h"
CTBot myBot;
typedef struct telegramMessage
{
	byte retries;
	int64_t user;
	String message;
};

String telegramToken = "";
int64_t telegramUsers[telegramUsersNumber];
const byte telegramMessageBufferSize = 10;
const int telegramMessageMaxSize = 900;
telegramMessage telegramOutboundBuffer[telegramMessageBufferSize];
byte telegramOutboundBufferPos = 0;
int telegramMessageDelay = 2000;
byte telegramRetries = 3;
unsigned long telegramLastTime = 0;
bool TelegramEnable = false;

void addMessageToTelegramOutboundBuffer(int64_t user, String message, byte retries)
{
	// slice message to pieces
	while (message.length() > telegramMessageMaxSize)
	{
		addMessageToTelegramOutboundBuffer(user, message.substring(0, telegramMessageMaxSize - 1), retries);
		message = message.substring(telegramMessageMaxSize);
	}

	if (telegramOutboundBufferPos >= telegramMessageBufferSize - 1)
	{
		for (int i = 0; i < telegramMessageBufferSize - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		}
	}
	telegramOutboundBuffer[telegramOutboundBufferPos].user = user;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = message;
	telegramOutboundBuffer[telegramOutboundBufferPos].retries = retries;
	telegramOutboundBufferPos++;
}

void removeMessageFromTelegramOutboundBuffer(byte messageNumber)
{
	for (int i = messageNumber; i < telegramMessageBufferSize - 1; i++)
	{
		if (telegramOutboundBuffer[i + 1].message != "") telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
	}
	telegramOutboundBufferPos--;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = "";
	telegramOutboundBuffer[telegramOutboundBufferPos].user = 0;
}

void sendBufferToTelegram()
{
	if (TelegramEnable && WiFi.status() == WL_CONNECTED && myBot.testConnection())
	{
		bool isRegisteredUser = false;
		bool isAdmin = false;
		TBMessage msg;
		while (myBot.getNewMessage(msg))
		{
			Serial.println("TELEGRAM message [" + msg.sender.username + "]: " + msg.text);

			//assign 1st sender as admin if admin is empty
			if (telegramUsers[0] == 0)
			{
				telegramUsers[0] = msg.sender.id;
				Serial.println("admin set to [" + uint64ToString(telegramUsers[0]) + "]");
			}

			//check if it's registered user
			for (int i = 0; i < telegramUsersNumber; i++)
			{
				if (telegramUsers[i] == msg.sender.id)
				{
					isRegisteredUser = true;
					if (i == 0) isAdmin = true;
					break;
				}
			}

			//process data from TELEGRAM
			if (isRegisteredUser && msg.text != "")
			{
				String str = deviceName + ">> " + msg.text;
				Serial.println("registered user message: " + str);
				addMessageToTelegramOutboundBuffer(msg.sender.id, deviceName + ": " + str, telegramRetries);
				str = processCommand(msg.text, TELEGRAM_CHANNEL, isAdmin);
				Serial.println("Command answer: " + str);
				addMessageToTelegramOutboundBuffer(msg.sender.id, deviceName + ": " + str, telegramRetries);
			}
			yield();
		}

		if (telegramOutboundBuffer[0].message == "")
		{
			return;
		}

		if (sendToTelegram(telegramOutboundBuffer[0].user, telegramOutboundBuffer[0].message))
		{
			removeMessageFromTelegramOutboundBuffer(0);
		}
		else
		{
			telegramOutboundBuffer[0].retries--;
			if (telegramOutboundBuffer[0].retries <= 0)
			{
				removeMessageFromTelegramOutboundBuffer(0);
			}
		}
	}
}

bool sendToTelegram(int64_t user, String str)
{
	bool result = true;
	if (WiFi.status() == WL_CONNECTED)
	{
		result = myBot.testConnection();
		if (result)
		{
			str.replace("\r\n", "; ");
			result = myBot.sendMessage(user, str);
			if (!result) Serial.println("\nError sending to TELEGRAM: " + str);
		}
		else Serial.println("TELEGRAM connection error");
	}
	else Serial.println("\nWiFi not connected");
	return result;
}

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
	} while (input);
	return result;
}

uint64_t StringToUint64(String value)
{
	uint64_t result = 0;

	const char* p = value.c_str();
	const char* q = p + value.length();
	while (p < q)
	{
		result = (result << 1) + (result << 3) + *(p++) - '0';
	}
	return result;
}
#endif

// not implemented yet
// Google script config
// Need token
#ifdef GSCRIPT
#include <HTTPSRedirect.h>
String GScriptId = "";
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";
String url = String("/macros/s/") + GScriptId + "/exec";
//String[] valuesNames = {"?tag", "?value="};

const int httpsPort = 443;
HTTPSRedirect gScriptClient(httpsPort);
bool GScriptEnable = false;

bool sendValueToGoogle(String value)
{
	HTTPSRedirect* client = nullptr;
	client = new HTTPSRedirect(httpsPort);
	client->setInsecure();
	client->setPrintResponseBody(true);
	client->setContentTypeHeader("application/json");

	bool flag = false;
	for (int i = 0; i < 5; i++)
	{
		int retval = client->connect(host, httpsPort);
		if (retval == 1)
		{
			flag = true;
			break;
		}
		else Serial.println("Connection failed. Retrying...");
	}

	if (!flag)
	{
		Serial.print("Could not connect to server: ");
		Serial.println(host);
		Serial.println("Exiting...");
		return flag;
	}

	if (client->setFingerprint(fingerprint))
	{
		Serial.println("Certificate match.");
	}
	else
	{
		Serial.println("Certificate mis-match");
	}

	// Send data to Google Sheets
	flag = client->POST(url, host, value, false);
	delete client;
	client = nullptr;
	return flag;
}
#endif

// Pushingbox config
// PushingBox device ID
#ifdef PUSHINGBOX
String pushingBoxId = "";
String pushingBoxParameter = "";
const int pushingBoxMessageMaxSize = 900;
const char* pushingBoxServer = "api.pushingbox.com";
bool PushingBoxEnable = false;

bool sendToPushingBox(String message)
{
	// slice message to pieces
	while (message.length() > pushingBoxMessageMaxSize)
	{
		sendToPushingBox(message.substring(0, pushingBoxMessageMaxSize - 1));
		message = message.substring(pushingBoxMessageMaxSize);
	}

	WiFiClient client;
	bool flag = true;
	//Serial.println("- connecting to pushing server ");
	if (client.connect(pushingBoxServer, 80))
	{
		//Serial.println("- succesfully connected");

		String postStr = "devid=";
		postStr += pushingBoxId;
		postStr += "&" + pushingBoxParameter + "=";
		postStr += message;
		postStr += "\r\n\r\n";

		//Serial.println("- sending data...");

		client.print("POST /pushingbox HTTP/1.1\n");
		client.print("Host: " + String(pushingBoxServer) + "\n");
		client.print("Connection: close\n");
		client.print("Content-Type: application/x-www-form-urlencoded\n");
		client.print("Content-Length: ");
		client.print(postStr.length());
		client.print("\n\n");
		client.print(postStr);
		//Serial.println(postStr);
		if (client.available())
		{
			String reply = client.readString();
			Serial.println("PushingBox answer: " + reply);
			if (reply != "200 OK") flag = false;
		}
	}
	else flag = false;
	client.stop();
	//Serial.println("- stopping the client");
	return flag;
}
#endif

// HTTP server
// Need port 
#ifdef HTTP_SERVER_ENABLE
#include <ESP8266WebServer.h>
unsigned int httpPort = 80;
ESP8266WebServer http_server(httpPort);
bool httpServerEnable = false;

void handleRoot()
{
	String str = ParseSensorReport(history_log[history_record_number - 1], "<BR>");
	http_server.sendContent(HTML_header() + str + HTML_footer());
}

void handleNotFound()
{
	String message = "File Not Found\n\nURI: ";
	message += http_server.uri();
	message += "\nMethod: ";
	message += (http_server.method() == HTTP_GET) ? "GET" : "POST\nArguments: ";
	message += http_server.args();
	message += "\n";
	for (uint8_t i = 0; i < http_server.args(); i++)
	{
		message += " " + http_server.argName(i) + ": " + http_server.arg(i) + "\n";
		yield();
	}
	http_server.send(404, "text/plain", message);
}

String HTML_header()
{
	return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: " + String(round(checkSensorPeriod / 1000)) + "\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
}

String HTML_footer()
{
	return "<br />\r\n</html>\r\n";
}

#endif

// NTP Server
// Need server address, local time zone
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>
IPAddress timeServer(129, 6, 15, 28); //129.6.15.28 = time-a.nist.gov
WiFiUDP Udp;
int timeZone = +3;     // Moscow Time
const unsigned int localPort = 8888;  // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
int ntpRefreshDelay = 180;
bool NTPenable = false;

time_t getNtpTime()
{
	if (!NTPenable || WiFi.status() != WL_CONNECTED) return 0;

	while (Udp.parsePacket() > 0) yield(); // discard any previously received packets
	Serial.println("Transmit NTP Request");
	sendNTPpacket(timeServer);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			//Serial.println("Receive NTP Response");
			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			isTimeSet = true;
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
	}
	Serial.println("No NTP Response");
	return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
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
#endif

/*
AM2320 I2C temperature + humidity sensor
PINS: D1 - SCL, D2 - SDA
pin 1 of the sensor to +3.3V/+5V
pin 2 of the sensor to SDA
pin 3 of the sensor to GND
pin 4 of the sensor to SCL
5-10K pullup resistor from pin 2 (SDA) to pin 1 (power) of the sensor
5-10K pullup resistor from pin 4 (SCL) to pin 1 (power) of the sensor
*/
#ifdef AMS2320_ENABLE
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
Adafruit_AM2320 am2320 = Adafruit_AM2320();
#endif

// HTU21D I2C temperature + humidity sensor
// PINS: D1 - SCL, D2 - SDA
#ifdef HTU21D_ENABLE
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
Adafruit_HTU21DF htu21d = Adafruit_HTU21DF();
#endif

/*
DS18B20 OneWire temperature sensor
PINS: D3 - OneWire
pin 1 to +3.3V/+5V
pin 2 to whatever your OneWire is
pin 3 to GROUND
5-10K pullup resistor from pin 2 (data) to pin 1 (power) of the sensor
*/
#ifdef DS18B20_ENABLE
#include <OneWire.h>
#include "DallasTemperature.h"
#define ONE_WIRE_PIN 5 //D1
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds1820(&oneWire);
#endif

/*
DHT11/DHT21(AM2301)/DHT22(AM2302, AM2321) temperature + humidity sensor
PINS: D3 - DATA
pin 1 of the sensor to +3.3V/+5V
pin 2 of the sensor to whatever your DHTPIN is
pin 4 of the sensor to GROUND
5-10K pullup resistor from pin 2 (data) to pin 1 (power) of the sensor
*/
#ifdef DHT_ENABLE
#include "DHT.h"
#define DHTTYPE DHT11 //DHT11, DHT21, DHT22 
#define DHTPIN 0 //D3
DHT dht(DHTPIN, DHTTYPE);
#endif

// MH-Z19 CO2 sensor via UART
// PINS: D5 - TX, D6 - RX
#ifdef MH_Z19_UART_ENABLE
#include <SoftwareSerial.h>;
#define MHZ19_TX 14 //D5
#define MHZ19_RX 12 //D6
SoftwareSerial co2sensor(MHZ19_TX, MHZ19_RX, false, 32);
unsigned int co2_uart_avg[3] = { 0, 0, 0 };
int mhtemp_s = 0;

unsigned int co2SerialRead()
{
	byte cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
	unsigned char response[9];
	byte crc = 0;
	unsigned int ppm = 0;
	mhtemp_s = 0;

	co2sensor.write(cmd, 9);
	memset(response, 0, 9);
	co2sensor.readBytes(response, 9);
	crc = 0;
	for (int myVal = 1; myVal < 8; myVal++) crc += response[myVal];
	crc = 255 - crc;
	crc++;
	if (response[0] == 0xFF && response[1] == 0x86 && response[8] == crc)
	{
		unsigned int responseHigh = (unsigned int)response[2];
		unsigned int responseLow = (unsigned int)response[3];
		ppm = (256 * responseHigh) + responseLow;
		mhtemp_s = int(response[4]) - 40;
	}
	else
	{
		Serial.println("CRC error: " + String(crc) + " / " + String(response[8]));
		ppm = 0;
	}
	return ppm;
}
#endif

// MH-Z19 CO2 sensor via PPM signal
// PINS: D7 - PPM
#ifdef MH_Z19_PPM_ENABLE
#define MHZ19_PPM 13 //D7
unsigned int co2_ppm_avg[3] = { 0, 0, 0 };

unsigned int co2PPMRead(byte pin)
{
	unsigned int th, tl, ppm;
	long timeout = 1000;
	long timer = millis();
	do
	{
		th = pulseIn(pin, HIGH, 1004000) / 1000;
		tl = 1004 - th;
		ppm = 5000 * (th - 2) / (th + tl - 4);
		if (millis() - timer > timeout)
		{
			Serial.println("Failed to read PPM");
			return 0;
		}
		yield();
	} while (th == 0);
	return ppm;
}
#endif

/* TM1637 display.
TM1637 display leds layout:
	  a
	  _
	f| |b
	  -g
	e|_|c
	  d
PINS: D3 - CLK, D4 - DIO
*/
#ifdef TM1637DISPLAY_ENABLE
#include <TM1637Display.h>
#define TM1637_CLK 0 //D3
#define TM1637_DIO 2 //D4
const uint8_t SEG_DEGREE[] = { SEG_A | SEG_B | SEG_F | SEG_G };
const uint8_t SEG_HUMIDITY[] = { SEG_C | SEG_E | SEG_F | SEG_G };
TM1637Display display(TM1637_CLK, TM1637_DIO);
unsigned long displaySwitchedLastTime = 0;
unsigned long displaySwitchPeriod = 3000;
byte sw = 0;
#endif

#ifdef ADC_ENABLE
//ADC_MODE(ADC_VCC);

int getAdc()
{
	float AverageValue = 0;
	int MeasurementsToAverage = 16;
	for (int i = 0; i < MeasurementsToAverage; ++i)
	{
		AverageValue += (float)analogRead(A0);
		delay(1);
	}
	AverageValue /= MeasurementsToAverage;
	return int(AverageValue);
}
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
unsigned long int intCount1 = 0;
byte intMode1 = FALLING;
void int1count()
{
	intCount1++;
}
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
unsigned long int intCount2 = 0;
byte intMode2 = FALLING;
void int2count()
{
	intCount2++;
}
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
unsigned long int intCount3 = 0;
byte intMode3 = FALLING;
void int3count()
{
	intCount3++;
}
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
unsigned long int intCount4 = 0;
byte intMode4 = FALLING;
void int4count()
{
	intCount4++;
}
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
unsigned long int intCount5 = 0;
byte intMode5 = FALLING;
void int5count()
{
	intCount5++;
}
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
unsigned long int intCount6 = 0;
byte intMode6 = FALLING;
void int6count()
{
	intCount6++;
}
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
unsigned long int intCount7 = 0;
byte intMode7 = FALLING;
void int7count()
{
	intCount7++;
}
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
unsigned long int intCount8 = 0;
byte intMode8 = FALLING;
void int8count()
{
	intCount8++;
}
#endif

#ifdef INPUT1_ENABLE
bool in1;
#endif
#ifdef INPUT2_ENABLE
bool in2;
#endif
#ifdef INPUT3_ENABLE
bool in3;
#endif
#ifdef INPUT4_ENABLE
bool in4;
#endif
#ifdef INPUT5_ENABLE
bool in5;
#endif
#ifdef INPUT6_ENABLE
bool in6;
#endif
#ifdef INPUT7_ENABLE
bool in7;
#endif
#ifdef INPUT8_ENABLE
bool in8;
#endif

#ifdef OUTPUT1_ENABLE
bool out1 = OUT_OFF;
#endif
#ifdef OUTPUT2_ENABLE
bool out2 = OUT_OFF;
#endif
#ifdef OUTPUT3_ENABLE
bool out3 = OUT_OFF;
#endif
#ifdef OUTPUT4_ENABLE
bool out4 = OUT_OFF;
#endif
#ifdef OUTPUT5_ENABLE
bool out5 = OUT_OFF;
#endif
#ifdef OUTPUT6_ENABLE
bool out6 = OUT_OFF;
#endif
#ifdef OUTPUT7_ENABLE
bool out7 = OUT_OFF;
#endif
#ifdef OUTPUT8_ENABLE
bool out8 = OUT_OFF;
#endif

void setup()
{
	//WiFi.setPhyMode(WIFI_PHY_MODE_11B); //WIFI_PHY_MODE_11B = 1 (60-215mA); WIFI_PHY_MODE_11G = 2 (145mA); WIFI_PHY_MODE_11N = 3 (135mA)
	//WiFi.setOutputPower(); //[0 - 20.5]dBm
	Serial.begin(115200);

	//init EEPROM of certain size
	int eeprom_size = CollectEepromSize();
	EEPROM.begin(eeprom_size + 100);

	//read config from EEPROM (FLASH)
	ssid = readConfigString(SSID_addr, SSID_size);
	WiFiPass = readConfigString(PASS_addr, PASS_size);
	telnetPort = readConfigString(TELNET_PORT_addr, TELNET_PORT_size).toInt();
	checkSensorPeriod = readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt();
	deviceName = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);

	// init WiFi
	WiFi.begin(ssid.c_str(), WiFiPass.c_str());
	WiFiIntendedStatus = WIFI_CONNECTING;
	Serial.println("\r\nConnecting to " + ssid);
	telnetServer.begin(telnetPort);
	telnetServer.setNoDelay(true);

#ifdef SLEEP_ENABLE
	if (readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size) == "1") SleepEnable = true;
#endif

#ifdef EVENTS_ENABLE
	/*int a = EVENTS_TABLE_size / eventsNumber;
	for (int i = 0; i < eventsNumber; i++)
	{
		eventsActions[i] = readConfigString((EVENTS_TABLE_addr + i * a), a);
	}*/
	if (readConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size) == "1") EventsEnable = true;
#endif

#ifdef SCHEDULER_ENABLE
	/*int b = SCHEDULER_TABLE_size / schedulesNumber;
	for (int i = 0; i < schedulesNumber; i++)
	{
		schedulesActions[i] = readConfigString((TELEGRAM_USERS_TABLE_addr + i * b), b);
	}*/
	if (readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size) == "1") SchedulerEnable = true;
#endif

#ifdef EMAIL_ENABLE
	smtpServerAddress = readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	smtpServerPort = readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size).toInt();
	smtpLogin = readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	smtpPassword = readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	smtpTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);
	if (readConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size) == "1") EMailEnable = true;
#endif

#ifdef TELEGRAM_ENABLE
	telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	for (int i = 0; i < telegramUsersNumber; i++)
	{
		telegramUsers[i] = StringToUint64(readConfigString((TELEGRAM_USERS_TABLE_addr + i * m), m));
	}
	if (readConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size) == "1") TelegramEnable = true;
	if (TelegramEnable)
	{
		//myBot.wifiConnect(ssid, WiFiPass);
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif

#ifdef GSCRIPT
	String GScriptId = readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	if (readConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size) == "1") GScriptEnable = true;
#endif

#ifdef PUSHINGBOX
	pushingBoxId = readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
	pushingBoxParameter = readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	if (readConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size) == "1") PushingBoxEnable = true;
#endif

#ifdef HTTP_SERVER_ENABLE
	httpPort = readConfigString(HTTP_PORT_addr, HTTP_PORT_size).toInt();
	if (readConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size) == "1") httpServerEnable = true;
	http_server.on("/", handleRoot);
	/*http_server.on("/inline", []()
	  {
	  http_server.send(200, "text/plain", "this works as well");
	  });*/
	http_server.onNotFound(handleNotFound);
	if (httpServerEnable) http_server.begin(httpPort);
#endif

#ifdef NTP_TIME_ENABLE
	timeServer.fromString(readConfigString(NTP_SERVER_addr, NTP_SERVER_size));
	timeZone = readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size).toInt();
	ntpRefreshDelay = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt();
	if (readConfigString(ENABLE_NTP_addr, ENABLE_NTP_size) == "1") NTPenable = true;
	if (NTPenable)
	{
		Udp.begin(localPort);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
	}
#endif

#ifdef AMS2320_ENABLE
	if (!am2320.begin())
	{
		Serial.println("Couldn't find AMS2320 sensor!");
	}
#endif

#ifdef HTU21D_ENABLE
	if (!htu21d.begin())
	{
		Serial.println("Couldn't find HTU21D sensor!");
	}
#endif

#ifdef DS18B20_ENABLE
	ds1820.begin();
#endif

#ifdef DHT_ENABLE
	dht.begin();
#endif

#ifdef MH_Z19_UART_ENABLE
	co2sensor.begin(9600);
	co2_uart_avg[0] = co2_uart_avg[1] = co2_uart_avg[2] = co2SerialRead();
#endif

#ifdef MH_Z19_PPM_ENABLE
	pinMode(MHZ19_PPM, INPUT);
	co2_ppm_avg[0] = co2_ppm_avg[1] = co2_ppm_avg[2] = co2PPMRead(MHZ19_PPM);
#endif

#ifdef TM1637DISPLAY_ENABLE
	displaySwitchPeriod = readConfigString(TM1637DISPLAY_REFRESH_addr, TM1637DISPLAY_REFRESH_size).toInt();
	display.setBrightness(1);
	display.showNumberDec(0, false);
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	intMode1 = readConfigString(INT1_MODE_addr, INT1_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER1_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER1_ENABLE, int1count, intMode1);
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	intMode2 = readConfigString(INT2_MODE_addr, INT2_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER2_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER2_ENABLE, int2count, intMode2);
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	intMode3 = readConfigString(INT3_MODE_addr, INT3_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER3_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER3_ENABLE, int3count, intMode3);
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	intMode4 = readConfigString(INT4_MODE_addr, INT4_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER4_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER4_ENABLE, int4count, intMode4);
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	intMode5 = readConfigString(INT5_MODE_addr, INT5_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER5_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER5_ENABLE, int5count, intMode5);
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	intMode6 = readConfigString(INT6_MODE_addr, INT6_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER6_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER6_ENABLE, int6count, intMode6);
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	intMode7 = readConfigString(INT7_MODE_addr, INT7_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER7_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER7_ENABLE, int7count, intMode7);
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	intMode8 = readConfigString(INT8_MODE_addr, INT8_MODE_size).toInt();
	pinMode(INTERRUPT_COUNTER8_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER8_ENABLE, int8count, intMode8);
#endif

#ifdef INPUT1_ENABLE
	pinMode(INPUT1_ENABLE, INPUT_PULLUP);
	in1 = digitalRead(INPUT1_ENABLE);
#endif
#ifdef INPUT2_ENABLE
	pinMode(INPUT2_ENABLE, INPUT_PULLUP);
	in2 = digitalRead(INPUT2_ENABLE);
#endif
#ifdef INPUT3_ENABLE
	pinMode(INPUT3_ENABLE, INPUT_PULLUP);
	in3 = digitalRead(INPUT3_ENABLE);
#endif
#ifdef INPUT4_ENABLE
	pinMode(INPUT4_ENABLE, INPUT_PULLUP);
	in4 = digitalRead(INPUT4_ENABLE);
#endif
#ifdef INPUT5_ENABLE
	pinMode(INPUT5_ENABLE, INPUT_PULLUP);
	in5 = digitalRead(INPUT5_ENABLE);
#endif
#ifdef INPUT6_ENABLE
	pinMode(INPUT6_ENABLE, INPUT_PULLUP);
	in6 = digitalRead(INPUT6_ENABLE);
#endif
#ifdef INPUT7_ENABLE
	pinMode(INPUT7_ENABLE, INPUT_PULLUP);
	in7 = digitalRead(INPUT7_ENABLE);
#endif
#ifdef INPUT8_ENABLE
	pinMode(INPUT8_ENABLE, INPUT_PULLUP);
	in8 = digitalRead(INPUT8_ENABLE);
#endif

#ifdef OUTPUT1_ENABLE
	out1 = readConfigString(OUT1_INIT_addr, OUT1_INIT_size).toInt();
	pinMode(OUTPUT1_ENABLE, OUTPUT);
	digitalWrite(OUTPUT1_ENABLE, out1);
#endif
#ifdef OUTPUT2_ENABLE
	out2 = readConfigString(OUT2_INIT_addr, OUT2_INIT_size).toInt();
	pinMode(OUTPUT2_ENABLE, OUTPUT);
	digitalWrite(OUTPUT2_ENABLE, out2);
#endif
#ifdef OUTPUT3_ENABLE
	out3 = readConfigString(OUT3_INIT_addr, OUT3_INIT_size).toInt();
	pinMode(OUTPUT3_ENABLE, OUTPUT);
	digitalWrite(OUTPUT3_ENABLE, out3);
#endif
#ifdef OUTPUT4_ENABLE
	out4 = readConfigString(OUT4_INIT_addr, OUT4_INIT_size).toInt();
	pinMode(OUTPUT4_ENABLE, OUTPUT);
	digitalWrite(OUTPUT4_ENABLE, out4);
#endif
#ifdef OUTPUT5_ENABLE
	out5 = readConfigString(OUT5_INIT_addr, OUT5_INIT_size).toInt();
	pinMode(OUTPUT5_ENABLE, OUTPUT);
	digitalWrite(OUTPUT5_ENABLE, out5);
#endif
#ifdef OUTPUT6_ENABLE
	out6 = readConfigString(OUT6_INIT_addr, OUT6_INIT_size).toInt();
	pinMode(OUTPUT6_ENABLE, OUTPUT);
	digitalWrite(OUTPUT6_ENABLE, out6);
#endif
#ifdef OUTPUT7_ENABLE
	out7 = readConfigString(OUT7_INIT_addr, OUT7_INIT_size).toInt();
	pinMode(OUTPUT7_ENABLE, OUTPUT);
	digitalWrite(OUTPUT7_ENABLE, out7);
#endif
#ifdef OUTPUT8_ENABLE
	out8 = readConfigString(OUT8_INIT_addr, OUT8_INIT_size).toInt();
	pinMode(OUTPUT8_ENABLE, OUTPUT);
	digitalWrite(OUTPUT8_ENABLE, out8);
#endif
}

void loop()
{
	//check if connection is present and reconnect
	if (WiFiIntendedStatus != WIFI_STOP)
	{
		unsigned long reconnectTimeStart = millis();
		if (WiFiIntendedStatus == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED)
		{
			Serial.println("Connection lost.");
			WiFiIntendedStatus = WIFI_CONNECTING;
		}
		if (WiFiIntendedStatus != WIFI_CONNECTED && WiFi.status() == WL_CONNECTED)
		{
			Serial.println("Connected to \"" + String(ssid) + "\" AP.");
			Serial.print("Use \'telnet ");
			Serial.print(WiFi.localIP());
			Serial.println(":" + String(telnetPort) + "\'");
			WiFiIntendedStatus = WIFI_CONNECTED;
		}

#ifdef SLEEP_ENABLE
		//try to connect WiFi to get commands
		if (SleepEnable && WiFiIntendedStatus == WIFI_CONNECTING && WiFi.status() != WL_CONNECTED && millis() - reconnectTimeStart < reconnectTimeOut)
		{
			Serial.println("Reconnecting...");
			while (WiFi.status() != WL_CONNECTED && millis() - reconnectTimeStart < reconnectTimeOut)
			{
				delay(50);
				if (WiFi.status() == WL_CONNECTED) WiFiIntendedStatus = WIFI_CONNECTED;
				yield();
			}
		}
#endif
	}

	//check if it's time to get sensors value
	if (millis() - checkSensorLastTime > checkSensorPeriod)
	{
		checkSensorLastTime = millis();
		String str = "";
		sensor = collectData();
		if (autoReport)
		{
			str = ParseSensorReport(sensor, "\r\n") + "\r\n" + str + "\r\n";
			if (WiFi.status() == WL_CONNECTED)
			{
				for (int i = 0; i < MAX_SRV_CLIENTS; i++)
				{
					if (serverClient[i] && serverClient[i].connected())
					{
						sendToNet(str, i);
						yield();
					}
				}

#ifdef TELEGRAM_ENABLE
				if (TelegramEnable)
				{
					for (int i = 0; i < telegramUsersNumber; i++)
					{
						if (telegramUsers[i] > 0)
						{
							addMessageToTelegramOutboundBuffer(telegramUsers[i], deviceName + ": " + str, telegramRetries);
						}
					}
				}
#endif

#ifdef GSCRIPT
				if (GScriptEnable) sendValueToGoogle(str);
#endif

#ifdef PUSHINGBOX
				if (PushingBoxEnable) sendToPushingBox(str);
#endif
			}
			Serial.println(str);
		}
	}


	//check if it's time to collect log
	if (millis() - logLastTime > logPeriod)
	{
		logLastTime = millis();
		saveLog(collectData());

#ifdef EMAIL_ENABLE
		if (history_record_number >= (int)(LOG_SIZE * 0.7))
		{
			if (EMailEnable && WiFi.status() == WL_CONNECTED && history_record_number > 0)
			{
				bool sendOk = true;
				int n = 0;
				String tmpStr = ParseSensorReport(history_log[n], ";");
				n++;
				while (n < history_record_number)
				{
					int freeMem = ESP.getFreeHeap() / 3;
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number) break;

						Serial.println("history_record_number=" + String(history_record_number));
						Serial.println("freeMem=" + String(freeMem));
						Serial.println("n=" + String(n));

						tmpStr += ParseSensorReport(history_log[n], ";") + "\r\n";
						yield();
					}
					sendOk = sendMail(smtpTo, deviceName + " log", tmpStr);
					tmpStr = "";
					yield();
					if (sendOk)
					{
						Serial.println("Log sent to EMail");
						history_record_number = 0;
					}
					else Serial.println("Error sending log to EMail " + String(history_record_number) + " records.");
				}
			}
		}
#endif
	}

	//check UART for data
	while (Serial.available())
	{
		char c = Serial.read();
		if (c == '\r' || c == '\n') uartReady = true;
		else uartCommand += (char)c;
	}

	//process data from UART
	if (uartReady)
	{
		uartReady = false;
		if (uartCommand != "")
		{
			String str = processCommand(uartCommand, UART_CHANNEL, true);
			Serial.println(str);
		}
		uartCommand = "";
	}

	//process data from HTTP/Telnet/TELEGRAM if available
	if (WiFi.status() == WL_CONNECTED)
	{
#ifdef HTTP_SERVER_ENABLE
		if (httpServerEnable) http_server.handleClient();
#endif

		if (telnetEnable)
		{
			//check if there are any new clients
			if (telnetServer.hasClient())
			{
				int i;
				for (i = 0; i < MAX_SRV_CLIENTS; i++)
				{
					//find free/disconnected spot
					if (!serverClient[i] || !serverClient[i].connected())
					{
						if (serverClient[i])
						{
							serverClient[i].stop();
						}
						serverClient[i] = telnetServer.available();
						Serial.println("Client connected: #" + String(i));
						break;
					}
				}
				//no free/disconnected spot so reject
				if (i >= MAX_SRV_CLIENTS)
				{
					WiFiClient serverClient = telnetServer.available();
					serverClient.stop();
					Serial.print("No more slots. Client rejected.");
				}
			}

			//check network clients for data
			for (int i = 0; i < MAX_SRV_CLIENTS; i++)
			{
				if (serverClient[i] && serverClient[i].connected())
				{
					if (serverClient[i].available())
					{
						//get data from the telnet client
						while (serverClient[i].available()) netInput[i] = serverClient[i].readString();
					}
				}
				yield();
			}

			//Process data from network
			for (int i = 0; i < MAX_SRV_CLIENTS; i++)
			{
				if (netInput[i] != "")
				{
					String str = processCommand(netInput[i], TELNET_CHANNEL, true);
					sendToNet(str, i);
					netInput[i] = "";
				}
				yield();
			}
		}

#ifdef TELEGRAM_ENABLE
		//check TELEGRAM for data
		if (TelegramEnable && millis() - telegramLastTime > telegramMessageDelay && WiFi.status() == WL_CONNECTED)
		{
			sendBufferToTelegram();
			telegramLastTime = millis();
		}
#endif
	}

#ifdef TM1637DISPLAY_ENABLE
	if (millis() - displaySwitchedLastTime > displaySwitchPeriod)
	{
		displaySwitchedLastTime = millis();

		//show CO2
		if (sw == 0)
		{
			int co2_avg = getCo2();
			if (co2_avg > 0) display.showNumberDec(co2_avg, false);
			else sw++;
		}
		//show Temperature
		if (sw == 1)
		{
			int temp = getTemperature();
			if (temp > -200)
			{
				display.showNumberDec(temp, false, 3, 0);
				display.setSegments(SEG_DEGREE, 1, 3);
			}
			else sw++;
		}
		//show Humidity
		if (sw == 2)
		{
			int humidity = getHumidity();
			if (humidity > -200)
			{
				display.showNumberDec(humidity, false, 3, 0);
				display.setSegments(SEG_HUMIDITY, 1, 3);
			}
			else sw++;
		}
		//show clock
		if (sw == 3)
		{
			display.showNumberDecEx(hour(), 0xff, true, 2, 0);
			display.showNumberDecEx(minute(), 0xff, true, 2, 2);
		}
		sw++;
		if (sw > 3) sw = 0;
	}
#endif

#ifdef EVENTS_ENABLE
	if (EventsEnable)
	{
		int a = EVENTS_TABLE_size / eventsNumber;
		for (int i = 0; i < eventsNumber; i++)
		{
			processEvent(readConfigString((EVENTS_TABLE_addr + i * a), a), i);
		}
	}
#endif

#ifdef SCHEDULER_ENABLE
	if (SchedulerEnable)
	{
		//????????????????
	}
#endif

#ifdef SLEEP_ENABLE
	if (SleepEnable)
	{
		unsigned long int ms = millis() - checkSensorLastTime;
		if (ms < checkSensorPeriod)
		{
			Serial.println("Sleep " + String(checkSensorPeriod - ms) + "ms");
			Serial.flush();
			ESP.deepSleep((checkSensorPeriod - ms) * 1000);
		}
	}
#endif
}

int CollectEepromSize()
{
	int eeprom_size = SSID_size;
	eeprom_size += PASS_size;
	eeprom_size += TELNET_PORT_size;
	eeprom_size += SENSOR_READ_DELAY_size;
	eeprom_size += DEVICE_NAME_size;

	eeprom_size += ENABLE_SLEEP_size;

	eeprom_size += EVENTS_TABLE_size;
	eeprom_size += ENABLE_EVENTS_size;

	eeprom_size += SCHEDULER_TABLE_size;
	eeprom_size += ENABLE_SCHEDULER_size;

	eeprom_size += SMTP_SERVER_ADDRESS_size;
	eeprom_size += SMTP_SERVER_PORT_size;
	eeprom_size += SMTP_LOGIN_size;
	eeprom_size += SMTP_PASSWORD_size;
	eeprom_size += SMTP_TO_size;
	eeprom_size += ENABLE_EMAIL_size;

	eeprom_size += TELEGRAM_TOKEN_size;
	eeprom_size += TELEGRAM_USERS_TABLE_size;
	eeprom_size += ENABLE_TELEGRAM_size;

	eeprom_size += GSCRIPT_ID_size;
	eeprom_size += ENABLE_GSCRIPT_size;

	eeprom_size += PUSHINGBOX_ID_size;
	eeprom_size += PUSHINGBOX_PARAM_size;
	eeprom_size += ENABLE_PUSHINGBOX_size;

	eeprom_size += HTTP_PORT_size;
	eeprom_size += ENABLE_HTTP_size;

	eeprom_size += NTP_SERVER_size;
	eeprom_size += NTP_TIME_ZONE_size;
	eeprom_size += NTP_REFRESH_DELAY_size;
	eeprom_size += ENABLE_NTP_size;

	eeprom_size += TM1637DISPLAY_REFRESH_size;

	eeprom_size += INT1_MODE_size;
	eeprom_size += INT2_MODE_size;
	eeprom_size += INT3_MODE_size;
	eeprom_size += INT4_MODE_size;
	eeprom_size += INT5_MODE_size;
	eeprom_size += INT6_MODE_size;
	eeprom_size += INT7_MODE_size;
	eeprom_size += INT8_MODE_size;

	eeprom_size += OUT1_INIT_size;
	eeprom_size += OUT2_INIT_size;
	eeprom_size += OUT3_INIT_size;
	eeprom_size += OUT4_INIT_size;
	eeprom_size += OUT5_INIT_size;
	eeprom_size += OUT6_INIT_size;
	eeprom_size += OUT7_INIT_size;
	eeprom_size += OUT8_INIT_size;

	return eeprom_size;
}

String readConfigString(int startAt, int maxlen)
{
	String str = "";
	for (int i = 0; i < maxlen; i++)
	{
		char c = (char)EEPROM.read(i + startAt);
		if (c == 0) i = maxlen;
		else str += c;
		yield();
	}
	return str;
}

void writeConfigString(int startAt, int maxlen, String str)
{
	for (int i = 0; i < maxlen; i++)
	{
		if (i < str.length())EEPROM.write(i + startAt, (uint8_t)str[i]);
		else EEPROM.write(i + startAt, 0);
		yield();
	}
	EEPROM.commit();
}

void sendToNet(String str, int clientN)
{
	if (serverClient[clientN] && serverClient[clientN].connected())
	{
		serverClient[clientN].print(str);
		delay(1);
	}
}

String printConfig()
{
	String str = "Device name: " + readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size) + "\r\n";
	str += "WiFi AP: " + readConfigString(SSID_addr, SSID_size) + "\r\n";
	str += "WiFi Password: " + readConfigString(PASS_addr, PASS_size) + "\r\n";
	str += "Telnet port: " + readConfigString(TELNET_PORT_addr, TELNET_PORT_size) + "\r\n";
	str += "Sensor read period: " + readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size) + "\r\n";
	str += "Telnet clients limit: " + String(MAX_SRV_CLIENTS) + "\r\n";

	//Log settings
	str += "Device log size: " + String(LOG_SIZE) + "\r\n";
	str += "Log record period: " + String(logPeriod) + "\r\n";

#ifdef SLEEP_ENABLE
	str += "WiFi reconnect timeout for SLEEP mode: " + String(reconnectTimeOut) + "\r\n";
#endif

	int m;
#ifdef HTTP_SERVER_ENABLE
	str += "HTTP port: " + readConfigString(HTTP_PORT_addr, HTTP_PORT_size) + "\r\n";
	str += "HTTP service enabled: " + readConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size) + "\r\n";
#endif

#ifdef EMAIL_ENABLE
	str += "SMTP server: " + readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size) + "\r\n";
	str += "SMTP port: " + readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size) + "\r\n";
	str += "SMTP login: " + readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size) + "\r\n";
	str += "SMTP password: " + readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size) + "\r\n";
	str += "Email To: " + readConfigString(SMTP_TO_addr, SMTP_TO_size) + "\r\n";
	str += "EMAIL service enabled: " + readConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size) + "\r\n";
#endif

#ifdef TELEGRAM_ENABLE
	str += "TELEGRAM token: " + readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size) + "\r\n";
	str += "Admin: " + uint64ToString(telegramUsers[0]) + "\r\n";
	m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	for (int i = 1; i < telegramUsersNumber; i++)
	{
		str += "\tUser #" + String(i) + ": " + readConfigString((TELEGRAM_USERS_TABLE_addr + i * m), m) + "\r\n";
	}
	str += "TELEGRAM service enabled: " + readConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size) + "\r\n";
#endif

#ifdef GSCRIPT
	str += "GSCRIPT token: " + readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size) + "\r\n";
	str += "GSCRIPT service enabled: " + readConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size) + "\r\n";
#endif

#ifdef PUSHINGBOX
	str += "PUSHINGBOX token: " + readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size) + "\r\n";
	str += "PUSHINGBOX parameter name: " + readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size) + "\r\n";
	str += "PUSHINGBOX service enabled: " + readConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size) + "\r\n";
#endif

#ifdef NTP_TIME_ENABLE
	str += "NTP server: " + readConfigString(NTP_SERVER_addr, NTP_SERVER_size) + "\r\n";
	str += "NTP time zone: " + readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size) + "\r\n";
	str += "NTP refresh delay: " + readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size) + "\r\n";
	str += "NTP service enabled: " + readConfigString(ENABLE_NTP_addr, ENABLE_NTP_size) + "\r\n";
#endif

#ifdef EVENTS_ENABLE
	m = EVENTS_TABLE_size / eventsNumber;
	for (int i = 1; i < eventsNumber; i++)
	{
		str += "\tEVENT #" + String(i) + ": " + readConfigString((EVENTS_TABLE_addr + i * m), m) + "\r\n";
	}
	str += "EVENTS service enabled: " + readConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size) + "\r\n";
#endif

#ifdef SCHEDULER_ENABLE
	m = SCHEDULER_TABLE_size / schedulesNumber;
	for (int i = 1; i < schedulesNumber; i++)
	{
		str += "\tSCHEDULE #" + String(i) + ": " + readConfigString((SCHEDULER_TABLE_addr + i * m), m) + "\r\n";
	}
	str += "SCHEDULER service enabled: " + readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size) + "\r\n";
#endif

#ifdef SLEEP_ENABLE
	str += "SLEEP mode enabled: " + readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size) + "\r\n";
#endif

#ifdef TM1637DISPLAY_ENABLE
	str += "TM1637 display refresh delay:";
	str += readConfigString(TM1637DISPLAY_REFRESH_addr, TM1637DISPLAY_REFRESH_size) + "\r\n";
	str += "pin TM1637_CLK: " + String(TM1637_CLK) + "\r\n";
	str += "pin TM1637_DIO: " + String(TM1637_DIO) + "\r\n";
#endif

#ifdef AMS2320_ENABLE
	str += "AMS2320 on i2c\r\n";
#endif

#ifdef HTU21D_ENABLE
	str += "HTU21D on i2c\r\n";
#endif

#ifdef DS18B20_ENABLE
	str += "DS18B20 on pin " + String(ONE_WIRE_PIN) + "\r\n";
#endif

#ifdef DHT_ENABLE
	str += DHTTYPE + " on pin " + String(DHTPIN) + "\r\n";
#endif

#ifdef MH_Z19_UART_ENABLE
	str += "MH-Z19 CO2 UART sensor:\r\n";
	str += "pin TX" + String(MHZ19_TX) + "\r\n";
	str += "pin RX" + String(MHZ19_RX) + "\r\n";
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += "MH-Z19 CO2 PPM sensor:\r\n";
	str += "pin PPM" + String(MHZ19_PPM) + "\r\n";
#endif

#ifdef ADC_ENABLE
	str += "ADC input:\r\n";
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	str += "Interrupt counter1 input on pin ";
	str += String(INTERRUPT_COUNTER1_ENABLE) + "\r\n";
	str += "Interrupt1 mode: ";
	str += readConfigString(INT1_MODE_addr, INT1_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	str += "Interrupt counter2 input on pin ";
	str += String(INTERRUPT_COUNTER2_ENABLE) + "\r\n";
	str += "Interrupt2 mode: ";
	str += readConfigString(INT2_MODE_addr, INT2_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	str += "Interrupt counter3 input on pin ";
	str += String(INTERRUPT_COUNTER3_ENABLE) + "\r\n";
	str += "Interrupt3 mode: ";
	str += readConfigString(INT3_MODE_addr, INT3_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	str += "Interrupt counter4 input on pin ";
	str += String(INTERRUPT_COUNTER4_ENABLE) + "\r\n";
	str += "Interrupt4 mode: ";
	str += readConfigString(INT4_MODE_addr, INT4_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	str += "Interrupt counter5 input on pin ";
	str += String(INTERRUPT_COUNTER5_ENABLE) + "\r\n";
	str += "Interrupt5 mode: ";
	str += readConfigString(INT5_MODE_addr, INT5_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	str += "Interrupt counter6 input on pin ";
	str += String(INTERRUPT_COUNTER6_ENABLE) + "\r\n";
	str += "Interrupt6 mode: ";
	str += readConfigString(INT6_MODE_addr, INT6_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	str += "Interrupt counter7 input on pin ";
	str += String(INTERRUPT_COUNTER7_ENABLE) + "\r\n";
	str += "Interrupt7 mode: ";
	str += readConfigString(INT7_MODE_addr, INT7_MODE_size) + "\r\n";
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	str += "Interrupt counter8 input on pin ";
	str += String(INTERRUPT_COUNTER8_ENABLE) + "\r\n";
	str += "Interrupt8 mode: ";
	str += readConfigString(INT8_MODE_addr, INT8_MODE_size) + "\r\n";
#endif
#ifdef INPUT1_ENABLE
	str += "Input1 on pin ";
	str += String(INPUT1_ENABLE) + "\r\n";
#endif
#ifdef INPUT2_ENABLE
	str += "Input2 on pin ";
	str += String(INPUT2_ENABLE) + "\r\n";
#endif
#ifdef INPUT3_ENABLE
	str += "Input3 on pin ";
	str += String(INPUT3_ENABLE) + "\r\n";
#endif
#ifdef INPUT4_ENABLE
	str += "Input4 on pin ";
	str += String(INPUT4_ENABLE) + "\r\n";
#endif
#ifdef INPUT5_ENABLE
	str += "Input5 on pin ";
	str += String(INPUT5_ENABLE) + "\r\n";
#endif
#ifdef INPUT6_ENABLE
	str += "Input6 on pin ";
	str += String(INPUT6_ENABLE) + "\r\n";
#endif
#ifdef INPUT7_ENABLE
	str += "Input7 on pin ";
	str += String(INPUT7_ENABLE) + "\r\n";
#endif
#ifdef INPUT8_ENABLE
	str += "Input8 on pin ";
	str += String(INPUT8_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT1_ENABLE
	str += "Output1 on pin ";
	str += String(OUTPUT1_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT2_ENABLE
	str += "Output2 on pin ";
	str += String(OUTPUT2_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT3_ENABLE
	str += "Output3 on pin ";
	str += String(OUTPUT3_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT4_ENABLE
	str += "Output4 on pin ";
	str += String(OUTPUT4_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT5_ENABLE
	str += "Output5 on pin ";
	str += String(OUTPUT5_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT6_ENABLE
	str += "Output6 on pin ";
	str += String(OUTPUT6_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT7_ENABLE
	str += "Output7 on pin ";
	str += String(OUTPUT7_ENABLE) + "\r\n";
#endif
#ifdef OUTPUT8_ENABLE
	str += "Output8 on pin ";
	str += String(OUTPUT8_ENABLE) + "\r\n";
#endif
	return str;
}

String printStatus()
{
	String str = currentTime() + "\r\n";
	str += "Time is ";
	if (isTimeSet) str += "set\r\n";
	else str += "not set\r\n";

	str += "WiFi enabled: " + String(wifiEnable) + "\r\n";
	str += "Telnet enabled: " + String(telnetEnable) + "\r\n";
#ifdef NTP_TIME_ENABLE
	str += "NTP enabled: " + String(NTPenable) + "\r\n";
#endif
	if (WiFi.status() == WL_CONNECTED)
	{
		str += "Connected to \"" + String(ssid) + "\" access point\r\n";
	}
	else str += "WiFi not connected\r\n";
	str += "Use: telnet ";
	str += WiFi.localIP().toString();
	str += ":" + String(telnetPort) + "\r\n";

	int netClientsNum = 0;
	for (int i = 0; i < MAX_SRV_CLIENTS; i++)
	{
		if (serverClient[i] && serverClient[i].connected()) netClientsNum++;
	}
	str += "Telnet clients connected: " + String(netClientsNum) + "\r\n";

	str += "Log records collected: " + String(history_record_number) + "\r\n";
	str += "Free memory: " + String(ESP.getFreeHeap()) + "\r\n";
	return str;
}

String printHelp()
{
	String tmp = "get_sensor\r\n";
	tmp += "get_status\r\n";
	tmp += "get_config\r\n";
	tmp += "[ADMIN] set_time=yyyy.mm.dd hh:mm:ss";
	tmp += "[ADMIN] autoreport=0/1\r\n";
	tmp += "[ADMIN] set_output?=0/1\r\n";

	tmp += "[ADMIN][FLASH] set_interrupt_mode?=FALLING/RISING/CHANGE\r\n";
	tmp += "[ADMIN][FLASH] set_output_init?=on/off/x\r\n";

	//tmp += "[ADMIN] getLog = uart/telnet/email/telegram";

	tmp += "[ADMIN][FLASH] check_period=n\r\n";

	tmp += "[ADMIN][FLASH] device_name=****\r\n";

	tmp += "[ADMIN][FLASH] ssid=****\r\n";
	tmp += "[ADMIN][FLASH] wifi_pass=****\r\n";
	tmp += "[ADMIN] wifi_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] telnet_port=n\r\n";
	tmp += "[ADMIN][FLASH] telnet_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] http_port=n\r\n";
	tmp += "[ADMIN][FLASH] http_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] smtp_server=****\r\n";
	tmp += "[ADMIN][FLASH] smtp_port=n\r\n";
	tmp += "[ADMIN][FLASH] smtp_login=****\r\n";
	tmp += "[ADMIN][FLASH] smtp_pass=****\r\n";
	tmp += "[ADMIN][FLASH] smtp_to=****@***.***\r\n";
	tmp += "[ADMIN][FLASH] smtp_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] set_user?=n\r\n";
	tmp += "[ADMIN][FLASH] telegram_token=****\r\n";
	tmp += "[ADMIN][FLASH] telegram_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] gscript_token=****\r\n";
	tmp += "[ADMIN][FLASH] gscript_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] pushingbox_token=****\r\n";
	tmp += "[ADMIN][FLASH] pushingbox_parameter=****\r\n";
	tmp += "[ADMIN][FLASH] pushingbox_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] ntp_server=***.***.***.***\r\n";
	tmp += "[ADMIN][FLASH] ntp_time_zone=n\r\n";
	tmp += "[ADMIN][FLASH] ntp_refresh_delay=n\r\n";
	tmp += "[ADMIN][FLASH] ntp_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] set_event?=condition:action1;action2\r\n";
	tmp += "[ADMIN][FLASH] events_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] set_schedule?=time:action1;action2\r\n";
	tmp += "[ADMIN][FLASH] scheduler_enable=1/0\r\n";

	tmp += "[ADMIN][FLASH] display_refresh=n\r\n";

	tmp += "[ADMIN][FLASH] sleep_enable=1/0\r\n";

	tmp += "[ADMIN] reset\r\n";
	return tmp;
}

String currentTime()
{
	// digital clock display of the time
	String tmp = String(hour());
	tmp += ":";
	tmp += String(minute());
	tmp += ":";
	tmp += String(second());
	tmp += " ";
	tmp += String(year());
	tmp += "/";
	tmp += String(month());
	tmp += "/";
	tmp += String(day());
	return tmp;
}

sensorDataCollection collectData()
{
	sensorDataCollection sensorData;
	sensorData.logyear = year();
	sensorData.logmonth = month();
	sensorData.logday = day();
	sensorData.loghour = hour();
	sensorData.logminute = minute();
	sensorData.logsecond = second();

#ifdef AMS2320_ENABLE
	sensorData.ams_humidity = am2320.readHumidity();
	sensorData.ams_temp = am2320.readTemperature();
#endif

#ifdef HTU21D_ENABLE
	sensorData.htu21d_humidity = htu21d.readHumidity();
	sensorData.htu21d_temp = htu21d.readTemperature();
#endif

#ifdef DS18B20_ENABLE
	ds1820.requestTemperatures();
	sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
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
	sensorData.mh_ppm_co2 = co2PPMRead(MHZ19_PPM);
#endif

#ifdef ADC_ENABLE
	sensorData.adc = getAdc();
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	sensorData.InterruptCounter1 = intCount1;
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	sensorData.InterruptCounter2 = intCount2;
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	sensorData.InterruptCounter3 = intCount3;
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	sensorData.InterruptCounter4 = intCount4;
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	sensorData.InterruptCounter5 = intCount5;
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	sensorData.InterruptCounter6 = intCount6;
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	sensorData.InterruptCounter7 = intCount7;
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	sensorData.InterruptCounter8 = intCount8;
#endif

#ifdef INPUT1_ENABLE
	sensorData.input1 = in1 = digitalRead(INPUT1_ENABLE);
#endif
#ifdef INPUT2_ENABLE
	sensorData.input2 = in2 = digitalRead(INPUT2_ENABLE);
#endif
#ifdef INPUT3_ENABLE
	sensorData.input3 = in3 = digitalRead(INPUT3_ENABLE);
#endif
#ifdef INPUT4_ENABLE
	sensorData.input4 = in4 = digitalRead(INPUT4_ENABLE);
#endif
#ifdef INPUT5_ENABLE
	sensorData.input5 = in5 = digitalRead(INPUT5_ENABLE);
#endif
#ifdef INPUT6_ENABLE
	sensorData.input6 = in6 = digitalRead(INPUT6_ENABLE);
#endif
#ifdef INPUT7_ENABLE
	sensorData.input7 = in7 = digitalRead(INPUT7_ENABLE);
#endif
#ifdef INPUT8_ENABLE
	sensorData.input8 = in8 = digitalRead(INPUT8_ENABLE);
#endif

#ifdef OUTPUT1_ENABLE
	sensorData.output1 = digitalRead(OUTPUT1_ENABLE);
#endif
#ifdef OUTPUT2_ENABLE
	sensorData.output2 = digitalRead(OUTPUT2_ENABLE);
#endif
#ifdef OUTPUT3_ENABLE
	sensorData.output3 = digitalRead(OUTPUT3_ENABLE);
#endif
#ifdef OUTPUT4_ENABLE
	sensorData.output4 = digitalRead(OUTPUT4_ENABLE);
#endif
#ifdef OUTPUT5_ENABLE
	sensorData.output5 = digitalRead(OUTPUT5_ENABLE);
#endif
#ifdef OUTPUT6_ENABLE
	sensorData.output6 = digitalRead(OUTPUT6_ENABLE);
#endif
#ifdef OUTPUT7_ENABLE
	sensorData.output7 = digitalRead(OUTPUT7_ENABLE);
#endif
#ifdef OUTPUT8_ENABLE
	sensorData.output8 = digitalRead(OUTPUT8_ENABLE);
#endif
	return sensorData;
}

String ParseSensorReport(sensorDataCollection data, String delimiter)
{
	String str = String(data.loghour);
	str += ":";
	str += String(data.logminute);
	str += ":";
	str += String(data.logsecond);
	str += " ";
	str += String(data.logyear);
	str += "/";
	str += String(data.logmonth);
	str += "/";
	str += String(data.logday);

#ifdef MH_Z19_UART_ENABLE
	str += delimiter + "UART CO2=" + String(data.mh_uart_co2);
	str += delimiter + "MH-Z19 t=" + String(data.mh_temp);
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += delimiter + "PPM CO2=" + String(data.mh_ppm_co2);
#endif

#ifdef AMS2320_ENABLE
	str += delimiter + "AMS2320 t=" + String(data.ams_temp);
	str += delimiter + "AMS2320 h=" + String(data.ams_humidity);
#endif

#ifdef HTU21D_ENABLE
	str += delimiter + "HTU21D t=" + String(data.htu21d_temp);
	str += delimiter + "HTU21D h=" + String(data.htu21d_humidity);
#endif

#ifdef DS18B20_ENABLE
	str += delimiter + "DS18B20 t=" + String(data.ds1820_temp);
#endif

#ifdef DHT_ENABLE
	str += delimiter + DHTTYPE + " t=" + String(data.dht_temp);
	str += delimiter + DHTTYPE + " h=" + String(data.dht_humidity);
#endif

#ifdef ADC_ENABLE
	str += delimiter + "ADC=" + String(data.adc);
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	str += delimiter + "Int1=" + String(data.InterruptCounter1);
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	str += delimiter + "Int2=" + String(data.InterruptCounter2);
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	str += delimiter + "Int3=" + String(data.InterruptCounter3);
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	str += delimiter + "Int4=" + String(data.InterruptCounter4);
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	str += delimiter + "Int5=" + String(data.InterruptCounter5);
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	str += delimiter + "Int6=" + String(data.InterruptCounter6);
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	str += delimiter + "Int7=" + String(data.InterruptCounter7);
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	str += delimiter + "Int8=" + String(data.InterruptCounter8);
#endif

#ifdef INPUT1_ENABLE
	str += delimiter + "IN1=" + String(data.input1);
#endif
#ifdef INPUT2_ENABLE
	str += delimiter + "IN2=" + String(data.input2);
#endif
#ifdef INPUT3_ENABLE
	str += delimiter + "IN3=" + String(data.input3);
#endif
#ifdef INPUT4_ENABLE
	str += delimiter + "IN4=" + String(data.input4);
#endif
#ifdef INPUT5_ENABLE
	str += delimiter + "IN5=" + String(data.input5);
#endif
#ifdef INPUT6_ENABLE
	str += delimiter + "IN6=" + String(data.input6);
#endif
#ifdef INPUT7_ENABLE
	str += delimiter + "IN7=" + String(data.input7);
#endif
#ifdef INPUT8_ENABLE
	str += delimiter + "IN8=" + String(data.input8);
#endif

#ifdef OUTPUT1_ENABLE
	str += delimiter + "OUT1=" + String(data.output1);
#endif
#ifdef OUTPUT2_ENABLE
	str += delimiter + "OUT2=" + String(data.output2);
#endif
#ifdef OUTPUT3_ENABLE
	str += delimiter + "OUT3=" + String(data.output3);
#endif
#ifdef OUTPUT4_ENABLE
	str += delimiter + "OUT4=" + String(data.output4);
#endif
#ifdef OUTPUT5_ENABLE
	str += delimiter + "OUT5=" + String(data.output5);
#endif
#ifdef OUTPUT6_ENABLE
	str += delimiter + "OUT6=" + String(data.output6);
#endif
#ifdef OUTPUT7_ENABLE
	str += delimiter + "OUT7=" + String(data.output7);
#endif
#ifdef OUTPUT8_ENABLE
	str += delimiter + "OUT8=" + String(data.output8);
#endif
	return str;
}

String processCommand(String command, byte channel, bool isAdmin)
{
	String tmp = command;
	tmp.toLowerCase();
	String str = "";
	if (tmp == "get_sensor")
	{
		str = ParseSensorReport(collectData(), "\r\n") + "\r\n";
	}
	else if (tmp == "get_status")
	{
		str = printStatus() + "\r\n";
	}
	else if (tmp == "get_config")
	{
		str = currentTime() + "\r\n" + printConfig() + "\r\n";
	}
	else if (isAdmin)
	{
		if (tmp.startsWith("set_time=") && tmp.length() == 28)
		{
			int t = command.indexOf('=');

			int _hr = command.substring(20, 2).toInt();
			int _min = command.substring(23, 2).toInt();
			int _sec = command.substring(26, 2).toInt();
			int _day = command.substring(17, 2).toInt();
			int _month = command.substring(14, 2).toInt();
			int _yr = command.substring(9, 4).toInt();
			setTime(_hr, _min, _sec, _day, _month, _yr);
			str = "Time set to:" + String(_yr) + ".";
			str += String(_month) + ".";
			str += String(_day) + ".";
			str += String(_hr) + ":";
			str += String(_min) + ":";
			str += String(_sec) + "\r\n";
		}
		else if (tmp.startsWith("check_period=") && tmp.length() > 14)
		{
			checkSensorPeriod = command.substring(command.indexOf('=') + 1).toInt();
			str = "New sensor check period = \"" + String(checkSensorPeriod) + "\"\r\n";
			writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(checkSensorPeriod));
		}
		else if (tmp.startsWith("device_name=") && tmp.length() > 13)
		{
			deviceName = command.substring(command.indexOf('=') + 1);
			str = "New device name = \"" + deviceName + "\"\r\n";
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
		}
		else if (tmp.startsWith("ssid=") && tmp.length() > 6)
		{
			ssid = command.substring(command.indexOf('=') + 1);
			str = "New SSID = \"" + ssid + "\"\r\n";
			writeConfigString(SSID_addr, SSID_size, ssid);
			WiFi.begin(ssid.c_str(), WiFiPass.c_str());
		}
		else if (tmp.startsWith("wifi_pass=") && tmp.length() > 11)
		{
			WiFiPass = command.substring(command.indexOf('=') + 1);
			str = "New password = \"" + WiFiPass + "\"\r\n";
			writeConfigString(PASS_addr, PASS_size, WiFiPass);
			WiFi.begin(ssid.c_str(), WiFiPass.c_str());
		}
		else if (tmp.startsWith("wifi_enable=") && tmp.length() > 13)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				wifiEnable = false;
				WiFi.mode(WIFI_OFF);
				WiFiIntendedStatus = WIFI_STOP;
				str = "WiFi disabled\r\n";
			}
			else if (ar == '1')
			{
				wifiEnable = true;
				WiFi.mode(WIFI_STA);
				WiFi.begin(ssid.c_str(), WiFiPass.c_str());
				WiFiIntendedStatus = WIFI_CONNECTING;
				str = "WiFi enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}

		else if (tmp.startsWith("telnet_port=") && tmp.length() > 13)
		{
			telnetPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New telnet port = \"" + String(telnetPort) + "\"\r\n";
			writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith("telnet_enable=") && tmp.length() > 15)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				telnetEnable = false;
				telnetServer.stop();
				str = "Telnet disabled\r\n";
			}
			else if (ar == '1')
			{
				telnetEnable = true;
				telnetServer.begin();
				telnetServer.setNoDelay(true);
				str = "Telnet enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}

		else if (tmp == "reset" && tmp.length() == 6)
		{
			Serial.println("Resetting...");
			Serial.flush();
			ESP.restart();
		}

		else if (tmp.startsWith("autoreport=") && tmp.length() > 12)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				autoReport = false;
				str = "autoreport=off\r\n";
			}
			else if (ar == '1')
			{
				autoReport = true;
				str = "autoreport=on\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}

		else if (tmp.startsWith("set_output") && tmp.length() > 13)
		{
			str = set_output(tmp);
		}

		else if (tmp.startsWith("set_output_init") && tmp.length() > 18)
		{
			int t = command.indexOf('=');
			if (t >= 17 && t < tmp.length() - 1)
			{
				byte outNum = tmp.substring(15, t - 1).toInt();
				String outStateStr = tmp.substring(t + 1);
				bool outState = outStateStr.toInt();
				//Serial.println("Out" + String(outNum) + "=" + String(outState));
				if (outStateStr == "on") outState = OUT_ON;
				else if (outStateStr == "off") outState = OUT_OFF;
				else
				{
					outState = outStateStr.toInt();
				}
#ifdef OUTPUT1_ENABLE
				if (outNum == 1)
				{
					writeConfigString(OUT1_INIT_addr, OUT1_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT2_ENABLE
				if (outNum == 2)
				{
					writeConfigString(OUT2_INIT_addr, OUT2_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT3_ENABLE
				if (outNum == 3)
				{
					writeConfigString(OUT3_INIT_addr, OUT3_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT4_ENABLE
				if (outNum == 4)
				{
					writeConfigString(OUT4_INIT_addr, OUT4_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT5_ENABLE
				if (outNum == 5)
				{
					writeConfigString(OUT5_INIT_addr, OUT5_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT6_ENABLE
				if (outNum == 6)
				{
					writeConfigString(OUT6_INIT_addr, OUT6_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT7_ENABLE
				if (outNum == 7)
				{
					writeConfigString(OUT7_INIT_addr, OUT7_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
#ifdef OUTPUT8_ENABLE
				if (outNum == 8)
				{
					writeConfigString(OUT8_INIT_addr, OUT8_INIT_size, String((int)outState));
					str = "Output #" + String(outNum) + " initial state set to " + String(int(outState));
				}
#endif
				if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
			}
		}

		else if (tmp.startsWith("set_interrupt_mode") && tmp.length() > 21)
		{
			int t = command.indexOf('=');
			if (t >= 11 && t < tmp.length() - 1)
			{
				byte outNum = tmp.substring(10, t).toInt();
				String intMode = tmp.substring(t + 1);
				int intModeNum = 0;
				if (intMode == "rising") intModeNum = RISING;
				else if (intMode == "falling") intModeNum = FALLING;
				else if (intMode == "change") intModeNum = CHANGE;
				{
					return "Incorrect value \"" + String(intMode) + "\"";
				}
				//Serial.println("Interrupt" + String(outNum) + " mode =" + intMode);
#ifdef INTERRUPT_COUNTER1_ENABLE
				if (outNum == 1)
				{
					intMode1 = intModeNum;
					writeConfigString(INT1_MODE_addr, INT1_MODE_size, String(intMode1));
					attachInterrupt(INTERRUPT_COUNTER1_ENABLE, int1count, intMode1);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
				if (outNum == 2)
				{
					intMode2 = intModeNum;
					writeConfigString(INT2_MODE_addr, INT2_MODE_size, String(intMode2));
					attachInterrupt(INTERRUPT_COUNTER2_ENABLE, int2count, intMode2);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
				if (outNum == 3)
				{
					intMode3 = intModeNum;
					writeConfigString(INT3_MODE_addr, INT3_MODE_size, String(intMode3));
					attachInterrupt(INTERRUPT_COUNTER3_ENABLE, int3count, intMode3);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
				if (outNum == 4)
				{
					intMode4 = intModeNum;
					writeConfigString(INT4_MODE_addr, INT4_MODE_size, String(intMode4));
					attachInterrupt(INTERRUPT_COUNTER4_ENABLE, int4count, intMode4);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
				if (outNum == 5)
				{
					intMode5 = intModeNum;
					writeConfigString(INT5_MODE_addr, INT5_MODE_size, String(intMode5));
					attachInterrupt(INTERRUPT_COUNTER5_ENABLE, int5count, intMode5);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
				if (outNum == 6)
				{
					intMode6 = intModeNum;
					writeConfigString(INT6_MODE_addr, INT6_MODE_size, String(intMode6));
					attachInterrupt(INTERRUPT_COUNTER6_ENABLE, int6count, intMode6);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
				if (outNum == 7)
				{
					intMode7 = intModeNum;
					writeConfigString(INT7_MODE_addr, INT7_MODE_size, String(intMode7));
					attachInterrupt(INTERRUPT_COUNTER7_ENABLE, int7count, intMode7);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
				if (outNum == 8)
				{
					intMode8 = intModeNum;
					writeConfigString(INT8_MODE_addr, INT8_MODE_size, String(intMode8));
					attachInterrupt(INTERRUPT_COUNTER8_ENABLE, int8count, intMode8);
					str = "Output" + String(outNum) + "=" + intMode;
				}
#endif
				if (str == "")	str = "Incorrect interrupt #" + String(outNum) + "\r\n";
			}
		}

		//getLog=uart/telnet/email/telegram

#ifdef SLEEP_ENABLE
		else if (tmp.startsWith("sleep_enable=") && tmp.length() > 14)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				SleepEnable = false;
				writeConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size, String(ar));
				str = "SLEEP mode disabled\r\n";
			}
			else if (ar == '1')
			{
				SleepEnable = true;
				writeConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size, String(ar));
				str = "SLEEP mode enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef HTTP_SERVER_ENABLE
		else if (tmp.startsWith("http_port=") && tmp.length() > 11)
		{
			httpPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New HTTP port = \"" + String(httpPort) + "\"\r\n";
			writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
			if (httpServerEnable) http_server.begin(httpPort);
		}
		else if (tmp.startsWith("http_enable=") && tmp.length() > 13)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size, String(ar));
				httpServerEnable = false;
				if (httpServerEnable) http_server.stop();
				str = "HTTP server disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size, String(ar));
				httpServerEnable = true;
				http_server.on("/", handleRoot);
				http_server.onNotFound(handleNotFound);
				http_server.begin(httpPort);
				str = "HTTP server enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef EMAIL_ENABLE
		else if (tmp == "mailme")
		{
			if (EMailEnable && WiFi.status() == WL_CONNECTED)
			{
				if (history_record_number < 1) return "No log records";
				bool sendOk = true;
				int n = 0;
				String tmpStr = ParseSensorReport(history_log[n], ";");
				n++;
				while (n < history_record_number)
				{
					int freeMem = ESP.getFreeHeap() / 3;
					Serial.println("***");
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number) break;

						Serial.println("history_record_number=" + String(history_record_number));
						Serial.println("freeMem=" + String(freeMem));
						Serial.println("n=" + String(n));
						Serial.println("***");

						tmpStr += ParseSensorReport(history_log[n], ";") + "\r\n";
						yield();
					}
					SendEmail e(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 5000, false);
					sendOk = e.send(smtpLogin, smtpTo, deviceName + " log", tmpStr);
					tmpStr = "";
					yield();
					if (sendOk)
					{
						str = "Log sent to EMail";
						history_record_number = 0;
					}
					else str = "Error sending log to EMail " + String(history_record_number) + " records.";
				}
			}
		}
		else if (tmp.startsWith("smtp_server=") && tmp.length() > 13)
		{
			smtpServerAddress = command.substring(command.indexOf('=') + 1);
			str = "New SMTP server IP address = \"" + smtpServerAddress + "\"\r\n";
			writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, smtpServerAddress);
		}
		else if (tmp.startsWith("smtp_port=") && tmp.length() > 11)
		{
			smtpServerPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New SMTP port = \"" + String(smtpServerPort) + "\"\r\n";
			writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith("smtp_login=") && tmp.length() > 12)
		{
			smtpLogin = command.substring(command.indexOf('=') + 1);
			str = "New SMTP login = \"" + smtpLogin + "\"\r\n";
			writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, smtpLogin);
		}
		else if (tmp.startsWith("smtp_pass=") && tmp.length() > 11)
		{
			smtpPassword = command.substring(command.indexOf('=') + 1);
			str = "New SMTP password = \"" + smtpPassword + "\"\r\n";
			writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, smtpPassword);
		}
		else if (tmp.startsWith("smtp_to=") && tmp.length() > 9)
		{
			smtpTo = command.substring(command.indexOf('=') + 1);
			str = "New SMTP TO address = \"" + smtpTo + "\"\r\n";
			writeConfigString(SMTP_TO_addr, SMTP_TO_size, smtpTo);
		}
		else if (tmp.startsWith("smtp_enable=") && tmp.length() > 13)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size, String(ar));
				EMailEnable = false;
				str = "EMail reporting disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size, String(ar));
				EMailEnable = true;
				str = "EMail reporting enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef TELEGRAM_ENABLE
		else if (tmp.startsWith("set_user") && tmp.length() > 9)
		{
			int t = command.indexOf('=');
			if (t >= 9 && t < tmp.length() - 1)
			{
				byte userNum = tmp.substring(8, t).toInt();
				uint64_t newUser = StringToUint64(tmp.substring(t + 1));
				if (userNum < telegramUsersNumber)
				{
					int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
					writeConfigString(TELEGRAM_USERS_TABLE_addr + userNum * m, m, uint64ToString(newUser));
					telegramUsers[userNum] = newUser;
					str = "User #" + String(userNum) + " is now: " + uint64ToString(telegramUsers[userNum]) + "\r\n";
				}
				else
				{
					str = "User #" + String(userNum) + " is out of range [0," + String(telegramUsersNumber - 1) + "]\r\n";
				}
			}
		}
		else if (tmp.startsWith("telegram_token=") && tmp.length() > 16)
		{
			telegramToken = command.substring(command.indexOf('=') + 1);
			str = "New TELEGRAM token = \"" + telegramToken + "\"\r\n";
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, telegramToken);
			myBot.setTelegramToken(telegramToken);
		}
		else if (tmp.startsWith("telegram_enable=") && tmp.length() > 17)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size, String(ar));
				TelegramEnable = false;
				str = "Telegram disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size, String(ar));
				TelegramEnable = true;
				//myBot.wifiConnect(ssid, WiFiPass);
				myBot.setTelegramToken(telegramToken);
				//myBot.testConnection();
				str = "Telegram enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef GSCRIPT
		else if (tmp.startsWith("gscript_token=") && tmp.length() > 15)
		{
			GScriptId = command.substring(command.indexOf('=') + 1);
			str = "New GScript token = \"" + GScriptId + "\"\r\n";
			writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, GScriptId);
		}
		else if (tmp.startsWith("gscript_enable=") && tmp.length() > 16)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size, String(ar));
				GScriptEnable = false;
				str = "GScript disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size, String(ar));
				GScriptEnable = true;
				str = "GScript enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef PUSHINGBOX
		else if (tmp.startsWith("pushingbox_token=") && tmp.length() > 18)
		{
			pushingBoxId = command.substring(command.indexOf('=') + 1);
			str = "New PushingBox token = \"" + pushingBoxId + "\"\r\n";
			writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, pushingBoxId);
		}
		else if (tmp.startsWith("pushingbox_parameter=") && tmp.length() > 22)
		{
			pushingBoxParameter = command.substring(command.indexOf('=') + 1);
			str = "New PUSHINGBOX parameter name = \"" + pushingBoxParameter + "\"\r\n";
			writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, pushingBoxParameter);
		}
		else if (tmp.startsWith("pushingbox_enable=") && tmp.length() > 19)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size, String(ar));
				PushingBoxEnable = false;
				str = "PushingBox disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size, String(ar));
				PushingBoxEnable = true;
				str = "PushingBox enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef NTP_TIME_ENABLE
		else if (tmp.startsWith("ntp_server=") && tmp.length() > 12)
		{
			String tmpSrv = (command.substring(command.indexOf('=') + 1));
			IPAddress tmpAddr;
			if (tmpAddr.fromString(tmpSrv))
			{
				timeServer = tmpAddr;
				str = "New NTP server = \"" + timeServer.toString() + "\"\r\n";
				writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, timeServer.toString());
			}
			else
			{
				str = "Incorrect NTP server \"" + tmpSrv + "\"\r\n";
			}
		}
		else if (tmp.startsWith("ntp_time_zone=") && tmp.length() > 15)
		{
			timeZone = command.substring(command.indexOf('=') + 1).toInt();
			str = "New NTP time zone = \"" + String(timeZone) + "\"\r\n";
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
		}
		else if (tmp.startsWith("ntp_refresh_delay=") && tmp.length() > 19)
		{
			ntpRefreshDelay = command.substring(command.indexOf('=') + 1).toInt();
			str = "New NTP refresh delay = \"" + String(ntpRefreshDelay) + "\"\r\n";
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);
		}
		else if (tmp.startsWith("ntp_enable=") && tmp.length() > 12)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_NTP_addr, ENABLE_NTP_size, String(ar));
				NTPenable = false;
				setSyncProvider(nullptr);
				Udp.stop();
				str = "NTP disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_NTP_addr, ENABLE_NTP_size, String(ar));
				NTPenable = true;
				Udp.begin(localPort);
				setSyncProvider(getNtpTime);
				setSyncInterval(ntpRefreshDelay);
				str = "NTP enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef EVENTS_ENABLE
		else if (tmp.startsWith("set_event") && tmp.length() > 12)
		{
			int t = command.indexOf('=');
			if (t >= 10 && t < tmp.length() - 1)
			{
				byte eventNum = tmp.substring(9, t).toInt();
				String newEvent = tmp.substring(t + 1);
				if (eventNum < eventsNumber)
				{
					int m = EVENTS_TABLE_size / eventsNumber;
					writeConfigString(EVENTS_TABLE_addr + eventNum * m, m, newEvent);
					str = "Event #" + String(eventNum) + " is now: " + newEvent + "\r\n";
				}
				else
				{
					str = "Event #" + String(eventNum) + " is out of range [0," + String(eventsNumber - 1) + "]\r\n";
				}
			}
		}
		else if (tmp.startsWith("events_enable=") && tmp.length() > 15)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size, String(ar));
				EventsEnable = false;
				str = "Events disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size, String(ar));
				EventsEnable = true;
				str = "Events enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef SCHEDULER_ENABLE
		else if (tmp.startsWith("set_schedule") && tmp.length() > 15)
		{
			int t = command.indexOf('=');
			if (t >= 13 && t < tmp.length() - 1)
			{
				byte scheduleNum = tmp.substring(12, t).toInt();
				String newSchedule = tmp.substring(t + 1);
				if (scheduleNum < schedulesNumber)
				{
					int m = SCHEDULER_TABLE_size / schedulesNumber;
					writeConfigString(SCHEDULER_TABLE_addr + scheduleNum * m, m, newSchedule);
					str = "Schedule #" + String(scheduleNum) + " is now: " + newSchedule + "\r\n";
				}
				else
				{
					str = "Schedule #" + String(scheduleNum) + " is out of range [0," + String(schedulesNumber - 1) + "]\r\n";
				}
			}
		}
		else if (tmp.startsWith("scheduler_enable=") && tmp.length() > 18)
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				writeConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size, String(ar));
				SchedulerEnable = false;
				str = "Scheduler disabled\r\n";
			}
			else if (ar == '1')
			{
				writeConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size, String(ar));
				SchedulerEnable = true;
				str = "Scheduler enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef TM1637DISPLAY_ENABLE
		else if (tmp.startsWith("display_refresh=") && tmp.length() > 17)
		{
			displaySwitchPeriod = command.substring(command.indexOf('=') + 1).toInt();
			str = "New display refresh period = \"" + String(displaySwitchPeriod) + "\"\r\n";
			writeConfigString(TM1637DISPLAY_REFRESH_addr, TM1637DISPLAY_REFRESH_size, String(displaySwitchPeriod));
		}
#endif

		else
		{
			str = "Incorrect command: \"" + command + "\"\r\n";
			// str += printHelp() + "\r\n";
		}
	}
	else
	{
		str = "Incorrect command: \"" + command + "\"\r\n";
		// str += printHelp() + "\r\n";
	}
	return str;
}

String processEvent(String event, byte eventNum)
{
	//conditions:
	//	value<=>x; adc, temp, hum, co2,
	//	input?=0/1/c;
	//	output?=0/1/c;
	//	counter?>x;
	String condition = event.substring(0, event.indexOf(':') - 1);
	condition.toLowerCase();
	event = event.substring(event.indexOf(':') + 1);
	String str = "";
	if (condition.startsWith("input"))
	{
		int t = condition.indexOf('=');
		if (t >= 6 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(5, t).toInt();
			String outStateStr = condition.substring(t + 1);
			bool outState = OUT_OFF;
			if (outStateStr != "c")
			{
				if (outStateStr == "on") outState = OUT_ON;
				else if (outStateStr == "off") outState = OUT_OFF;
				else outState = outStateStr.toInt();
			}
#ifdef INPUT1_ENABLE
			if (outNum == 1)
			{
				if (outStateStr == "c")
				{
					if (in1 != digitalRead(INPUT1_ENABLE))
					{
						in1 = digitalRead(INPUT1_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT1_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT2_ENABLE
			if (outNum == 2)
			{
				if (outStateStr == "c")
				{
					if (in2 != digitalRead(INPUT2_ENABLE))
					{
						in2 = digitalRead(INPUT2_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT2_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT3_ENABLE
			if (outNum == 3)
			{
				if (outStateStr == "c")
				{
					if (in3 != digitalRead(INPUT3_ENABLE))
					{
						in3 = digitalRead(INPUT3_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT3_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT4_ENABLE
			if (outNum == 4)
			{
				if (outStateStr == "c")
				{
					if (in4 != digitalRead(INPUT4_ENABLE))
					{
						in4 = digitalRead(INPUT4_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT4_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT5_ENABLE
			if (outNum == 5)
			{
				if (outStateStr == "c")
				{
					if (in5 != digitalRead(INPUT5_ENABLE))
					{
						in5 = digitalRead(INPUT5_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT5_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT6_ENABLE
			if (outNum == 6)
			{
				if (outStateStr == "c")
				{
					if (in6 != digitalRead(INPUT6_ENABLE))
					{
						in6 = digitalRead(INPUT6_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT6_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT7_ENABLE
			if (outNum == 7)
			{
				if (outStateStr == "c")
				{
					if (in7 != digitalRead(INPUT7_ENABLE))
					{
						in7 = digitalRead(INPUT7_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT7_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INPUT8_ENABLE
			if (outNum == 8)
			{
				if (outStateStr == "c")
				{
					if (in8 != digitalRead(INPUT8_ENABLE))
					{
						in8 = digitalRead(INPUT8_ENABLE);
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(INPUT8_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
			if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
		}
	}
	else if (condition.startsWith("output"))
	{
		int t = condition.indexOf('=');
		if (t >= 7 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(6, t).toInt();
			String outStateStr = condition.substring(t + 1);
			bool outState = OUT_OFF;
			if (outStateStr != "c")
			{
				if (outStateStr == "on") outState = OUT_ON;
				else if (outStateStr == "off") outState = OUT_OFF;
				else outState = outStateStr.toInt();
			}
#ifdef OUTPUT1_ENABLE
			if (outNum == 1)
			{
				if (outStateStr == "c")
				{
					if (out1 != digitalRead(OUTPUT1_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT1_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT2_ENABLE
			if (outNum == 2)
			{
				if (outStateStr == "c")
				{
					if (out2 != digitalRead(OUTPUT2_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT2_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT3_ENABLE
			if (outNum == 3)
			{
				if (outStateStr == "c")
				{
					if (out3 != digitalRead(OUTPUT3_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT3_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT4_ENABLE
			if (outNum == 4)
			{
				if (outStateStr == "c")
				{
					if (out4 != digitalRead(OUTPUT4_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT4_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT5_ENABLE
			if (outNum == 5)
			{
				if (outStateStr == "c")
				{
					if (out5 != digitalRead(OUTPUT5_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT5_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT6_ENABLE
			if (outNum == 6)
			{
				if (outStateStr == "c")
				{
					if (out6 != digitalRead(OUTPUT6_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT6_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT7_ENABLE
			if (outNum == 7)
			{
				if (outStateStr == "c")
				{
					if (out7 != digitalRead(OUTPUT7_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT7_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef OUTPUT8_ENABLE
			if (outNum == 8)
			{
				if (outStateStr == "c")
				{
					if (out8 != digitalRead(OUTPUT8_ENABLE))
					{
						str = ProcessAction(event, eventNum);
					}
				}
				else if (outState == digitalRead(OUTPUT8_ENABLE)) str = ProcessAction(event, eventNum);
			}
#endif
			if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
		}
	}
	else if (condition.startsWith("counter"))
	{
		int t = condition.indexOf('>');
		if (t >= 8 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(7, t).toInt();
			String outStateStr = condition.substring(t + 1);
			int outState = outStateStr.toInt();
#ifdef INTERRUPT_COUNTER1_ENABLE
			if (outNum == 1)
			{
				if (intCount1 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
			if (outNum == 2)
			{
				if (intCount2 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
			if (outNum == 3)
			{
				if (intCount3 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
			if (outNum == 4)
			{
				if (intCount4 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
			if (outNum == 5)
			{
				if (intCount5 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
			if (outNum == 6)
			{
				if (intCount6 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
			if (outNum == 7)
			{
				if (intCount7 > outState) str = ProcessAction(event, eventNum);
			}
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
			if (outNum == 8)
			{
				if (intCount8 > outState) str = ProcessAction(event, eventNum);
			}
#endif
			if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
		}
		else str = "Incorrect condition: \"" + condition + "\"\r\n";
	}

#ifdef ADC_ENABLE
	else if (condition.startsWith("adc") && condition.length() > 4)
	{
		char oreration = condition[3];
		int value = condition.substring(4).toInt();
		if (oreration == '>' && value > getAdc())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else if (oreration == '<' && value < getAdc())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else
		{
			str = "Incorrect condition: \"" + condition + "\"\r\n";
		}
	}
#endif

#if defined(DS18B20_ENABLE) || defined(MH_Z19_UART_ENABLE) || defined(MH_Z19_PPM_ENABLE) || defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE)
	else if (condition.startsWith("temperature") && condition.length() > 12)
	{
		char oreration = condition[11];
		int value = condition.substring(12).toInt();
		if (oreration == '>' && value > getTemperature())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else if (oreration == '<' && value < getTemperature())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else
		{
			str = "Incorrect condition: \"" + condition + "\"\r\n";
		}
	}
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE)
	else if (condition.startsWith("humidity") && condition.length() > 9)
	{
		char oreration = condition[8];
		int value = condition.substring(9).toInt();
		if (oreration == '>' && value > getHumidity())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else if (oreration == '<' && value < getHumidity())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else
		{
			str = "Incorrect condition: \"" + condition + "\"\r\n";
		}
	}
#endif

#if defined(MH_Z19_UART_ENABLE)|| defined(MH_Z19_PPM_ENABLE)
	else if (condition.startsWith("co2") && condition.length() > 4)
	{
		char oreration = condition[3];
		int value = condition.substring(4).toInt();
		if (oreration == '>' && value > getCo2())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else if (oreration == '<' && value < getCo2())
		{
			str = "Event started: \"" + condition + "\"\r\n";
			str += ProcessAction(event, eventNum);
		}
		else
		{
			str = "Incorrect condition: \"" + condition + "\"\r\n";
		}
	}
#endif
	else
	{
		str = "Incorrect event#" + String(eventNum) + " condition: \"" + condition + "\"\r\n";
		str += printHelp() + "\r\n";
	}
	return str;
}

String ProcessAction(String action, byte eventNum)
{
#ifdef EVENTS_ENABLE
	eventsFlags[eventNum] = true;
#endif

	int n = countOf(action, ';');
	if (action.lastIndexOf(';') == action.length() - 1) n--;
	String str;
	int i = 0;
	while (i < n)
	{
		String tmpAction = action.substring(0, action.indexOf(';') - 1);
		action = action.substring(action.indexOf(';') + 1);
		//set_output?=x
		if (tmpAction.startsWith("set_output") && tmpAction.length() >= 13)
		{
			str = set_output(tmpAction);
		}
		//reset_counter?
		else if (tmpAction.startsWith("reset_counter") && tmpAction.length() > 14)
		{
			int n = tmpAction.substring(14).toInt();

#ifdef INTERRUPT_COUNTER1_ENABLE
			if (n == 1) intCount1 = 0;
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
			if (n == 2) intCount2 = 0;
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
			if (n == 3) intCount3 = 0;
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
			if (n == 4) intCount4 = 0;
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
			if (n == 5) intCount5 = 0;
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
			if (n == 6) intCount6 = 0;
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
			if (n == 7) intCount7 = 0;
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
			if (n == 8) intCount8 = 0;
#endif
		}
		//reset_flag?
#ifdef EVENTS_ENABLE
		else if (tmpAction.startsWith("reset_flag") && tmpAction.length() > 10)
		{
			int n = tmpAction.substring(10).toInt();
			eventsFlags[n] = false;
		}
#endif
		//send_telegram=* / usrer#,message
#ifdef TELEGRAM_ENABLE
		else if (tmpAction.startsWith("send_telegram"))
		{
			int i = 0;
			int j = 0;
			if (tmpAction[14] == '*') //if * then all
			{
				i = 0;
				j = telegramUsersNumber;
			}
			else
			{
				i = tmpAction.substring(11).toInt();
				j = i + 1;
			}
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			for (; i < j; i++)
			{
				if (telegramUsers[i] > 0)
				{
					addMessageToTelegramOutboundBuffer(telegramUsers[i], deviceName + ": " + tmpAction + ParseSensorReport(sensor, ";"), telegramRetries);
				}
			}
		}
#endif
		//send_PushingBox=message
#ifdef PUSHINGBOX
		else if (tmpAction.startsWith("send_PushingBox"))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			bool sendOk = sendToPushingBox(deviceName + ": " + tmpAction + "\n" + ParseSensorReport(sensor, "\n"));
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//send_mail=address,message
#ifdef EMAIL_ENABLE
		else if (tmpAction.startsWith("send_mail"))
		{
			String address = tmpAction.substring(tmpAction.indexOf('=') + 1, tmpAction.indexOf(',') - 1);
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			bool sendOk = sendMail(address, deviceName + " alert", tmpAction);
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//send_GScript=message
#ifdef GSCRIPT
		else if (tmpAction.startsWith("send_GScript"))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			bool sendOk = sendValueToGoogle(deviceName + ": " + tmpAction);
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//save_log
		else if (tmpAction.startsWith("save_log"))
		{
			saveLog(sensor);
		}
		else
		{
			str = "Incorrect action #" + String(i) + " in the event#" + String(eventNum) + ": \"" + tmpAction + "\"\r\n";
			str += printHelp() + "\r\n";
		}
		i++;
	}
	return str;
}

int countOf(String str, char c)
{
	int count = -1;
	for (int i = 0; i < str.length(); i++)
		if (str[i] == c) count++;
	if (count > -1) count++;
	return count;
}

String set_output(String tmpAction)
{
	int t = tmpAction.indexOf('=');
	String str;
	if (t >= 11 && t < tmpAction.length() - 1)
	{
		byte outNum = tmpAction.substring(10, t - 1).toInt();
		String outStateStr = tmpAction.substring(t + 1);
		int outState = outStateStr.toInt();
		bool pwm_mode = false;
		//Serial.println("Out" + String(outNum) + "=" + String(outState));
		if (outStateStr == "on") outState = OUT_ON;
		else if (outStateStr == "off") outState = OUT_OFF;
		else
		{
			pwm_mode = true;
			outState = outStateStr.toInt();
		}
#ifdef OUTPUT1_ENABLE
		if (outNum == 1)
		{
			out1 = (bool)outState;
			digitalWrite(OUTPUT1_ENABLE, out1);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT2_ENABLE
		if (outNum == 2)
		{
			out2 = (bool)outState;
			if (pwm_mode) analogWrite(OUTPUT2_ENABLE, outState);
			else digitalWrite(OUTPUT2_ENABLE, out2);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT3_ENABLE
		if (outNum == 3)
		{
			out3 = (bool)outState;
			digitalWrite(OUTPUT3_ENABLE, out3);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT4_ENABLE
		if (outNum == 4)
		{
			out4 = (bool)outState;
			digitalWrite(OUTPUT4_ENABLE, out4);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT5_ENABLE
		if (outNum == 5)
		{
			out5 = (bool)outState;
			if (pwm_mode) analogWrite(OUTPUT5_ENABLE, outState);
			else digitalWrite(OUTPUT5_ENABLE, out5);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT6_ENABLE
		if (outNum == 6)
		{
			out6 = (bool)outState;
			if (pwm_mode) analogWrite(OUTPUT6_ENABLE, outState);
			else digitalWrite(OUTPUT6_ENABLE, out6);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT7_ENABLE
		if (outNum == 7)
		{
			out7 = (bool)outState;
			digitalWrite(OUTPUT7_ENABLE, out7);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
#ifdef OUTPUT8_ENABLE
		if (outNum == 8)
		{
			out8 = (bool)outState;
			if (pwm_mode) analogWrite(OUTPUT8_ENABLE, outState);
			else digitalWrite(OUTPUT8_ENABLE, out8);
			str = "Output" + String(outNum) + "=" + String(int(outState));
		}
#endif
		if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
	}
	return str;
}

int getTemperature()
{
	int ppm_avg = 0;
	int temp = -200;
	int humidity = -1;

#ifdef MH_Z19_UART_ENABLE
	temp = sensor.mh_temp;
#endif

#ifdef DHT_ENABLE
	temp = sensor.dht_temp;
#endif

#ifdef AMS2320_ENABLE
	temp = sensor.ams_temp;
#endif

#ifdef HTU21D_ENABLE
	temp = sensor.htu21d_humidity;
#endif

#ifdef DS18B20_ENABLE
	temp = sensor.ds1820_temp;
#endif
	return temp;
}

int getHumidity()
{
	int humidity = -1;

#ifdef DHT_ENABLE
	humidity = sensor.dht_humidity;
#endif

#ifdef AMS2320_ENABLE
	humidity = round(sensor.ams_humidity);
#endif

#ifdef HTU21D_ENABLE
	humidity = sensor.htu21d_temp;
#endif

	return humidity;
}

int getCo2()
{
	int co2_avg = 0;

#if defined(MH_Z19_PPM_ENABLE)
	if (sensor.mh_ppm_co2 > 0)
	{
		co2_ppm_avg[0] = co2_ppm_avg[1];
		co2_ppm_avg[1] = co2_ppm_avg[2];
		co2_ppm_avg[2] = sensor.mh_ppm_co2;
		co2_avg = (co2_ppm_avg[0] + co2_ppm_avg[1] + co2_ppm_avg[2]) / 3;
	}
#endif

#ifdef MH_Z19_UART_ENABLE
	if (sensor.mh_uart_co2 > 0)
	{
		co2_uart_avg[0] = co2_uart_avg[1];
		co2_uart_avg[1] = co2_uart_avg[2];
		co2_uart_avg[2] = sensor.mh_uart_co2;
		co2_avg = (co2_uart_avg[0] + co2_uart_avg[1] + co2_uart_avg[2]) / 3;
	}
#endif

	return co2_avg;
}

void saveLog(sensorDataCollection record)
{
	history_log[history_record_number] = record;
	history_record_number++;

	//if log is full - shift data back
	if (history_record_number >= LOG_SIZE)
	{
		history_record_number--;
		for (int i = 0; i < LOG_SIZE - 2; i++)
		{
			history_log[i] = history_log[i + 1];
			yield();
		}
	}
}