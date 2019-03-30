/*
GPIO 16		 D0 - WAKE, USER button, connect to RST to wake up after sleep
GPIO 5		 D1 - I2C_SCL
GPIO 4		~D2 - I2C_SDA
GPIO 0		 D3 - pullup for FLASH boot, pulldown for UART boot, TM1637_CLK
GPIO 2		 D4 - pullup, Serial1_TX, TM1637_DIO
GPIO 14		~D5 - SoftUART_TX
GPIO 12		~D6 - SoftUART_RX
GPIO 13		 D7 -
GPIO 15		~D8 - pulldown
GPIO 13		 A7 - analog in
*/

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
		 send_mail=address;message
		 send_GScript
		 save_log;
*/

/*
Planned features:
 - SCHEDULE service - restore flags at the start of the day
 - reduce RAM usage (unnecessary String)
 - GoogleDocs service
 - input mode setup (pullup/down)
 - efficient sleep till next event planned with network connectivity restoration after wake up and parameter / timers restore to fire.
 - compile time pin arrangement control
*/


/*
Sensors to be supported:
 - BME280 (I2C)
 - ACS712 (A0)
 - GPRS module via UART/SoftUART(TX_pin, RX_pin, baud_rate); send SMS, make call
*/

//#define SLEEP_ENABLE //connect D0 and EN pins to start contoller after sleep
#define NTP_TIME_ENABLE
#define HTTP_SERVER_ENABLE
#define EVENTS_ENABLE
#define SCHEDULER_ENABLE
//
#define AMS2320_ENABLE
//#define HTU21D_ENABLE
//#define DS18B20_ENABLE
//#define DHT_ENABLE
#define MH_Z19_UART_ENABLE
//#define MH_Z19_PPM_ENABLE
#define TM1637DISPLAY_ENABLE
//#define SSD1306DISPLAY_ENABLE
//
#define TELEGRAM_ENABLE
#define PUSHINGBOX
//#define EMAIL_ENABLE
//#define GSCRIPT
//
//#define ADC_ENABLE
//
//#define INTERRUPT_COUNTER1_ENABLE 5		// D1 - I2C_SCL
//#define INTERRUPT_COUNTER2_ENABLE 4		//~D2 - I2C_SDA
//#define INTERRUPT_COUNTER3_ENABLE 0		// D3 - FLASH, TM1637_CLK, keep HIGH on boot to boot from FLASH or LOW to boot from UART
//#define INTERRUPT_COUNTER4_ENABLE 2		// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INTERRUPT_COUNTER5_ENABLE 14	//~D5 - SoftUART_TX
//#define INTERRUPT_COUNTER6_ENABLE 12	//~D6 - SoftUART_RX
//#define INTERRUPT_COUNTER7_ENABLE 13	// D7
//#define INTERRUPT_COUNTER8_ENABLE 15	//~D8 - keep LOW on boot
//
//#define INPUT1_ENABLE 5					// D1 - I2C_SCL
//#define INPUT2_ENABLE 4					//~D2 - I2C_SDA
//#define INPUT3_ENABLE 0					// D3 - FLASH, TM1637_CLK
//#define INPUT4_ENABLE 2					// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INPUT5_ENABLE 14				//~D5 - SoftUART_TX
//#define INPUT6_ENABLE 12				//~D6 - SoftUART_RX
//#define INPUT7_ENABLE 13				// D7
//#define INPUT8_ENABLE 15				//~D8 - keep LOW on boot
//
//#define OUTPUT1_ENABLE 5				// D1 - I2C_SCL
//#define OUTPUT2_ENABLE 4				//~D2 - I2C_SDA
//#define OUTPUT3_ENABLE 0				// D3 - FLASH, TM1637_CLK
//#define OUTPUT4_ENABLE 2				// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define OUTPUT5_ENABLE 14				//~D5 - SoftUART_TX
//#define OUTPUT6_ENABLE 12				//~D6 - SoftUART_RX
//#define OUTPUT7_ENABLE 13				// D7
//#define OUTPUT8_ENABLE 15				//~D8 - keep LOW on boot

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include "datastructures.h"

#define SWITCH_ON_NUMBER F("1")
#define SWITCH_OFF_NUMBER F("0")
#define SWITCH_ON F("on")
#define SWITCH_OFF F("off")
#define eol F("\r\n")
#define divider F(";")
#define quote F("\"")

// WiFi settings
#define MAX_SRV_CLIENTS 5 //how many clients should be able to telnet to this ESP8266
String ssid = "";
String wiFiPassword = "";
unsigned int telnetPort = 23;
String netInput[MAX_SRV_CLIENTS];
byte wiFiIntendedStatus = WIFI_STOP;
WiFiServer telnetServer(telnetPort);
WiFiClient serverClient[MAX_SRV_CLIENTS];
bool wifiEnable = true;
bool telnetEnable = true;
String deviceName = "";

unsigned long checkSensorLastTime = 0;
unsigned long checkSensorPeriod = 5000;

String uartCommand = "";
bool uartReady = false;

// Log settings for 32 hours of storage
#define LOG_SIZE 192 // Save sensors value each 10 minutes: ( 32hours * 60minutes/hour ) / 10minutes/pcs = 192 pcs
sensorDataCollection history_log[LOG_SIZE];
unsigned int history_record_number = 0;
unsigned long logLastTime = 0;
unsigned long logPeriod = ((60 / (LOG_SIZE / 32)) * 60 * 1000) / 5; // 60minutes/hour / ( 192pcs / 32hours ) * 60seconds/minute * 1000 msec/second
sensorDataCollection sensor;

bool timeIsSet = false;
bool autoReport = false;

// not implemented yet
#ifdef SLEEP_ENABLE
unsigned long reconnectTimeOut = 5000;
bool sleepEnable = false;
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
bool eMailEnable = false;

bool sendMail(String address, String subject, String message)
{
	bool result = false;
	if (eMailEnable && WiFi.status() == WL_CONNECTED)
	{
		SendEmail email(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 5000, false);
		result = email.send(smtpLogin, address, subject, message);
	}
	return result;
}

#endif

// Telegram config
// Need token
const byte telegramUsersNumber = 10;
#ifdef TELEGRAM_ENABLE
//#include <WiFiClientSecure.h>
#include "CTBot.h"
CTBot myBot;
const int telegramMessageMaxSize = 300;
typedef struct telegramMessage
{
	byte retries;
	int64_t user;
	String message;
};

String telegramToken = "";
int64_t telegramUsers[telegramUsersNumber];
const byte telegramMessageBufferSize = 10;
telegramMessage telegramOutboundBuffer[telegramMessageBufferSize];
byte telegramOutboundBufferPos = 0;
int telegramMessageDelay = 2000;
byte telegramRetries = 3;
unsigned long telegramLastTime = 0;
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

bool sendToTelegramServer(int64_t user, String str)
{
	bool result = false;
	if (telegramEnable && WiFi.status() == WL_CONNECTED)
	{
		result = myBot.testConnection();
		if (result)
		{
			String tmpStr = "";
			tmpStr.reserve(str.length() * 3);
			tmpStr = str;
			tmpStr.replace(F("\r"), "");
			tmpStr.replace(F("\n"), F("%0a"));
			tmpStr.replace(F("\t"), F("%09"));
			tmpStr.replace(F(" "), F("%20"));
			tmpStr.replace(F("["), F("%5b"));
			tmpStr.replace(F("]"), F("%5d"));
			tmpStr.replace(F("?"), F("%3f"));
			tmpStr.replace(F("@"), F("%40"));
			tmpStr.replace(F("&"), F("%26"));
			tmpStr.replace(F("\\"), F("5c"));
			tmpStr.replace(F("/"), F("%2f"));
			Serial.print(F("Sending: "));
			Serial.println(tmpStr);
			result = myBot.sendMessage(user, tmpStr);
			if (!result) Serial.println(F("failed"));
			else
			{
				Serial.println(F("success"));
				result = true;
			}
		}
		else Serial.println(F("TELEGRAM connection error"));
	}
	else Serial.println(F("WiFi not connected"));
	return result;
}

void addMessageToTelegramOutboundBuffer(int64_t user, String message, byte retries)
{
	if (telegramOutboundBufferPos >= telegramMessageBufferSize)
	{
		for (int i = 0; i < telegramMessageBufferSize - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
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
	for (int i = 0; i < telegramOutboundBufferPos; i++)
	{
		if (i < telegramMessageBufferSize - 1) telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		else
		{
			telegramOutboundBuffer[i].message = "";
			telegramOutboundBuffer[i].user = 0;
		}
	}
	telegramOutboundBufferPos--;
}

void sendToTelegram(int64_t user, String message, byte retries)
{
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= telegramMessageMaxSize)
		{
			addMessageToTelegramOutboundBuffer(user, message.substring(0, telegramMessageMaxSize), retries);
			message = message.substring(telegramMessageMaxSize);
		}
		else
		{
			addMessageToTelegramOutboundBuffer(user, message, retries);
			message = "";
		}
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
			Serial.print(F("Received TELEGRAM message ["));
			Serial.print(msg.sender.username);
			Serial.print(F("]: "));
			Serial.println(msg.text);

			//assign 1st sender as admin if admin is empty
			if (telegramUsers[0] == 0)
			{
				telegramUsers[0] = msg.sender.id;
				Serial.print(F("admin set to ["));
				Serial.print(uint64ToString(telegramUsers[0]));
				Serial.println(F("]"));
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
			if (isRegisteredUser && msg.text.length() > 0)
			{
				Serial.println(F("User is registered"));
				sendToTelegram(msg.sender.id, deviceName + ": " + msg.text, telegramRetries);
				String str = processCommand(msg.text, TELEGRAM_CHANNEL, isAdmin);
				Serial.print(F("Command answer: "));
				Serial.println(str);
				sendToTelegram(msg.sender.id, deviceName + ": " + str, telegramRetries);
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

#endif

// not implemented yet
// Google script config
// Need token
#ifdef GSCRIPT
#include <HTTPSRedirect.h>
String gScriptId = "";
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";
//String[] valuesNames = {"?tag", "?value="};

const int httpsPort = 443;
HTTPSRedirect gScriptClient(httpsPort);
bool gScriptEnable = false;

bool sendValueToGoogle(String value)
{
	bool flag = false;
	if (gScriptEnable && WiFi.status() == WL_CONNECTED)
	{
		HTTPSRedirect* client = nullptr;
		client = new HTTPSRedirect(httpsPort);
		client->setInsecure();
		client->setPrintResponseBody(true);
		client->setContentTypeHeader("application/json");

		for (int i = 0; i < 5; i++)
		{
			int retval = client->connect(host, httpsPort);
			if (retval == 1)
			{
				flag = true;
				break;
			}
			else Serial.println(F("Connection failed. Retrying..."));
		}

		if (!flag)
		{
			Serial.print(F("Could not connect to server: "));
			Serial.println(host);
			Serial.println(F("Exiting..."));
			return flag;
		}

		if (client->setFingerprint(fingerprint))
		{
			Serial.println(F("Certificate match."));
		}
		else
		{
			Serial.println(F("Certificate mis-match"));
		}

		// Send data to Google Sheets
		String url = F("/macros/s/");
		url += gScriptId;
		url += F("/exec");
		flag = client->POST(url, host, value, false);
		delete client;
		client = nullptr;
	}
	return flag;
}
#endif

// Pushingbox config
// PushingBox device ID
#ifdef PUSHINGBOX
#define PUSHINGBOX_SUBJECT F("device_name")
String pushingBoxId = "";
String pushingBoxParameter = "";
const int pushingBoxMessageMaxSize = 1000;
const char* pushingBoxServer = "api.pushingbox.com";
bool pushingBoxEnable = false;

bool sendToPushingBoxServer(String message)
{
	bool flag = false;
	if (pushingBoxEnable && WiFi.status() == WL_CONNECTED)
	{
		WiFiClient client;

		//Serial.println("- connecting to pushing server ");
		if (client.connect(pushingBoxServer, 80))
		{
			//Serial.println("- succesfully connected");

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

			//Serial.println("- sending data...");

			client.print(F("POST /pushingbox HTTP/1.1\n"));
			client.print(F("Host: "));
			client.print(String(pushingBoxServer));
			client.print(F("\nConnection: close\n"));
			client.print(F("Content-Type: application/x-www-form-urlencoded\n"));
			client.print(F("Content-Length: "));
			client.print(postStr.length());
			client.print(F("\n\n"));
			client.print(postStr);
			//Serial.println(postStr);
		}
		client.stop();
		//Serial.println("- stopping the client");
	}
	return flag;
}

bool sendToPushingBox(String message)
{
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= pushingBoxMessageMaxSize)
		{
			sendToPushingBoxServer(message.substring(0, pushingBoxMessageMaxSize));
			message = message.substring(pushingBoxMessageMaxSize);
		}
		else
		{
			sendToPushingBoxServer(message);
			message = "";
		}
	}
}
#endif

// HTTP server
// Need port 
#ifdef HTTP_SERVER_ENABLE
#include <ESP8266WebServer.h>
unsigned int httpPort = 80;
ESP8266WebServer http_server(httpPort);
bool httpServerEnable = false;

String HTML_footer()
{
	return F("<br />\r\n</html>\r\n");
}

void handleRoot()
{
	String str = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: ");
	str += String(round(checkSensorPeriod / 1000));
	str += F("\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
	str += ParseSensorReport(history_log[history_record_number - 1], F("<br>"));
	http_server.sendContent(str);
}

void handleNotFound()
{
	String message = F("File Not Found\n\nURI: ");
	message += http_server.uri();
	message += F("\nMethod: ");
	message += (http_server.method() == HTTP_GET) ? F("GET") : F("POST\nArguments: ");
	message += http_server.args();
	message += eol;
	for (uint8_t i = 0; i < http_server.args(); i++)
	{
		message += F(" ");
		message += http_server.argName(i);
		message += F(": ");
		message += http_server.arg(i);
		message += eol;
		yield();
	}
	http_server.send(404, F("text/plain"), message);
}
#endif

// NTP Server
// Need server address, local time zone
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>
#define NTP_PACKET_SIZE 48 // NTP time is in the first 48 bytes of message
IPAddress timeServer(129, 6, 15, 28); //129.6.15.28 = time-a.nist.gov
WiFiUDP Udp;
int timeZone = +3;     // Moscow Time
const unsigned int localPort = 8888;  // local port to listen for UDP packets
int ntpRefreshDelay = 180;
bool NTPenable = false;

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
	byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
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
	//Serial.print(F("NTP Request..."));
	sendNTPpacket(timeServer);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			//Serial.println(F("OK"));
			byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			timeIsSet = true;
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
	//Serial.println(F("failed"));
	return 0; // return 0 if unable to get the time
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
PINS: D7 - OneWire
pin 1 to +3.3V/+5V
pin 2 to whatever your OneWire is
pin 3 to GROUND
5-10K pullup resistor from pin 2 (data) to pin 1 (power) of the sensor
*/
#ifdef DS18B20_ENABLE
#include <OneWire.h>
#include "DallasTemperature.h"
#define ONE_WIRE_PIN 02 //D4
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
		ppm = (256 * (unsigned int)response[2]) + (unsigned int)response[3];
		mhtemp_s = int(response[4]) - 40;
		//Serial.println("PPM: " + String(ppm) + " / CO2: " + String(mhtemp_s));
	}
	else
	{
		//Serial.println("CRC error: " + String(crc) + " / " + String(response[8]));
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
			//Serial.println(F("Failed to read PPM"));
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
byte displayState = 0;
#endif

#ifdef SSD1306DISPLAY_ENABLE
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
const uint8_t* font = fixed_bold10x15;  // 10x15 pix
//const uint8_t* font = cp437font8x8;  // 8*8 pix
//const uint8_t* font = Adafruit5x7;  // 5*7 pix
SSD1306AsciiWire oled;
#ifndef TM1637DISPLAY_ENABLE
unsigned long displaySwitchedLastTime = 0;
unsigned long displaySwitchPeriod = 3000;
#endif
#endif

#ifdef ADC_ENABLE
//ADC_MODE(ADC_VCC);

int getAdc()
{
	float averageValue = 0;
	int measurementsToAverage = 16;
	for (int i = 0; i < measurementsToAverage; ++i)
	{
		averageValue += (float)analogRead(A0);
		delay(1);
	}
	averageValue /= measurementsToAverage;
	return int(averageValue);
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
int out1 = OUT_OFF;
#endif
#ifdef OUTPUT2_ENABLE
int out2 = OUT_OFF;
#endif
#ifdef OUTPUT3_ENABLE
int out3 = OUT_OFF;
#endif
#ifdef OUTPUT4_ENABLE
int out4 = OUT_OFF;
#endif
#ifdef OUTPUT5_ENABLE
int out5 = OUT_OFF;
#endif
#ifdef OUTPUT6_ENABLE
int out6 = OUT_OFF;
#endif
#ifdef OUTPUT7_ENABLE
int out7 = OUT_OFF;
#endif
#ifdef OUTPUT8_ENABLE
int out8 = OUT_OFF;
#endif

const byte eventsNumber = 10;
#ifdef EVENTS_ENABLE
bool eventsFlags[eventsNumber];
bool eventsEnable = false;

String getEvent(byte n)
{
	int singleEventLength = EVENTS_TABLE_size / eventsNumber;
	String eventStr = readConfigString((EVENTS_TABLE_addr + n * singleEventLength), singleEventLength);
	eventStr.trim();
	return eventStr;
}

void processEvent(String event, byte eventNum)
{
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
		int t = condition.indexOf('=');
		if (t >= 6 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(5, t).toInt();
			String outStateStr = condition.substring(t + 1);
			bool outState = OUT_OFF;
			if (outStateStr != F("c"))
			{
				if (outStateStr == SWITCH_ON) outState = OUT_ON;
				else if (outStateStr == SWITCH_OFF) outState = OUT_OFF;
				else outState = outStateStr.toInt();
			}
			//Serial.println("CheckIn" + String(outNum) + "=" + String(outState));
			const String cTxt = F("c");
#ifdef INPUT1_ENABLE
			if (outNum == 1)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in1 != digitalRead(INPUT1_ENABLE))
					{
						in1 = digitalRead(INPUT1_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT1_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT2_ENABLE
			if (outNum == 2)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in2 != digitalRead(INPUT2_ENABLE))
					{
						in2 = digitalRead(INPUT2_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT2_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT3_ENABLE
			if (outNum == 3)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in3 != digitalRead(INPUT3_ENABLE))
					{
						in3 = digitalRead(INPUT3_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT3_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT4_ENABLE
			if (outNum == 4)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in4 != digitalRead(INPUT4_ENABLE))
					{
						in4 = digitalRead(INPUT4_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT4_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT5_ENABLE
			if (outNum == 5)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in5 != digitalRead(INPUT5_ENABLE))
					{
						in5 = digitalRead(INPUT5_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT5_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT6_ENABLE
			if (outNum == 6)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in6 != digitalRead(INPUT6_ENABLE))
					{
						in6 = digitalRead(INPUT6_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT6_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT7_ENABLE
			if (outNum == 7)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in7 != digitalRead(INPUT7_ENABLE))
					{
						in7 = digitalRead(INPUT7_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT7_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INPUT8_ENABLE
			if (outNum == 8)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (in8 != digitalRead(INPUT8_ENABLE))
					{
						in8 = digitalRead(INPUT8_ENABLE);
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(INPUT8_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
		}
	}
	else if (condition.startsWith(F("output")))
	{
		int t = condition.indexOf('=');
		if (t >= 7 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(6, t).toInt();
			String outStateStr = condition.substring(t + 1);
			int outState = OUT_OFF;
			if (outStateStr != "c")
			{
				if (outStateStr == SWITCH_ON) outState = OUT_ON;
				else if (outStateStr == SWITCH_OFF) outState = OUT_OFF;
				else outState = outStateStr.toInt();
			}
			//Serial.println("CheckOut" + String(outNum) + "=" + String(outState));
			const String cTxt = F("c");
#ifdef OUTPUT1_ENABLE
			if (outNum == 1)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out1 != digitalRead(OUTPUT1_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT1_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT2_ENABLE
			if (outNum == 2)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out2 != digitalRead(OUTPUT2_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT2_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT3_ENABLE
			if (outNum == 3)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out3 != digitalRead(OUTPUT3_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT3_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT4_ENABLE
			if (outNum == 4)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out4 != digitalRead(OUTPUT4_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT4_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT5_ENABLE
			if (outNum == 5)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out5 != digitalRead(OUTPUT5_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT5_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT6_ENABLE
			if (outNum == 6)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out6 != digitalRead(OUTPUT6_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT6_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT7_ENABLE
			if (outNum == 7)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out7 != digitalRead(OUTPUT7_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT7_ENABLE))  ProcessAction(event, eventNum, true);
			}
#endif
#ifdef OUTPUT8_ENABLE
			if (outNum == 8)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (outStateStr == cTxt)
				{
					if (out8 != digitalRead(OUTPUT8_ENABLE))
					{
						ProcessAction(event, eventNum, true);
					}
				}
				else if (outState == digitalRead(OUTPUT8_ENABLE)) ProcessAction(event, eventNum, true);
			}
#endif
		}
	}
	else if (condition.startsWith(F("counter")))
	{
		int t = condition.indexOf('>');
		if (t >= 8 && t < condition.length() - 1)
		{
			byte outNum = condition.substring(7, t).toInt();
			String outStateStr = condition.substring(t + 1);
			int outState = outStateStr.toInt();
			//Serial.println("Counter" + String(outNum) + "=" + String(outState));
#ifdef INTERRUPT_COUNTER1_ENABLE
			if (outNum == 1)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount1 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
			if (outNum == 2)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount2 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
			if (outNum == 3)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount3 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
			if (outNum == 4)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount4 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
			if (outNum == 5)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount5 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
			if (outNum == 6)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount6 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
			if (outNum == 7)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount7 > outState) ProcessAction(event, eventNum, true);
			}
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
			if (outNum == 8)
			{
				Serial.print(F("Event #"));
				Serial.print(String(eventNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (intCount8 > outState) ProcessAction(event, eventNum, true);
			}
#endif
		}
	}

#ifdef ADC_ENABLE
	else if (condition.startsWith(F("adc")) && condition.length() > 4)
	{
		int adcValue = getAdc();
		char operation = condition[3];
		int value = condition.substring(4).toInt();
		//Serial.println("Evaluating: " + String(adcValue) + operation + String(value));
		if (operation == '>' && adcValue > value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
		else if (operation == '<' && adcValue < value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#if defined(DS18B20_ENABLE) || defined(MH_Z19_UART_ENABLE) || defined(MH_Z19_PPM_ENABLE) || defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE)
	else if (condition.startsWith(F("temperature")) && condition.length() > 12)
	{
		char oreration = condition[11];
		int value = condition.substring(12).toInt();
		if (oreration == '>' && getTemperature() > value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && getTemperature() < value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE)
	else if (condition.startsWith(F("humidity")) && condition.length() > 9)
	{
		char oreration = condition[8];
		int value = condition.substring(9).toInt();
		if (oreration == '>' && getHumidity() > value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && getHumidity() < value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
	}
#endif

#if defined(MH_Z19_UART_ENABLE)|| defined(MH_Z19_PPM_ENABLE)
	else if (condition.startsWith(F("co2")) && condition.length() > 4)
	{
		char oreration = condition[3];
		int value = condition.substring(4).toInt();
		if (oreration == '>' && getCo2() > value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
		else if (oreration == '<' && getCo2() < value)
		{
			Serial.print(F("Event #"));
			Serial.print(String(eventNum));
			Serial.print(F(" started: \""));
			Serial.println(condition);
			ProcessAction(event, eventNum, true);
		}
	}
#endif
}
#endif

// not tested
const byte schedulesNumber = 10;
#ifdef SCHEDULER_ENABLE
bool schedulesFlags[schedulesNumber];
bool schedulerEnable = false;

String getSchedule(byte n)
{
	int singleSchedluleLength = SCHEDULER_TABLE_size / schedulesNumber;
	String scheduleStr = readConfigString((SCHEDULER_TABLE_addr + n * singleSchedluleLength), singleSchedluleLength);
	scheduleStr.trim();
	return scheduleStr;
}

void processSchedule(String schedule, byte scheduleNum)
{
	if (schedulesFlags[scheduleNum]) return;
	//conditions:
	//	daily - daily@hh:mm;action1;action2;...
	//	weekly - weekly@week_day.hh:mm;action1;action2;...
	//	monthly - monthly@month_day.hh:mm;action1;action2;...
	//	date - date@yyyy.mm.dd.hh:mm;action1;action2;...
	int t = schedule.indexOf('@');
	int t1 = schedule.indexOf(';');
	if (t <= 4 || t1 < 11 || t1 < t) return;
	String condition = schedule.substring(0, t);
	condition.trim();
	condition.toLowerCase();
	String time = schedule.substring(t + 1, t1);
	time.trim();
	time.toLowerCase();
	schedule = schedule.substring(t1 + 1);
	if (condition.startsWith(F("daily@")) && t == 5)
	{
		if (time.length() == 5)
		{
			int _hr = time.substring(0, 2).toInt();
			int _min = time.substring(3, 5).toInt();
			if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
			{
				Serial.print(F("Schedule #"));
				Serial.print(String(scheduleNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
			}
		}
	}
	else if (condition.startsWith(F("weekly@")) && t == 6)
	{
		if (time.length() == 7)
		{
			int _weekDay = time.substring(0, 1).toInt();
			int _hr = time.substring(2, 4).toInt();
			int _min = time.substring(5, 7).toInt();
			if (_weekDay > 0 && _weekDay < 8 && _hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
			{
				Serial.print(F("Schedule #"));
				Serial.print(String(scheduleNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (weekday() == _weekDay && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
			}
		}
	}
	else if (condition.startsWith(F("monthly@")) && t == 7)
	{
		if (time.length() == 8)
		{
			int _monthDay = time.substring(0, 2).toInt();
			int _hr = time.substring(3, 5).toInt();
			int _min = time.substring(6, 8).toInt();
			if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
			{
				Serial.print(F("Schedule #"));
				Serial.print(String(scheduleNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (day() == _monthDay && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
			}
		}
	}
	else if (condition.startsWith(F("date@")) && t == 4)
	{
		if (time.length() == 16)
		{
			int _yr = time.substring(0, 4).toInt();
			int _month = time.substring(5, 7).toInt();
			int _day = time.substring(8, 10).toInt();
			int _hr = time.substring(11, 13).toInt();
			int _min = time.substring(14, 16).toInt();
			if (_hr >= 0 && _hr < 24 && _min >= 0 && _min < 60)
			{
				Serial.print(F("Schedule #"));
				Serial.print(String(scheduleNum));
				Serial.print(F(" started: \""));
				Serial.println(condition);
				if (year() == _yr && month() == _month && day() == _day && hour() >= _hr && minute() >= _min) ProcessAction(schedule, scheduleNum, false);
			}
		}
	}
}
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
	wiFiPassword = readConfigString(PASS_addr, PASS_size);
	telnetPort = readConfigString(TELNET_PORT_addr, TELNET_PORT_size).toInt();
	checkSensorPeriod = readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size).toInt();
	deviceName = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);

	// init WiFi
	WiFi.begin(ssid.c_str(), wiFiPassword.c_str());
	wiFiIntendedStatus = WIFI_CONNECTING;
	Serial.println(F("\r\nConnecting to "));
	Serial.println(ssid);
	unsigned long reconnectTimeStart = millis();
	/*while (WiFi.status() != WL_CONNECTED && millis() - reconnectTimeStart < 5000)
	{
		delay(50);
		if (WiFi.status() == WL_CONNECTED) break;
		yield();
	}*/
	telnetServer.begin(telnetPort);
	telnetServer.setNoDelay(true);

#ifdef SLEEP_ENABLE
	if (readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size) == SWITCH_ON_NUMBER) sleepEnable = true;
#endif

#ifdef EVENTS_ENABLE
	if (readConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size) == SWITCH_ON_NUMBER) eventsEnable = true;
#endif

#ifdef SCHEDULER_ENABLE
	/*int b = SCHEDULER_TABLE_size / schedulesNumber;
	for (int i = 0; i < schedulesNumber; i++)
	{
		schedulesActions[i] = readConfigString((TELEGRAM_USERS_TABLE_addr + i * b), b);
	}*/
	if (readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size) == SWITCH_ON_NUMBER) schedulerEnable = true;
#endif

#ifdef EMAIL_ENABLE
	smtpServerAddress = readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	smtpServerPort = readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size).toInt();
	smtpLogin = readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	smtpPassword = readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	smtpTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);
	if (readConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size) == SWITCH_ON_NUMBER) eMailEnable = true;
#endif

#ifdef TELEGRAM_ENABLE
	telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	for (int i = 0; i < telegramUsersNumber; i++)
	{
		telegramUsers[i] = StringToUint64(readConfigString((TELEGRAM_USERS_TABLE_addr + i * m), m));
	}
	if (readConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size) == SWITCH_ON_NUMBER) telegramEnable = true;
	if (telegramEnable)
	{
		//myBot.wifiConnect(ssid, wiFiPassword);
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif

#ifdef GSCRIPT
	String gScriptId = readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	if (readConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size) == SWITCH_ON_NUMBER) gScriptEnable = true;
#endif

#ifdef PUSHINGBOX
	pushingBoxId = readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
	pushingBoxParameter = readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	if (readConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size) == SWITCH_ON_NUMBER) pushingBoxEnable = true;
#endif

#ifdef HTTP_SERVER_ENABLE
	httpPort = readConfigString(HTTP_PORT_addr, HTTP_PORT_size).toInt();
	if (readConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size) == SWITCH_ON_NUMBER) httpServerEnable = true;
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
	if (readConfigString(ENABLE_NTP_addr, ENABLE_NTP_size) == SWITCH_ON_NUMBER) NTPenable = true;
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
		Serial.println(F("Couldn't find AMS2320 sensor!"));
	}
#endif

#ifdef HTU21D_ENABLE
	if (!htu21d.begin())
	{
		Serial.println(F("Couldn't find HTU21D sensor!"));
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

#ifdef SSD1306DISPLAY_ENABLE
	Wire.begin();
	Wire.setClock(400000L);
	oled.begin(&Adafruit128x64, I2C_ADDRESS);
	oled.setFont(font);
#endif

	String outStateStr = "";
	outStateStr.reserve(5);
	const String stateOne = F("1");
	const String stateTwo = F("2");
	const String stateThree = F("3");
#ifdef INTERRUPT_COUNTER1_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode1 = RISING;
	else if (outStateStr == stateTwo) intMode1 = FALLING;
	else if (outStateStr == stateThree) intMode1 = CHANGE;
	pinMode(INTERRUPT_COUNTER1_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER1_ENABLE, int1count, intMode1);
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode2 = RISING;
	else if (outStateStr == stateTwo) intMode2 = FALLING;
	else if (outStateStr == stateThree) intMode2 = CHANGE;
	pinMode(INTERRUPT_COUNTER2_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER2_ENABLE, int2count, intMode2);
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode3 = RISING;
	else if (outStateStr == stateTwo) intMode3 = FALLING;
	else if (outStateStr == stateThree) intMode3 = CHANGE;
	pinMode(INTERRUPT_COUNTER3_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER3_ENABLE, int3count, intMode3);
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode4 = RISING;
	else if (outStateStr == stateTwo) intMode4 = FALLING;
	else if (outStateStr == stateThree) intMode4 = CHANGE;
	pinMode(INTERRUPT_COUNTER4_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER4_ENABLE, int4count, intMode4);
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode5 = RISING;
	else if (outStateStr == stateTwo) intMode5 = FALLING;
	else if (outStateStr == stateThree) intMode5 = CHANGE;
	pinMode(INTERRUPT_COUNTER5_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER5_ENABLE, int5count, intMode5);
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode6 = RISING;
	else if (outStateStr == stateTwo) intMode6 = FALLING;
	else if (outStateStr == stateThree) intMode6 = CHANGE;
	pinMode(INTERRUPT_COUNTER6_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER6_ENABLE, int6count, intMode6);
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode7 = RISING;
	else if (outStateStr == stateTwo) intMode7 = FALLING;
	else if (outStateStr == stateThree) intMode7 = CHANGE;
	pinMode(INTERRUPT_COUNTER7_ENABLE, INPUT_PULLUP);
	attachInterrupt(INTERRUPT_COUNTER7_ENABLE, int7count, intMode7);
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	outStateStr = readConfigString(INT1_MODE_addr, INT1_MODE_size);
	if (outStateStr == stateOne) intMode8 = RISING;
	else if (outStateStr == stateTwo) intMode8 = FALLING;
	else if (outStateStr == stateThree) intMode8 = CHANGE;
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
	pinMode(OUTPUT1_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT1_INIT_addr, OUT1_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out1 = (int)OUT_ON;
		digitalWrite(OUTPUT1_ENABLE, out1);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out1 = (int)OUT_OFF;
		digitalWrite(OUTPUT8_ENABLE, out1);
	}
	else
	{
		out1 = outStateStr.toInt();
		analogWrite(OUTPUT1_ENABLE, out1);
	}
#endif
#ifdef OUTPUT2_ENABLE
	pinMode(OUTPUT2_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT2_INIT_addr, OUT2_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out2 = (int)OUT_ON;
		digitalWrite(OUTPUT2_ENABLE, out2);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out2 = (int)OUT_OFF;
		digitalWrite(OUTPUT2_ENABLE, out2);
	}
	else
	{
		out2 = outStateStr.toInt();
		analogWrite(OUTPUT2_ENABLE, out2);
	}
#endif
#ifdef OUTPUT3_ENABLE
	pinMode(OUTPUT3_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT3_INIT_addr, OUT3_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out3 = (int)OUT_ON;
		digitalWrite(OUTPUT3_ENABLE, out3);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out3 = (int)OUT_OFF;
		digitalWrite(OUTPUT3_ENABLE, out3);
	}
	else
	{
		out3 = outStateStr.toInt();
		analogWrite(OUTPUT3_ENABLE, out3);
	}
#endif
#ifdef OUTPUT4_ENABLE
	pinMode(OUTPUT4_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT4_INIT_addr, OUT4_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out4 = (int)OUT_ON;
		digitalWrite(OUTPUT4_ENABLE, out4);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out4 = (int)OUT_OFF;
		digitalWrite(OUTPUT4_ENABLE, out4);
	}
	else
	{
		out4 = outStateStr.toInt();
		analogWrite(OUTPUT4_ENABLE, out4);
	}
#endif
#ifdef OUTPUT5_ENABLE
	pinMode(OUTPUT5_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT5_INIT_addr, OUT5_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out5 = (int)OUT_ON;
		digitalWrite(OUTPUT5_ENABLE, out5);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out5 = (int)OUT_OFF;
		digitalWrite(OUTPUT5_ENABLE, out5);
	}
	else
	{
		out5 = outStateStr.toInt();
		analogWrite(OUTPUT5_ENABLE, out5);
	}
#endif
#ifdef OUTPUT6_ENABLE
	pinMode(OUTPUT6_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT6_INIT_addr, OUT6_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out6 = (int)OUT_ON;
		digitalWrite(OUTPUT6_ENABLE, out6);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out6 = (int)OUT_OFF;
		digitalWrite(OUTPUT6_ENABLE, out6);
	}
	else
	{
		out6 = outStateStr.toInt();
		analogWrite(OUTPUT6_ENABLE, out6);
	}
#endif
#ifdef OUTPUT7_ENABLE
	pinMode(OUTPUT7_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT7_INIT_addr, OUT7_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out7 = (int)OUT_ON;
		digitalWrite(OUTPUT7_ENABLE, out7);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out7 = (int)OUT_OFF;
		digitalWrite(OUTPUT7_ENABLE, out7);
	}
	else
	{
		out7 = outStateStr.toInt();
		analogWrite(OUTPUT7_ENABLE, out7);
	}
#endif
#ifdef OUTPUT8_ENABLE
	pinMode(OUTPUT8_ENABLE, OUTPUT);
	outStateStr = readConfigString(OUT8_INIT_addr, OUT8_INIT_size);
	if (outStateStr == SWITCH_ON)
	{
		out8 = (int)OUT_ON;
		digitalWrite(OUTPUT8_ENABLE, out8);
	}
	else if (outStateStr == SWITCH_OFF)
	{
		out8 = (int)OUT_OFF;
		digitalWrite(OUTPUT8_ENABLE, out8);
	}
	else
	{
		out8 = outStateStr.toInt();
		analogWrite(OUTPUT8_ENABLE, out8);
	}
#endif
	sensor = collectData();
}

void loop()
{
	//check if connection is present and reconnect
	if (wiFiIntendedStatus != WIFI_STOP)
	{
		unsigned long reconnectTimeStart = millis();
		if (wiFiIntendedStatus == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED)
		{
			Serial.println(F("Connection lost."));
			wiFiIntendedStatus = WIFI_CONNECTING;
		}
		if (wiFiIntendedStatus != WIFI_CONNECTED && WiFi.status() == WL_CONNECTED)
		{
			Serial.print(F("Connected to \""));
			Serial.print(String(ssid));
			Serial.println(F("\" AP."));
			Serial.print(F("Use \'telnet "));
			Serial.print(WiFi.localIP());
			Serial.print(F(":"));
			Serial.print(String(telnetPort));
			Serial.println(F("\'"));
			wiFiIntendedStatus = WIFI_CONNECTED;
		}

#ifdef SLEEP_ENABLE
		//try to connect WiFi to get commands
		if (sleepEnable && wiFiIntendedStatus == WIFI_CONNECTING && WiFi.status() != WL_CONNECTED && millis() - reconnectTimeStart < reconnectTimeOut)
		{
			Serial.println(F("Reconnecting..."));
			while (WiFi.status() != WL_CONNECTED && millis() - reconnectTimeStart < reconnectTimeOut)
			{
				delay(50);
				if (WiFi.status() == WL_CONNECTED) wiFiIntendedStatus = WIFI_CONNECTED;
				yield();
			}
		}
#endif
	}

	//check if it's time to get sensors value
	if (millis() - checkSensorLastTime > checkSensorPeriod)
	{
		checkSensorLastTime = millis();
		String str = deviceName;
		str += F(": ");
		sensor = collectData();
		if (autoReport)
		{
			str += ParseSensorReport(sensor, eol);
			Serial.println(str);
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
				if (telegramEnable)
				{
					for (int i = 0; i < telegramUsersNumber; i++)
					{
						if (telegramUsers[i] > 0)
						{
							sendToTelegram(telegramUsers[i], str, telegramRetries);
						}
					}
				}
#endif

#ifdef GSCRIPT
				if (gScriptEnable) sendValueToGoogle(str);
#endif
			}
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
			if (eMailEnable && WiFi.status() == WL_CONNECTED && history_record_number > 0)
			{
				bool sendOk = true;
				int n = 0;
				String tmpStr = ParseSensorReport(history_log[n], divider);
				tmpStr += eol;
				n++;
				while (n < history_record_number)
				{
					int freeMem = ESP.getFreeHeap() / 3;
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number) break;

						Serial.print(F("history_record_number="));
						Serial.println(String(history_record_number));
						Serial.print(F("freeMem="));
						Serial.println(String(freeMem));
						Serial.print(F("n="));
						Serial.println(String(n));

						tmpStr += ParseSensorReport(history_log[n], divider);
						tmpStr += eol;
						yield();
					}
					sendOk = sendMail(smtpTo, deviceName + " log", tmpStr);
					tmpStr = "";
					yield();
					if (sendOk)
					{
						Serial.println(F("Log sent to EMail"));
						history_record_number = 0;
					}
					else
					{
						Serial.print(F("Error sending log to EMail "));
						Serial.print(String(history_record_number));
						Serial.println(F(" records."));
					}
				}
			}
		}
#endif
	}

	//check UART for data
	while (Serial.available() && !uartReady)
	{
		char c = Serial.read();
		if (c == '\r' || c == '\n') uartReady = true;
		else uartCommand += (char)c;
	}

	//process data from UART
	if (uartReady)
	{
		uartReady = false;
		if (uartCommand.length() > 0)
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
						Serial.println(F("Client connected: #"));
						Serial.println(String(i));
						break;
					}
				}
				//no free/disconnected spot so reject
				if (i >= MAX_SRV_CLIENTS)
				{
					WiFiClient serverClient = telnetServer.available();
					serverClient.stop();
					Serial.print(F("No more slots. Client rejected."));
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
				if (netInput[i].length() > 0)
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
		if (telegramEnable && millis() - telegramLastTime > telegramMessageDelay && WiFi.status() == WL_CONNECTED)
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
		if (displayState == 0)
		{
			int co2_avg = getCo2();
			if (co2_avg > -1000) display.showNumberDec(co2_avg, false);
			else displayState++;
		}
		//show Temperature
		if (displayState == 1)
		{
			int temp = getTemperature();
			if (temp > -1000)
			{
				display.showNumberDec(temp, false, 3, 0);
				display.setSegments(SEG_DEGREE, 1, 3);
			}
			else displayState++;
		}
		//show Humidity
		if (displayState == 2)
		{
			int humidity = getHumidity();
			if (humidity > -1000)
			{
				display.showNumberDec(humidity, false, 3, 0);
				display.setSegments(SEG_HUMIDITY, 1, 3);
			}
			else displayState++;
		}
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
	}
#endif

#ifdef SSD1306DISPLAY_ENABLE
	if (millis() - displaySwitchedLastTime > displaySwitchPeriod)
	{
		displaySwitchedLastTime = millis();

		oled.clear();
		String tmpStr = "";
		byte n = 0;
		//show WiFi status
		//if (WiFi.status() == WL_CONNECTED) tmpStr += F("Wi-Fi: ok\r\n");
		//else tmpStr += F("Wi-Fi: fail\r\n");
		n++;

		//show clock
		if (timeIsSet)
		{
			int _hr = hour();
			int _min = minute();
			if (_hr < 10) tmpStr += F("0");
			tmpStr += String(_hr);
			tmpStr += F(":");
			if (_min < 10) tmpStr += F("0");
			tmpStr += String(_min);
			tmpStr += F("\r\n");
			n++;
		}

		//show Temperature
		int temp = getTemperature();
		if (temp > -1000)
		{
			tmpStr += F("T: ");
			tmpStr += String(temp);
			tmpStr += F("\r\n");
			n++;
		}

		//show Humidity
		int humidity = getHumidity();
		if (humidity > -1000)
		{
			tmpStr += F("H: ");
			tmpStr += String(humidity);
			tmpStr += F("%\r\n");
			n++;
		}

		//show CO2
		int co2_avg = getCo2();
		//Serial.println("CO2 display: " + String(co2_avg));
		if (co2_avg > -1000)
		{
			tmpStr += F("CO2: ");
			tmpStr += String(co2_avg);
			tmpStr += "\r\n";
			n++;
		}

		if (n < 3) oled.set2X();
		else oled.set1X();
		oled.print(tmpStr);
	}
#endif

#ifdef EVENTS_ENABLE
	if (eventsEnable)
	{
		for (int i = 0; i < eventsNumber; i++)
		{
			String eventStr = getEvent(i);
			if (eventStr.length() > 0)
			{
				processEvent(eventStr, i);
			}
		}
	}
#endif

#ifdef SCHEDULER_ENABLE
	if (schedulerEnable && timeIsSet)
	{
		// reset flags at the start of the day
		if (false)
		{
			for (int i = 0; i < schedulesNumber; i++)schedulesFlags[schedulesNumber];
		}

		// Process all schedules from EEPROM
		for (int i = 0; i < schedulesNumber; i++)
		{
			String scheduleStr = getSchedule(i);
			if (scheduleStr.length() > 0)
			{
				processSchedule(scheduleStr, i);
			}
		}
	}
#endif

#ifdef SLEEP_ENABLE
	if (sleepEnable)
	{
		unsigned long int ms = millis() - checkSensorLastTime;
		if (ms < checkSensorPeriod)
		{
			Serial.print(F("Sleep "));
			Serial.print(String(checkSensorPeriod - ms));
			Serial.println(F("ms"));
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
	String str = F("Device name: \"");
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += quote;
	str += eol;
	str += F("WiFi AP: \"");
	str += readConfigString(SSID_addr, SSID_size);
	str += quote;
	str += eol;
	str += F("WiFi Password: \"");
	str += readConfigString(PASS_addr, PASS_size);
	str += quote;
	str += eol;
	str += F("Telnet port: ");
	str += readConfigString(TELNET_PORT_addr, TELNET_PORT_size);
	str += eol;
	str += F("Sensor read period: ");
	str += readConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size);
	str += eol;
	str += F("Telnet clients limit: ");
	str += String(MAX_SRV_CLIENTS);
	str += eol;

	//Log settings
	str += F("Device log size: ");
	str += String(LOG_SIZE);
	str += eol;
	str += F("Log record period: ");
	str += String(logPeriod);
	str += eol;

#ifdef SLEEP_ENABLE
	str += F("WiFi reconnect timeout for SLEEP mode: ");
	str += String(reconnectTimeOut);
	str += eol;
#endif

#ifdef HTTP_SERVER_ENABLE
	str += F("HTTP port: ");
	str += readConfigString(HTTP_PORT_addr, HTTP_PORT_size);
	str += eol;
	str += F("HTTP service enabled: ");
	str += readConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size);
	str += eol;
#endif

#ifdef EMAIL_ENABLE
	str += F("SMTP server: \"");
	str += readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	str += quote;
	str += eol;
	str += F("SMTP port: ");
	str += readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size);
	str += eol;
	str += F("SMTP login: \"");
	str += readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	str += quote;
	str += eol;
	str += F("SMTP password: \"");
	str += readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	str += quote;
	str += eol;
	str += F("Email To: \"");
	str += readConfigString(SMTP_TO_addr, SMTP_TO_size);
	str += quote;
	str += eol;
	str += F("EMAIL service enabled: ");
	str += readConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size);
	str += eol;
#endif

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM token: \"");
	str += readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	str += quote;
	str += eol;
	str += F("Admin #0: ");
	str += getTelegramUser(0);
	str += eol;
	for (int i = 1; i < telegramUsersNumber; i++)
	{
		str += F("\tUser #");
		str += String(i);
		str += F(": ");
		str += getTelegramUser(i);
		str += eol;
	}
	str += F("TELEGRAM service enabled: ");
	str += readConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size);
	str += eol;
#endif

#ifdef GSCRIPT
	str += F("GSCRIPT token: \"");
	str += readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	str += quote;
	str += eol;
	str += F("GSCRIPT service enabled: ");
	str += readConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size);
	str += eol;
#endif

#ifdef PUSHINGBOX
	str += F("PUSHINGBOX token: \"");
	str += readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size) + quote;
	str += eol;
	str += F("PUSHINGBOX parameter name: \"");
	str += readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	str += quote;
	str += eol;
	str += F("PUSHINGBOX service enabled: ");
	str += readConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size);
	str += eol;
#endif

#ifdef NTP_TIME_ENABLE
	str += F("NTP server: ");
	str += readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	str += eol;
	str += F("NTP time zone: ");
	str += readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size);
	str += eol;
	str += F("NTP refresh delay: ");
	str += readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size);
	str += eol;
	str += F("NTP service enabled: ");
	str += readConfigString(ENABLE_NTP_addr, ENABLE_NTP_size);
	str += eol;
#endif

#ifdef EVENTS_ENABLE
	for (int i = 0; i < eventsNumber; i++)
	{
		str += F("\tEVENT #");
		str += String(i);
		str += F(": \"");
		str += getEvent(i);
		str += quote;
		str += eol;
	}
	str += F("EVENTS service enabled: ");
	str += readConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size);
	str += eol;
#endif

#ifdef SCHEDULER_ENABLE
	for (int i = 0; i < schedulesNumber; i++)
	{
		str += F("\tSCHEDULE #");
		str += String(i);
		str += F(": \"");
		str += getSchedule(i);
		str += quote;
		str += eol;
	}
	str += F("SCHEDULER service enabled: ");
	str += readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size);
	str += eol;
#endif

#ifdef SLEEP_ENABLE
	str += F("SLEEP mode enabled: ");
	str += readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size);
	str += eol;
#endif

#ifdef TM1637DISPLAY_ENABLE
	str += F("TM1637 display refresh delay:");
	str += readConfigString(TM1637DISPLAY_REFRESH_addr, TM1637DISPLAY_REFRESH_size);
	str += eol;
	str += F("TM1637_CLK on pin ");
	str += String(TM1637_CLK);
	str += eol;
	str += F("TM1637_DIO on pin ");
	str += String(TM1637_DIO);
	str += eol;
#endif

#ifdef AMS2320_ENABLE
	str += F("AMS2320 on i2c\r\n");
#endif

#ifdef HTU21D_ENABLE
	str += F("HTU21D on i2c\r\n");
#endif

#ifdef DS18B20_ENABLE
	str += F("DS18B20 on pin ");
	str += String(ONE_WIRE_PIN);
	str += eol;
#endif

#ifdef DHT_ENABLE
	str += DHTTYPE + " on pin " + String(DHTPIN) + eol;
#endif

#ifdef MH_Z19_UART_ENABLE
	str += F("MH-Z19 CO2 UART sensor:\r\n");
	str += F("TX on pin ");
	str += String(MHZ19_TX);
	str += eol;
	str += F("RX on pin ");
	str += String(MHZ19_RX);
	str += eol;
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += F("MH-Z19 CO2 PPM sensor:\r\n");
	str += F("PPM on pin ");
	str += String(MHZ19_PPM);
	str += eol;
#endif

#ifdef ADC_ENABLE
	str += F("ADC input: A0\r\n");
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	str += F("Interrupt1 counter on pin ");
	str += String(INTERRUPT_COUNTER1_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT1_MODE_addr, INT1_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	str += F("Interrupt2 counter on pin ");
	str += String(INTERRUPT_COUNTER2_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT2_MODE_addr, INT2_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	str += F("Interrupt3 counter on pin ");
	str += String(INTERRUPT_COUNTER3_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT3_MODE_addr, INT3_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	str += F("Interrupt4 counter on pin ");
	str += String(INTERRUPT_COUNTER4_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT4_MODE_addr, INT4_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	str += F("Interrupt5 counter on pin ");
	str += String(INTERRUPT_COUNTER5_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT5_MODE_addr, INT5_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	str += F("Interrupt6 counter on pin ");
	str += String(INTERRUPT_COUNTER6_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT6_MODE_addr, INT6_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	str += F("Interrupt7 counter on pin ");
	str += String(INTERRUPT_COUNTER7_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT7_MODE_addr, INT7_MODE_size);
	str += eol;
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	str += F("Interrupt8 counter on pin ");
	str += String(INTERRUPT_COUNTER8_ENABLE);
	str += eol;
	str += F("\tmode: ");
	str += readConfigString(INT8_MODE_addr, INT8_MODE_size);
	str += eol;
#endif

#ifdef INPUT1_ENABLE
	str += F("Input1 on pin ");
	str += String(INPUT1_ENABLE);
	str += eol;
#endif
#ifdef INPUT2_ENABLE
	str += F("Input2 on pin ");
	str += String(INPUT2_ENABLE);
	str += eol;
#endif
#ifdef INPUT3_ENABLE
	str += F("Input3 on pin ");
	str += String(INPUT3_ENABLE);
	str += eol;
#endif
#ifdef INPUT4_ENABLE
	str += F("Input4 on pin ");
	str += String(INPUT4_ENABLE);
	str += eol;
#endif
#ifdef INPUT5_ENABLE
	str += F("Input5 on pin ");
	str += String(INPUT5_ENABLE);
	str += eol;
#endif
#ifdef INPUT6_ENABLE
	str += F("Input6 on pin ");
	str += String(INPUT6_ENABLE);
	str += eol;
#endif
#ifdef INPUT7_ENABLE
	str += F("Input7 on pin ");
	str += String(INPUT7_ENABLE);
	str += eol;
#endif
#ifdef INPUT8_ENABLE
	str += F("Input8 on pin ");
	str += String(INPUT8_ENABLE);
	str += eol;
#endif

#ifdef OUTPUT1_ENABLE
	str += F("Output1 on pin ");
	str += String(OUTPUT1_ENABLE);
	str += eol;
	str += F("\t initial state: ");
	str += readConfigString(OUT1_INIT_addr, OUT1_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT2_ENABLE
	str += F("Output2 on pin ");
	str += String(OUTPUT2_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT2_INIT_addr, OUT2_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT3_ENABLE
	str += F("Output3 on pin ");
	str += String(OUTPUT3_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT3_INIT_addr, OUT3_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT4_ENABLE
	str += F("Output4 on pin ");
	str += String(OUTPUT4_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT4_INIT_addr, OUT4_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT5_ENABLE
	str += F("Output5 on pin ");
	str += String(OUTPUT5_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT5_INIT_addr, OUT5_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT6_ENABLE
	str += F("Output6 on pin ");
	str += String(OUTPUT6_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT6_INIT_addr, OUT6_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT7_ENABLE
	str += F("Output7 on pin ");
	str += String(OUTPUT7_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT7_INIT_addr, OUT7_INIT_size);
	str += eol;
#endif
#ifdef OUTPUT8_ENABLE
	str += F("Output8 on pin ");
	str += String(OUTPUT8_ENABLE);
	str += eol;
	str += F("\tinitial state: ");
	str += readConfigString(OUT8_INIT_addr, OUT8_INIT_size);
	str += eol;
#endif
	return str;
}

String printStatus()
{
	String str = currentTime();
	str += eol;
	str += F("Time is ");
	if (timeIsSet) str += F("set\r\n");
	else str += F("not set\r\n");

	str += F("WiFi enabled: ");
	str += String(wifiEnable);
	str += eol;
	str += F("Telnet enabled: ");
	str += String(telnetEnable);
	str += eol;

#ifdef NTP_TIME_ENABLE
	str += F("NTP enabled: ");
	str += String(NTPenable);
	str += eol;
#endif

	if (WiFi.status() == WL_CONNECTED)
	{
		str += F("Connected to \"");
		str += String(ssid);
		str += F("\" access point\r\n");
		str += F("Use: telnet ");
		str += WiFi.localIP().toString();
		str += F(":");
		str += String(telnetPort);
		str += eol;

		int netClientsNum = 0;
		for (int i = 0; i < MAX_SRV_CLIENTS; i++)
		{
			if (serverClient[i] && serverClient[i].connected()) netClientsNum++;
		}
		str += F("Telnet clients connected: ");
		str += String(netClientsNum);
		str += eol;
	}
	else str += F("WiFi not connected\r\n");

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM buffer position: ");
	str += String(telegramOutboundBufferPos);
	str += F("/");
	str += String(telegramMessageBufferSize);
	str += eol;
#endif

#ifdef EVENTS_ENABLE
	str += F("Events status:\r\n");
	for (int i = 0; i < eventsNumber; i++)
	{
		String eventStr = getEvent(i);
		if (eventStr.length() > 0)
		{
			str += F("\tEvent #");
			str += String(i);
			str += F(" active: ");
			str += String(eventsFlags[i]);
			str += eol;
		}
	}
#endif

	str += F("Log records collected: ");
	str += String(history_record_number);
	str += eol;
	str += F("Free memory: ");
	str += String(ESP.getFreeHeap());
	str += eol;
	return str;
}

String printHelp()
{
	return F("help\r\n"
		"get_sensor\r\n"
		"get_status\r\n"
		"get_config\r\n"
		"[ADMIN] set_time=yyyy.mm.dd hh:mm:ss\r\n"
		"[ADMIN] autoreport=0/1\r\n"
		"[ADMIN] set_output?=0/1\r\n"

		"[ADMIN][FLASH] set_interrupt_mode?=FALLING/RISING/CHANGE\r\n"
		"[ADMIN][FLASH] set_init_output?=on/off/x\r\n"

		"[ADMIN] getLog = uart/telnet/email/telegram\r\n"

		"[ADMIN][FLASH] check_period=n\r\n"

		"[ADMIN][FLASH] device_name=****\r\n"

		"[ADMIN][FLASH] ssid=****\r\n"
		"[ADMIN][FLASH] wifi_pass=****\r\n"
		"[ADMIN] wifi_enable=1/0\r\n"

		"[ADMIN][FLASH] telnet_port=n\r\n"
		"[ADMIN][FLASH] telnet_enable=1/0\r\n"

		"[ADMIN][FLASH] http_port=n\r\n"
		"[ADMIN][FLASH] http_enable=1/0\r\n"

		"[ADMIN][FLASH] smtp_server=****\r\n"
		"[ADMIN][FLASH] smtp_port=n\r\n"
		"[ADMIN][FLASH] smtp_login=****\r\n"
		"[ADMIN][FLASH] smtp_pass=****\r\n"
		"[ADMIN][FLASH] smtp_to=****@***.***\r\n"
		"[ADMIN][FLASH] smtp_enable=1/0\r\n"

		"[ADMIN][FLASH] set_user?=n\r\n"
		"[ADMIN][FLASH] telegram_token=****\r\n"
		"[ADMIN][FLASH] telegram_enable=1/0\r\n"

		"[ADMIN][FLASH] gscript_token=****\r\n"
		"[ADMIN][FLASH] gscript_enable=1/0\r\n"

		"[ADMIN][FLASH] pushingbox_token=****\r\n"
		"[ADMIN][FLASH] pushingbox_parameter=****\r\n"
		"[ADMIN][FLASH] pushingbox_enable=1/0\r\n"

		"[ADMIN][FLASH] ntp_server=***.***.***.***\r\n"
		"[ADMIN][FLASH] ntp_time_zone=n\r\n"
		"[ADMIN][FLASH] ntp_refresh_delay=n\r\n"
		"[ADMIN][FLASH] ntp_enable=1/0\r\n"

		"[ADMIN][FLASH] set_event?=condition:action1;action2\r\n"
		"[ADMIN][FLASH] events_enable=1/0\r\n"

		"[ADMIN][FLASH] set_schedule?=time:action1;action2\r\n"
		"[ADMIN][FLASH] scheduler_enable=1/0\r\n"

		"[ADMIN][FLASH] display_refresh=n\r\n"

		"[ADMIN][FLASH] sleep_enable=1/0\r\n"

		"[ADMIN] reset\r\n");
}

String currentTime()
{
	// digital clock display of the time
	String tmp = String(hour());
	tmp += F(":");
	tmp += String(minute());
	tmp += F(":");
	tmp += String(second());
	tmp += F(" ");
	tmp += String(year());
	tmp += F("/");
	tmp += String(month());
	tmp += F("/");
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
	str += F(":");
	str += String(data.logminute);
	str += F(":");
	str += String(data.logsecond);
	str += F(" ");
	str += String(data.logyear);
	str += F("/");
	str += String(data.logmonth);
	str += F("/");
	str += String(data.logday);

#ifdef MH_Z19_UART_ENABLE
	str += delimiter;
	str += F("UART CO2=");
	str += String(data.mh_uart_co2);
	str += delimiter;
	str += F("MH-Z19 t=");
	str += String(data.mh_temp);
#endif

#ifdef MH_Z19_PPM_ENABLE
	str += delimiter;
	str += F("PPM CO2=");
	str += String(data.mh_ppm_co2);
#endif

#ifdef AMS2320_ENABLE
	str += delimiter;
	str += F("AMS2320 t=");
	str += String(data.ams_temp);
	str += delimiter;
	str += F("AMS2320 h=");
	str += String(data.ams_humidity);
#endif

#ifdef HTU21D_ENABLE
	str += delimiter;
	str += F("HTU21D t=");
	str += String(data.htu21d_temp);
	str += delimiter;
	str += F("HTU21D h=");
	str += String(data.htu21d_humidity);
#endif

#ifdef DS18B20_ENABLE
	str += delimiter;
	str += F("DS18B20 t=");
	str += String(data.ds1820_temp);
#endif

#ifdef DHT_ENABLE
	str += delimiter;
	str += DHTTYPE;
	str += F(" t=");
	str += String(data.dht_temp);
	str += delimiter;
	str += DHTTYPE;
	str += F(" h=");
	str += String(data.dht_humidity);
#endif

#ifdef ADC_ENABLE
	str += delimiter;
	str += F("ADC=");
	str += String(data.adc);
#endif

#ifdef INTERRUPT_COUNTER1_ENABLE
	str += delimiter;
	str += F("counter1=");
	str += String(data.InterruptCounter1);
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	str += delimiter;
	str += F("counter2=");
	str += String(data.InterruptCounter2);
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	str += delimiter;
	str += F("counter3=");
	str += String(data.InterruptCounter3);
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	str += delimiter;
	str += F("counter4=");
	str += String(data.InterruptCounter4);
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	str += delimiter;
	str += F("counter5=");
	str += String(data.InterruptCounter5);
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	str += delimiter;
	str += F("counter6=");
	str += String(data.InterruptCounter6);
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	str += delimiter;
	str += F("counter7=");
	str += String(data.InterruptCounter7);
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	str += delimiter;
	str += F("counter8=");
	str += String(data.InterruptCounter8);
#endif

#ifdef INPUT1_ENABLE
	str += delimiter;
	str += F("IN1=");
	str += String(data.input1);
#endif
#ifdef INPUT2_ENABLE
	str += delimiter;
	str += F("IN2=");
	str += String(data.input2);
#endif
#ifdef INPUT3_ENABLE
	str += delimiter;
	str += F("IN3=");
	str += String(data.input3);
#endif
#ifdef INPUT4_ENABLE
	str += delimiter;
	str += F("IN4=");
	str += String(data.input4);
#endif
#ifdef INPUT5_ENABLE
	str += delimiter;
	str += F("IN5=");
	str += String(data.input5);
#endif
#ifdef INPUT6_ENABLE
	str += delimiter;
	str += F("IN6=");
	str += String(data.input6);
#endif
#ifdef INPUT7_ENABLE
	str += delimiter;
	str += F("IN7=");
	str += String(data.input7);
#endif
#ifdef INPUT8_ENABLE
	str += delimiter;
	str += F("IN8=");
	str += String(data.input8);
#endif

#ifdef OUTPUT1_ENABLE
	str += delimiter;
	str += F("OUT1=");
	str += String(data.output1);
#endif
#ifdef OUTPUT2_ENABLE
	str += delimiter;
	str += F("OUT2=");
	str += String(data.output2);
#endif
#ifdef OUTPUT3_ENABLE
	str += delimiter;
	str += F("OUT3=");
	str += String(data.output3);
#endif
#ifdef OUTPUT4_ENABLE
	str += delimiter;
	str += F("OUT4=");
	str += String(data.output4);
#endif
#ifdef OUTPUT5_ENABLE
	str += delimiter;
	str += F("OUT5=");
	str += String(data.output5);
#endif
#ifdef OUTPUT6_ENABLE
	str += delimiter;
	str += F("OUT6=");
	str += String(data.output6);
#endif
#ifdef OUTPUT7_ENABLE
	str += delimiter;
	str += F("OUT7=");
	str += String(data.output7);
#endif
#ifdef OUTPUT8_ENABLE
	str += delimiter;
	str += F("OUT8=");
	str += String(data.output8);
#endif
	return str;
}

String processCommand(String command, byte channel, bool isAdmin)
{
	String tmp = "";
	if (command.indexOf('=') > 0) tmp = command.substring(0, command.indexOf('=') + 1);
	else tmp = command;
	tmp.toLowerCase();
	String str = "";
	if (tmp == F("get_sensor"))
	{
		sensor = collectData();
		str = ParseSensorReport(sensor, eol);
		str += eol;
	}
	else if (tmp == F("get_status"))
	{
		str = printStatus();
		str += eol;
	}
	else if (tmp == F("get_config"))
	{
		str = currentTime();
		str += eol;
		str += printConfig();
		str += eol;
	}
	else if (tmp == F("help"))
	{
		str = currentTime();
		str += eol;
		str += printHelp();
		str += eol;
	}
	else if (isAdmin)
	{
		if (tmp.startsWith(F("set_time=")) && command.length() == 28)
		{
			int t = command.indexOf('=');

			int _hr = command.substring(20, 22).toInt();
			int _min = command.substring(23, 25).toInt();
			int _sec = command.substring(26, 28).toInt();
			int _day = command.substring(17, 19).toInt();
			int _month = command.substring(14, 16).toInt();
			int _yr = command.substring(9, 13).toInt();
			setTime(_hr, _min, _sec, _day, _month, _yr);
			str = F("Time set to:");
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
			str += eol;
			timeIsSet = true;
#ifdef NTP_TIME_ENABLE
			NTPenable = false;
#endif

		}
		else if (tmp.startsWith(F("check_period=")) && command.length() > 13)
		{
			checkSensorPeriod = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New sensor check period = \"");
			str += String(checkSensorPeriod);
			str += quote;
			str += eol;
			writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(checkSensorPeriod));
		}
		else if (tmp.startsWith(F("device_name=")) && command.length() > 12)
		{
			deviceName = command.substring(command.indexOf('=') + 1);
			str = F("New device name = \"");
			str += deviceName;
			str += quote;
			str += eol;
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
		}
		else if (tmp.startsWith(F("ssid=")) && command.length() > 5)
		{
			ssid = command.substring(command.indexOf('=') + 1);
			str = F("New SSID = \"");
			str += ssid;
			str += quote;
			str += eol;
			writeConfigString(SSID_addr, SSID_size, ssid);
			WiFi.begin(ssid.c_str(), wiFiPassword.c_str());
		}
		else if (tmp.startsWith(F("wifi_pass=")) && command.length() > 10)
		{
			wiFiPassword = command.substring(command.indexOf('=') + 1);
			str = F("New password = \"");
			str += wiFiPassword;
			str += quote;
			str += eol;
			writeConfigString(PASS_addr, PASS_size, wiFiPassword);
			WiFi.begin(ssid.c_str(), wiFiPassword.c_str());
		}
		else if (tmp.startsWith(F("wifi_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				wifiEnable = false;
				WiFi.mode(WIFI_OFF);
				wiFiIntendedStatus = WIFI_STOP;
				str = F("WiFi disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				wifiEnable = true;
				WiFi.mode(WIFI_STA);
				WiFi.begin(ssid.c_str(), wiFiPassword.c_str());
				wiFiIntendedStatus = WIFI_CONNECTING;
				str = F("WiFi enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += String(stateStr);
				str += eol;
			}
		}

		else if (tmp.startsWith(F("telnet_port=")) && command.length() > 12)
		{
			telnetPort = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New telnet port = \"");
			str += String(telnetPort);
			str += quote;
			str += eol;
			writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith(F("telnet_enable=")) && command.length() > 14)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				telnetEnable = false;
				telnetServer.stop();
				str = F("Telnet disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				telnetEnable = true;
				telnetServer.begin();
				telnetServer.setNoDelay(true);
				str = F("Telnet enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += String(stateStr);
				str += eol;
			}
		}

		else if (tmp == F("reset"))
		{
			if (channel == TELEGRAM_CHANNEL)
			{
				if (telegramEnable && WiFi.status() == WL_CONNECTED && myBot.testConnection())
				{
					TBMessage msg;
					while (myBot.getNewMessage(msg))
					{
						yield();
					}
				}
			}
			Serial.println(F("Resetting..."));
			Serial.flush();
			ESP.restart();
		}

		else if (tmp.startsWith(F("autoreport=")) && command.length() > 11)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				autoReport = false;
				str = F("autoreport=off\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				autoReport = true;
				str = F("autoreport=on\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += String(stateStr);
				str += eol;
			}
		}

		else if (tmp.startsWith(F("set_init_output")) && command.length() > 17)
		{
			int t = command.indexOf('=');
			if (t >= 16 && t < command.length() - 1)
			{
				byte outNum = command.substring(15, t).toInt();
				String outStateStr = command.substring(t + 1);
				int outState;
				//Serial.println("Out" + String(outNum) + "=" + outStateStr);
				if (outStateStr == SWITCH_ON) outState = OUT_ON;
				else if (outStateStr == SWITCH_OFF) outState = OUT_OFF;
				else
				{
					outState = outStateStr.toInt();
				}
				//Serial.println("InitOut" + String(outNum) + "=" + String(outState));
				const String outTxt1 = F("Output #");
				const String outTxt2 = F(" initial state set to ");
#ifdef OUTPUT1_ENABLE
				if (outNum == 1)
				{
					writeConfigString(OUT1_INIT_addr, OUT1_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT2_ENABLE
				if (outNum == 2)
				{
					writeConfigString(OUT2_INIT_addr, OUT2_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT3_ENABLE
				if (outNum == 3)
				{
					writeConfigString(OUT3_INIT_addr, OUT3_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT4_ENABLE
				if (outNum == 4)
				{
					writeConfigString(OUT4_INIT_addr, OUT4_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT5_ENABLE
				if (outNum == 5)
				{
					writeConfigString(OUT5_INIT_addr, OUT5_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT6_ENABLE
				if (outNum == 6)
				{
					writeConfigString(OUT6_INIT_addr, OUT6_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT7_ENABLE
				if (outNum == 7)
				{
					writeConfigString(OUT7_INIT_addr, OUT7_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
#ifdef OUTPUT8_ENABLE
				if (outNum == 8)
				{
					writeConfigString(OUT8_INIT_addr, OUT8_INIT_size, String(outState));
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += String(outState);
				}
#endif
				if (str.length() == 0)
				{
					str = F("Incorrect output #");
					str += String(outNum);
					str += eol;
				}
			}
			else
			{
				str = F("Incorrect command");
				str += String(command);
				str += eol;
			}
		}

		else if (tmp.startsWith(F("set_output")) && command.length() > 12)
		{
			str = set_output(command);
		}

		else if (tmp.startsWith(F("set_interrupt_mode")) && command.length() > 20)
		{
			int t = command.indexOf('=');
			if (t >= 19 && t < command.length() - 1)
			{
				byte outNum = command.substring(18, t).toInt();
				String intMode = command.substring(t + 1);
				//Serial.println("Out" + String(outNum) + "=" + intMode);
				byte intModeNum = 0;
				if (intMode == F("rising")) intModeNum = RISING;
				else if (intMode == F("falling")) intModeNum = FALLING;
				else if (intMode == F("change")) intModeNum = CHANGE;
				else
				{
					str = F("Incorrect value \"");
					str += String(intMode);
					str += quote;
				}
				//Serial.println("InterruptMode" + String(outNum) + "=" + String(intModeNum));
				const String outTxt1 = F("Interrupt #");
				const String outTxt2 = F(" mode: ");
#ifdef INTERRUPT_COUNTER1_ENABLE
				if (outNum == 1)
				{
					intMode1 = intModeNum;
					writeConfigString(INT1_MODE_addr, INT1_MODE_size, String(intMode1));
					attachInterrupt(INTERRUPT_COUNTER1_ENABLE, int1count, intMode1);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
				if (outNum == 2)
				{
					intMode2 = intModeNum;
					writeConfigString(INT2_MODE_addr, INT2_MODE_size, String(intMode2));
					attachInterrupt(INTERRUPT_COUNTER2_ENABLE, int2count, intMode2);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
				if (outNum == 3)
				{
					intMode3 = intModeNum;
					writeConfigString(INT3_MODE_addr, INT3_MODE_size, String(intMode3));
					attachInterrupt(INTERRUPT_COUNTER3_ENABLE, int3count, intMode3);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
				if (outNum == 4)
				{
					intMode4 = intModeNum;
					writeConfigString(INT4_MODE_addr, INT4_MODE_size, String(intMode4));
					attachInterrupt(INTERRUPT_COUNTER4_ENABLE, int4count, intMode4);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
				if (outNum == 5)
				{
					intMode5 = intModeNum;
					writeConfigString(INT5_MODE_addr, INT5_MODE_size, String(intMode5));
					attachInterrupt(INTERRUPT_COUNTER5_ENABLE, int5count, intMode5);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
				if (outNum == 6)
				{
					intMode6 = intModeNum;
					writeConfigString(INT6_MODE_addr, INT6_MODE_size, String(intMode6));
					attachInterrupt(INTERRUPT_COUNTER6_ENABLE, int6count, intMode6);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
				if (outNum == 7)
				{
					intMode7 = intModeNum;
					writeConfigString(INT7_MODE_addr, INT7_MODE_size, String(intMode7));
					attachInterrupt(INTERRUPT_COUNTER7_ENABLE, int7count, intMode7);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
				if (outNum == 8)
				{
					intMode8 = intModeNum;
					writeConfigString(INT8_MODE_addr, INT8_MODE_size, String(intMode8));
					attachInterrupt(INTERRUPT_COUNTER8_ENABLE, int8count, intMode8);
					str = outTxt1;
					str += String(outNum);
					str += outTxt2;
					str += intMode;
				}
#endif
				if (str.length() == 0)
				{
					str = F("Incorrect interrupt #");
					str += String(outNum);
					str += eol;
				}
			}
			else
			{
				str = F("Incorrect command");
				str += String(command);
				str += eol;
			}
		}

		//getLog=uart/telnet/email/telegram

#ifdef SLEEP_ENABLE
		else if (tmp.startsWith(F("sleep_enable=")) && command.length() > 13)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				sleepEnable = false;
				writeConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size, SWITCH_OFF_NUMBER);
				str = F("SLEEP mode disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				sleepEnable = true;
				writeConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size, SWITCH_ON_NUMBER);
				str = F("SLEEP mode enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef HTTP_SERVER_ENABLE
		else if (tmp.startsWith(F("http_port=")) && command.length() > 10)
		{
			httpPort = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New HTTP port = \"");
			str += String(httpPort);
			str += quote;
			str += eol;
			writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
			if (httpServerEnable) http_server.begin(httpPort);
		}
		else if (tmp.startsWith(F("http_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size, SWITCH_OFF_NUMBER);
				httpServerEnable = false;
				if (httpServerEnable) http_server.stop();
				str = F("HTTP server disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size, SWITCH_ON_NUMBER);
				httpServerEnable = true;
				http_server.on("/", handleRoot);
				http_server.onNotFound(handleNotFound);
				http_server.begin(httpPort);
				str = F("HTTP server enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef EMAIL_ENABLE
		// test function
		/*else if (tmp == F("mailme"))
		{
			if (eMailEnable && WiFi.status() == WL_CONNECTED)
			{
				if (history_record_number < 1) str = "No log records";
				bool sendOk = true;
				int n = 0;
				String tmpStr = ParseSensorReport(history_log[n], divider);
				tmpStr += eol;
				n++;
				while (n < history_record_number)
				{
					int freeMem = ESP.getFreeHeap() / 3;
					//Serial.println("***");
					for (; n < (freeMem / tmpStr.length()) - 1; n++)
					{
						if (n >= history_record_number) break;

						Serial.println("history_record_number=" + String(history_record_number));
						Serial.println("freeMem=" + String(freeMem));
						Serial.println("n=" + String(n));
						//Serial.println("***");

						tmpStr += ParseSensorReport(history_log[n], divider);
						tmpStr += eol;
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
		}*/
		else if (tmp.startsWith(F("smtp_server=")) && command.length() > 12)
		{
			smtpServerAddress = command.substring(command.indexOf('=') + 1);
			str = F("New SMTP server IP address = \"");
			str += smtpServerAddress;
			str += quote;
			str += eol;
			writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, smtpServerAddress);
		}
		else if (tmp.startsWith(F("smtp_port=")) && command.length() > 10)
		{
			smtpServerPort = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New SMTP port = \"");
			str += String(smtpServerPort);
			str += quote;
			str += eol;
			writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith(F("smtp_login=")) && command.length() > 11)
		{
			smtpLogin = command.substring(command.indexOf('=') + 1);
			str = F("New SMTP login = \"");
			str += smtpLogin;
			str += quote;
			str += eol;
			writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, smtpLogin);
		}
		else if (tmp.startsWith(F("smtp_pass=")) && command.length() > 10)
		{
			smtpPassword = command.substring(command.indexOf('=') + 1);
			str = F("New SMTP password = \"");
			str += smtpPassword;
			str += quote;
			str += eol;
			writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, smtpPassword);
		}
		else if (tmp.startsWith(F("smtp_to=")) && command.length() > 8)
		{
			smtpTo = command.substring(command.indexOf('=') + 1);
			str = F("New SMTP TO address = \"");
			str += smtpTo;
			str += quote;
			str += eol;
			writeConfigString(SMTP_TO_addr, SMTP_TO_size, smtpTo);
		}
		else if (tmp.startsWith(F("smtp_enable=")) && command.length() > 12)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size, SWITCH_OFF_NUMBER);
				eMailEnable = false;
				str = F("EMail reporting disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size, SWITCH_ON_NUMBER);
				eMailEnable = true;
				str = F("EMail reporting enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef TELEGRAM_ENABLE
		else if (tmp.startsWith(F("set_user")) && command.length() > 10)
		{
			int t = command.indexOf('=');
			if (t >= 9 && t < command.length() - 1)
			{
				byte userNum = command.substring(8, t).toInt();
				uint64_t newUser = StringToUint64(command.substring(t + 1));
				if (userNum >= 0 && userNum < telegramUsersNumber)
				{
					int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
					writeConfigString(TELEGRAM_USERS_TABLE_addr + userNum * m, m, uint64ToString(newUser));
					telegramUsers[userNum] = newUser;
					str = F("User #");
					str += String(userNum);
					str += F(" is now: ");
					str += uint64ToString(telegramUsers[userNum]);
					str += eol;
				}
				else
				{
					str = F("User #");
					str += String(userNum);
					str += F(" is out of range [0,");
					str += String(telegramUsersNumber - 1);
					str += F("]\r\n");
				}
			}
		}
		else if (tmp.startsWith(F("telegram_token=")) && command.length() > 15)
		{
			telegramToken = command.substring(command.indexOf('=') + 1);
			str = F("New TELEGRAM token = \"");
			str += telegramToken;
			str += quote;
			str += eol;
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, telegramToken);
			myBot.setTelegramToken(telegramToken);
		}
		else if (tmp.startsWith(F("telegram_enable=")) && command.length() > 16)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size, SWITCH_OFF_NUMBER);
				telegramEnable = false;
				str = F("Telegram disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size, SWITCH_ON_NUMBER);
				telegramEnable = true;
				//myBot.wifiConnect(ssid, wiFiPassword);
				myBot.setTelegramToken(telegramToken);
				//myBot.testConnection();
				str = F("Telegram enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef GSCRIPT
		else if (tmp.startsWith(F("gscript_token=")) && command.length() > 14)
		{
			gScriptId = command.substring(command.indexOf('=') + 1);
			str = F("New GScript token = \"");
			str += gScriptId;
			str += quote;
			str += eol;
			writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, gScriptId);
		}
		else if (tmp.startsWith(F("gscript_enable=")) && command.length() > 15)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size, SWITCH_OFF_NUMBER);
				gScriptEnable = false;
				str = F("GScript disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size, SWITCH_ON_NUMBER);
				gScriptEnable = true;
				str = F("GScript enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef PUSHINGBOX
		// test function
		/*else if (tmp == F("pbme"))
		{
			if (pushingBoxEnable && WiFi.status() == WL_CONNECTED)
			{
				sendToPushingBox(printHelp());
			}
		}*/
		else if (tmp.startsWith(F("pushingbox_token=")) && command.length() > 17)
		{
			pushingBoxId = command.substring(command.indexOf('=') + 1);
			str = F("New PushingBox token = \"");
			str += pushingBoxId;
			str += quote;
			str += eol;
			writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, pushingBoxId);
		}
		else if (tmp.startsWith(F("pushingbox_parameter=")) && command.length() > 21)
		{
			pushingBoxParameter = command.substring(command.indexOf('=') + 1);
			str = F("New PUSHINGBOX parameter name = \"");
			str += pushingBoxParameter;
			str += quote;
			str += eol;
			writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, pushingBoxParameter);
		}
		else if (tmp.startsWith(F("pushingbox_enable=")) && command.length() > 18)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size, SWITCH_OFF_NUMBER);
				pushingBoxEnable = false;
				str = F("PushingBox disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size, SWITCH_ON_NUMBER);
				pushingBoxEnable = true;
				str = F("PushingBox enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef NTP_TIME_ENABLE
		else if (tmp.startsWith(F("ntp_server=")) && command.length() > 11)
		{
			String tmpSrv = (command.substring(command.indexOf('=') + 1));
			IPAddress tmpAddr;
			if (tmpAddr.fromString(tmpSrv))
			{
				timeServer = tmpAddr;
				str = F("New NTP server = \"");
				str += timeServer.toString();
				str += quote;
				str += eol;
				writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, timeServer.toString());
			}
			else
			{
				str = F("Incorrect NTP server \"");
				str += tmpSrv;
				str += quote;
				str += eol;
			}
		}
		else if (tmp.startsWith(F("ntp_time_zone=")) && command.length() > 14)
		{
			timeZone = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New NTP time zone = \"");
			str += String(timeZone);
			str += quote;
			str += eol;
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
		}
		else if (tmp.startsWith(F("ntp_refresh_delay=")) && command.length() > 18)
		{
			ntpRefreshDelay = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New NTP refresh delay = \"");
			str += String(ntpRefreshDelay);
			str += quote;
			str += eol;
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);
		}
		else if (tmp.startsWith(F("ntp_enable=")) && command.length() > 11)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_NTP_addr, ENABLE_NTP_size, SWITCH_OFF_NUMBER);
				NTPenable = false;
				setSyncProvider(nullptr);
				Udp.stop();
				str = F("NTP disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_NTP_addr, ENABLE_NTP_size, SWITCH_ON_NUMBER);
				NTPenable = true;
				Udp.begin(localPort);
				setSyncProvider(getNtpTime);
				setSyncInterval(ntpRefreshDelay);
				str = F("NTP enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef EVENTS_ENABLE
		else if (tmp.startsWith(F("set_event")) && command.length() > 11)
		{
			int t = command.indexOf('=');
			if (t >= 10 && t < command.length() - 1)
			{
				byte eventNum = command.substring(9, t).toInt();
				String newEvent = command.substring(t + 1);
				if (eventNum >= 0 && eventNum < eventsNumber)
				{
					int m = EVENTS_TABLE_size / eventsNumber;
					writeConfigString(EVENTS_TABLE_addr + eventNum * m, m, newEvent);
					str = F("Event #");
					str += String(eventNum);
					str += F(" is now: ");
					str += newEvent;
					str += eol;
				}
				else
				{
					str = F("Event #");
					str += String(eventNum);
					str += F(" is out of range [0,");
					str += String(eventsNumber - 1);
					str += F("]\r\n");
				}
			}
		}
		else if (tmp.startsWith(F("events_enable=")) && command.length() > 14)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size, SWITCH_OFF_NUMBER);
				eventsEnable = false;
				str = F("Events disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size, SWITCH_ON_NUMBER);
				eventsEnable = true;
				str = F("Events enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#ifdef SCHEDULER_ENABLE
		else if (tmp.startsWith(F("set_schedule")) && command.length() > 14)
		{
			int t = command.indexOf('=');
			if (t >= 13 && t < command.length() - 1)
			{
				byte scheduleNum = command.substring(12, t).toInt();
				String newSchedule = command.substring(t + 1);
				if (scheduleNum >= 0 && scheduleNum < schedulesNumber)
				{
					int m = SCHEDULER_TABLE_size / schedulesNumber;
					writeConfigString(SCHEDULER_TABLE_addr + scheduleNum * m, m, newSchedule);
					str = F("Schedule #");
					str += String(scheduleNum);
					str += F(" is now: ");
					str += newSchedule;
					str += eol;
				}
				else
				{
					str = F("Schedule #");
					str += String(scheduleNum);
					str += F(" is out of range [0,");
					str += String(schedulesNumber - 1);
					str += F("]\r\n");
				}
			}
		}
		else if (tmp.startsWith(F("scheduler_enable=")) && command.length() > 17)
		{
			String stateStr = command.substring(command.indexOf('=') + 1);
			if (stateStr == SWITCH_OFF_NUMBER || stateStr == SWITCH_OFF)
			{
				writeConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size, SWITCH_OFF_NUMBER);
				schedulerEnable = false;
				str = F("Scheduler disabled\r\n");
			}
			else if (stateStr == SWITCH_ON_NUMBER || stateStr == SWITCH_ON)
			{
				writeConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size, SWITCH_ON_NUMBER);
				schedulerEnable = true;
				str = F("Scheduler enabled\r\n");
			}
			else
			{
				str = F("Incorrect value: ");
				str += stateStr;
				str += eol;
			}
		}
#endif

#if defined(TM1637DISPLAY_ENABLE) || defined(SSD1306DISPLAY_ENABLE)
		else if (tmp.startsWith(F("display_refresh=")) && command.length() > 16)
		{
			displaySwitchPeriod = command.substring(command.indexOf('=') + 1).toInt();
			str = F("New display refresh period = \"");
			str += String(displaySwitchPeriod);
			str += quote;
			str += eol;
			writeConfigString(TM1637DISPLAY_REFRESH_addr, TM1637DISPLAY_REFRESH_size, String(displaySwitchPeriod));
		}
#endif

		else
		{
			str = F("Incorrect command: \"");
			str += command;
			str += quote;
			str += eol;
		}
	}
	else
	{
		str = F("Incorrect command: \"");
		str += command;
		str += quote;
		str += eol;
	}
	return str;
}

#if defined(EVENTS_ENABLE) || defined(SCHEDULER_ENABLE)
void ProcessAction(String action, byte eventNum, bool eventOrSchedule)
{
	if (eventOrSchedule)
	{
#ifdef EVENTS_ENABLE
		eventsFlags[eventNum] = true;
#endif
	}
	else
	{
#ifdef SCHEDULER_ENABLE
		schedulesFlags[eventNum] = true;
#endif
	}

	do
	{
		int t = action.indexOf(divider);
		String tmpAction = "";
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
		Serial.print(F("Processing action: "));
		Serial.println(tmpAction);
		//set_output?=x
		if (tmpAction.startsWith(F("set_output")) && tmpAction.length() >= 12)
		{
			Serial.println(set_output(tmpAction));
		}
		//reset_counter?
		else if (tmpAction.startsWith(F("reset_counter")) && tmpAction.length() > 13)
		{
			int n = tmpAction.substring(13).toInt();

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
		else if (tmpAction.startsWith(F("reset_flag")) && tmpAction.length() > 10)
		{
			int n = tmpAction.substring(10).toInt();
			eventsFlags[n] = false;
		}
#endif
		//send_telegram=* / usrer#,message
#ifdef TELEGRAM_ENABLE
		else if (telegramEnable && tmpAction.startsWith(F("send_telegram=")))
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
				i = tmpAction.substring(14, tmpAction.indexOf(',')).toInt();
				j = i + 1;
			}
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			sensor = collectData();
			for (; i < j; i++)
			{
				if (telegramUsers[i] > 0)
				{
					String tmpStr = deviceName;
					tmpStr += F(":");
					tmpStr += divider;
					tmpStr += tmpAction;
					tmpStr += eol;
					tmpStr += ParseSensorReport(sensor, eol);
					sendToTelegram(telegramUsers[i], tmpStr, telegramRetries);
				}
			}
		}
#endif
		//send_PushingBox=message
#ifdef PUSHINGBOX
		else if (pushingBoxEnable && tmpAction.startsWith(F("send_pushingbox")))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			sensor = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += tmpAction;
			tmpStr += eol;
			tmpStr += ParseSensorReport(sensor, eol);
			bool sendOk = sendToPushingBox(tmpStr);
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				//eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//send_mail=address,message
#ifdef EMAIL_ENABLE
		else if (eMailEnable && tmpAction.startsWith(F("send_mail")))
		{
			String address = tmpAction.substring(10, tmpAction.indexOf(','));
			tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
			sensor = collectData();
			String tmpStr = tmpAction;
			tmpStr += eol;
			tmpStr += ParseSensorReport(sensor, eol);
			bool sendOk = sendMail(address, deviceName + " alert", tmpStr);
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				//eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//send_GScript=message
#ifdef GSCRIPT
		else if (gScriptEnable && tmpAction.startsWith(F("send_gscript")))
		{
			tmpAction = tmpAction.substring(tmpAction.indexOf('=') + 1);
			sensor = collectData();
			String tmpStr = deviceName;
			tmpStr += F(": ");
			tmpStr += tmpAction;
			tmpStr += eol;
			tmpStr += ParseSensorReport(sensor, eol);
			bool sendOk = sendValueToGoogle(tmpStr);
			if (!sendOk)
			{
#ifdef EVENTS_ENABLE
				//eventsFlags[eventNum] = false;
#endif
			}
		}
#endif
		//save_log
		else if (tmpAction.startsWith(F("save_log")))
		{
			saveLog(sensor);
		}
		else
		{
			Serial.print(F("Incorrect action: \""));
			Serial.print(tmpAction);
			Serial.println(quote);
		}
	} while (action.length() > 0);
}
#endif

String set_output(String outStr)
{
	//Serial.println("SetOut:" + outStr);
	int t = outStr.indexOf('=');
	String str = "";
	if (t >= 11 && t < outStr.length() - 1)
	{
		byte outNum = outStr.substring(10, t).toInt();
		String outStateStr = outStr.substring(t + 1);
		int outState;
		bool pwm_mode = false;
		if (outStateStr == SWITCH_ON) outState = (int)OUT_ON;
		else if (outStateStr == SWITCH_OFF) outState = (int)OUT_OFF;
		else
		{
			pwm_mode = true;
			outState = outStateStr.toInt();
		}
		//Serial.println("SetOut" + String(outNum) + "=" + String(outState));
		const String outTxt = F("Output");
		const String equalTxt = F("=");
#ifdef OUTPUT1_ENABLE
		if (outNum == 1)
		{
			out1 = outState;
			if (pwm_mode) analogWrite(OUTPUT1_ENABLE, outState);
			else digitalWrite(OUTPUT1_ENABLE, out1);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT2_ENABLE
		if (outNum == 2)
		{
			out2 = outState;
			if (pwm_mode) analogWrite(OUTPUT2_ENABLE, outState);
			else digitalWrite(OUTPUT2_ENABLE, out2);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT3_ENABLE
		if (outNum == 3)
		{
			out3 = outState;
			if (pwm_mode) analogWrite(OUTPUT3_ENABLE, outState);
			else digitalWrite(OUTPUT3_ENABLE, out3);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT4_ENABLE
		if (outNum == 4)
		{
			out4 = outState;
			if (pwm_mode) analogWrite(OUTPUT4_ENABLE, outState);
			else digitalWrite(OUTPUT4_ENABLE, out4);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT5_ENABLE
		if (outNum == 5)
		{
			out5 = outState;
			if (pwm_mode) analogWrite(OUTPUT5_ENABLE, outState);
			else digitalWrite(OUTPUT5_ENABLE, out5);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT6_ENABLE
		if (outNum == 6)
		{
			out6 = outState;
			if (pwm_mode) analogWrite(OUTPUT6_ENABLE, outState);
			else digitalWrite(OUTPUT6_ENABLE, out6);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT7_ENABLE
		if (outNum == 7)
		{
			out7 = outState;
			if (pwm_mode) analogWrite(OUTPUT7_ENABLE, outState);
			else digitalWrite(OUTPUT7_ENABLE, out7);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
#ifdef OUTPUT8_ENABLE
		if (outNum == 8)
		{
			out8 = outState;
			if (pwm_mode) analogWrite(OUTPUT8_ENABLE, outState);
			else digitalWrite(OUTPUT8_ENABLE, out8);
			str = outTxt;
			str += String(outNum);
			str += equalTxt;
			str += String((outState));
		}
#endif
		if (str.length() == 0)
		{
			str = F("Incorrect output #");
			str += String(outNum);
			str += eol;
		}
	}
	return str;
}

int getTemperature()
{
	int temp = -1000;
	//sensor = collectData();
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
	int humidity = -1000;
	//sensor = collectData();
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
	int co2_avg = -1000;
	//sensor = collectData();
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
	//Serial.println("Avg CO2: " + String(co2_uart_avg[0]) + " / " + String(co2_uart_avg[1]) + " / " + String(co2_uart_avg[2]));
	//Serial.println("New CO2: " + String(sensor.mh_uart_co2));
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

#ifdef TELEGRAM_ENABLE
String getTelegramUser(byte n)
{
	int singleUserLength = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	String userStr = readConfigString((TELEGRAM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}
#endif

/*int countOf(String str, char c)
{
	int count = -1;
	for (int i = 0; i < str.length(); i++)
		if (str[i] == c) count++;
	if (count > -1) count++;
	return count;
}*/