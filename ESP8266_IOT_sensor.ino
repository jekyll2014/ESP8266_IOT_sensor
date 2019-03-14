/* TM1637 display leds layout:
	  a
	  -
	f| |b
	g -
	e| |c
	  -
	  d
*/

/*
PIN	16		 D0 - WAKE, USER button
PIN 5		 D1 - I2C_SCL
PIN 4		~D2 - I2C_SDA
PIN 0		 D3 - FLASH, TM1637_CLK
PIN 2		 D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
PIN 14		~D5 - SoftUART_TX
PIN 12		~D6 - SoftUART_RX
PIN 13		 D7 - 
PIN 15		~D8 - keep LOW on boot
*/

// Untested feature:
// - set pwm output
// - select interrupt mode (FALLING/RISING/CHANGE)
// - set_time=

//Planned features:
// - event actions ( conditions: value </>/=, input state, output state, counter>max; actions: out?=x, send_mail, send_telegram, sang_GScript, send_PushingBox, save_log)
// - PushBullet service connection
// - GoogleDocs actions service
// - scheduling service
// - efficient sleep till next event planned with network connectivity restoration after wake up
// - compile time pin arrangement control

// Sensors to be supported:
// - DHT22/11(pin)
// - DS12B20(pin)
// - ACS712(range)
// - GPRS module via UART/SoftUART(TX_pin, RX_pin, baud_rate); send SMS, make call

//#define SLEEP_ENABLE //connect D0 and EN pins to start contoller after sleep
//#define EVENTS_ENABLE
//#define SCHEDULER_ENABLE
//#define AMS2320_ENABLE
//#define TM1637DISPLAY_ENABLE
//#define MH_Z19_UART_ENABLE
//#define MH_Z19_PPM_ENABLE
#define HTTP_SERVER_ENABLE
//#define EMAIL_ENABLE
#define TELEGRAM_ENABLE
//#define GSCRIPT
#define PUSHINGBOX
#define NTP_TIME_ENABLE
#define ADC_ENABLE // A0

//#define INTERRUPT_COUNTER1_ENABLE 5		// D1 - I2C_SCL
//#define INTERRUPT_COUNTER2_ENABLE 4		//~D2 - I2C_SDA
//#define INTERRUPT_COUNTER3_ENABLE 0		// D3 - FLASH, TM1637_CLK
//#define INTERRUPT_COUNTER4_ENABLE 2		// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INTERRUPT_COUNTER5_ENABLE 14		//~D5 - SoftUART_TX
//#define INTERRUPT_COUNTER6_ENABLE 12		//~D6 - SoftUART_RX
//#define INTERRUPT_COUNTER7_ENABLE 13		// D7
//#define INTERRUPT_COUNTER8_ENABLE 15		//~D8 - keep LOW on boot

//#define INPUT1_ENABLE 5					// D1 - I2C_SCL
//#define INPUT2_ENABLE 4					//~D2 - I2C_SDA
//#define INPUT3_ENABLE 0					// D3 - FLASH, TM1637_CLK
//#define INPUT4_ENABLE 2					// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INPUT5_ENABLE 14					//~D5 - SoftUART_TX
//#define INPUT6_ENABLE 12					//~D6 - SoftUART_RX
//#define INPUT7_ENABLE 13					// D7
//#define INPUT8_ENABLE 15					//~D8 - keep LOW on boot

#define OUTPUT1_ENABLE 5					// D1 - I2C_SCL
#define OUTPUT2_ENABLE 4					//~D2 - I2C_SDA
#define OUTPUT3_ENABLE 0					// D3 - FLASH, TM1637_CLK
#define OUTPUT4_ENABLE 2					// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
#define OUTPUT5_ENABLE 14					//~D5 - SoftUART_TX
#define OUTPUT6_ENABLE 12					//~D6 - SoftUART_RX
#define OUTPUT7_ENABLE 13					// D7
#define OUTPUT8_ENABLE 15					//~D8 - keep LOW on boot

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
unsigned long reconnectTimeOut = 5000;
WiFiServer telnetServer(telnetPort);
WiFiClient serverClient[MAX_SRV_CLIENTS];
bool telnetEnable = true;
bool wifiEnable = true;
String deviceName = "";

unsigned long checkSensorLastTime = 0;
unsigned long checkSensorPeriod = 5000;

String uartCommand;
bool uartReady = false;

//Log settings for 32 hours of storage
#define LOG_SIZE 192 // Save sensors value each 10 minutes: ( 32hours * 60minutes/hour ) / 10minutes/pcs = 192 pcs
sensorDataCollection history_log[LOG_SIZE];
unsigned int history_record_number = 0;
unsigned long logLastTime = 0;
unsigned long logPeriod = ((60 / (LOG_SIZE / 32)) * 60 * 1000) / 5; // 60minutes/hour / ( 192pcs / 32hours ) * 60seconds/minute * 1000 msec/second

bool isTimeSet = false;
bool autoReport = false;

const byte eventsNumber = 10;
const byte schedulesNumber = 10;
const byte telegramUsersNumber = 10;

//not tested
#ifdef SLEEP_ENABLE
bool SleepEnable = false;
#endif

//not implemented yet
#ifdef EVENTS_ENABLE
String eventsActions[eventsNumber];
bool EventsEnable = false;
#endif

//not implemented yet
#ifdef SCHEDULER_ENABLE
String schedulesActions[schedulesNumber];
bool SchedulerEnable = false;
#endif

// E-mail config
#ifdef EMAIL_ENABLE
#include <sendemail.h>
String smtpServerAddress = "";
unsigned int smtpServerPort = 2525;
String smtpLogin = "";
String smtpPassword = "";
String smtpTo = "";
bool EMailEnable = false;
#endif

// Telegram config
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
telegramMessage telegramOutboundBuffer[telegramMessageBufferSize];
byte telegramOutboundBufferPos = 0;
int telegramMessageDelay = 2000;
byte telegramRetries = 3;
unsigned long telegramLastTime = 0;
bool TelegramEnable = false;

void addMessageToTelegramOutboundBuffer(int64_t user, String message, byte retries)
{
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

void sendValueToGoogle(String value)
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
		return;
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
	client->POST(url, host, value, false);
	delete client;
	client = nullptr;
}
#endif

#ifdef PUSHINGBOX
String pushingBoxId = "";
String pushingBoxParameter = "";
const char* pushingBoxServer = "api.pushingbox.com";
bool PushingBoxEnable = false;

void sendToPushingBox(String message)
{
	WiFiClient client;

	//Serial.println("- connecting to pushing server ");
	if (client.connect(pushingBoxServer, 80))
	{
		//Serial.println("- succesfully connected");

		String postStr = "devid=";
		postStr += pushingBoxId;
		postStr += "&" + pushingBoxParameter + "=";
		postStr += String(message);
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
	}
	client.stop();
	//Serial.println("- stopping the client");
}
#endif

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

// MH-Z19 CO2 sensor via UART
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

// TM1637 display
#ifdef TM1637DISPLAY_ENABLE
#include <TM1637Display.h>
#define TM1637_CLK 0 //D3
#define TM1637_DIO 2 //D4
const uint8_t SEG_DEGREE[] = { SEG_A | SEG_B | SEG_F | SEG_G };
const uint8_t SEG_HUMIDITY[] = { SEG_C | SEG_E | SEG_F | SEG_G };
TM1637Display display(TM1637_CLK, TM1637_DIO);
unsigned int sw = 0;
#endif

// AM2320 I2C temperature + humidity sensor
#ifdef AMS2320_ENABLE
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
Adafruit_AM2320 am2320 = Adafruit_AM2320();
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
	SleepEnable = readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size).toInt();
#endif

#ifdef EVENTS_ENABLE
	int a = EVENTS_TABLE_size / eventsNumber;
	for (int i = 0; i < eventsNumber; i++)
	{
		eventsActions[i] = readConfigString((TELEGRAM_USERS_TABLE_addr + i * a), a);
	}
	EventsEnable = readConfigString(ENABLE_EVENTS_addr, ENABLE_EVENTS_size).toInt();
#endif

#ifdef SCHEDULER_ENABLE
	int b = SCHEDULER_TABLE_size / schedulesNumber;
	for (int i = 0; i < schedulesNumber; i++)
	{
		schedulesActions[i] = readConfigString((TELEGRAM_USERS_TABLE_addr + i * b), b);
	}
	SchedulerEnable = readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size).toInt();
#endif

#ifdef EMAIL_ENABLE
	smtpServerAddress = readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	smtpServerPort = readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size).toInt();
	smtpLogin = readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	smtpPassword = readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	smtpTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);
	EMailEnable = readConfigString(ENABLE_EMAIL_addr, ENABLE_EMAIL_size).toInt();
#endif

#ifdef TELEGRAM_ENABLE
	telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);

	int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
	for (int i = 0; i < telegramUsersNumber; i++)
	{
		telegramUsers[i] = StringToUint64(readConfigString((TELEGRAM_USERS_TABLE_addr + i * m), m));
	}

	TelegramEnable = readConfigString(ENABLE_TELEGRAM_addr, ENABLE_TELEGRAM_size).toInt();
	if (TelegramEnable)
	{
		//myBot.wifiConnect(ssid, WiFiPass);
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif

#ifdef GSCRIPT
	String GScriptId = readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
	GScriptEnable = readConfigString(ENABLE_GSCRIPT_addr, ENABLE_GSCRIPT_size).toInt();
#endif

#ifdef PUSHINGBOX
	pushingBoxId = readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
	pushingBoxParameter = readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);
	PushingBoxEnable = readConfigString(ENABLE_PUSHINGBOX_addr, ENABLE_PUSHINGBOX_size).toInt();
#endif

#ifdef HTTP_SERVER_ENABLE
	httpPort = readConfigString(HTTP_PORT_addr, HTTP_PORT_size).toInt();
	httpServerEnable = readConfigString(ENABLE_HTTP_addr, ENABLE_HTTP_size).toInt();
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
	NTPenable = readConfigString(ENABLE_NTP_addr, ENABLE_NTP_size).toInt();
	if (NTPenable)
	{
		Udp.begin(localPort);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
	}
#endif

#ifdef AMS2320_ENABLE
	am2320.begin();
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
#endif
#ifdef INPUT2_ENABLE
	pinMode(INPUT2_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT3_ENABLE
	pinMode(INPUT3_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT4_ENABLE
	pinMode(INPUT4_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT5_ENABLE
	pinMode(INPUT5_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT6_ENABLE
	pinMode(INPUT6_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT7_ENABLE
	pinMode(INPUT7_ENABLE, INPUT_PULLUP);
#endif
#ifdef INPUT8_ENABLE
	pinMode(INPUT8_ENABLE, INPUT_PULLUP);
#endif

#ifdef OUTPUT1_ENABLE
	pinMode(OUTPUT1_ENABLE, OUTPUT);
	digitalWrite(OUTPUT1_ENABLE, out1);

#endif
#ifdef OUTPUT2_ENABLE
	pinMode(OUTPUT2_ENABLE, OUTPUT);
	digitalWrite(OUTPUT2_ENABLE, out2);
#endif
#ifdef OUTPUT3_ENABLE
	pinMode(OUTPUT3_ENABLE, OUTPUT);
	digitalWrite(OUTPUT3_ENABLE, out3);
#endif
#ifdef OUTPUT4_ENABLE
	pinMode(OUTPUT4_ENABLE, OUTPUT);
	digitalWrite(OUTPUT4_ENABLE, out4);
#endif
#ifdef OUTPUT5_ENABLE
	pinMode(OUTPUT5_ENABLE, OUTPUT);
	digitalWrite(OUTPUT5_ENABLE, out5);
#endif
#ifdef OUTPUT6_ENABLE
	pinMode(OUTPUT6_ENABLE, OUTPUT);
	digitalWrite(OUTPUT6_ENABLE, out6);
#endif
#ifdef OUTPUT7_ENABLE
	pinMode(OUTPUT7_ENABLE, OUTPUT);
	digitalWrite(OUTPUT7_ENABLE, out7);
#endif
#ifdef OUTPUT8_ENABLE
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

		sensorDataCollection sensor = collectData();

		int ppm_p_avg = 0;
		int ppm_u_avg = 0;
		int ams_temp = -200;
		int mh_temp = -200;
		String str = "";

#ifdef MH_Z19_UART_ENABLE
		if (sensor.mh_uart_co2 > 0)
		{
			co2_uart_avg[0] = co2_uart_avg[1];
			co2_uart_avg[1] = co2_uart_avg[2];
			co2_uart_avg[2] = sensor.mh_uart_co2;
			ppm_u_avg = (co2_uart_avg[0] + co2_uart_avg[1] + co2_uart_avg[2]) / 3;
			str += "Average UART CO2 = " + String(ppm_u_avg) + "\r\n";
		}
#endif
#if defined(MH_Z19_PPM_ENABLE)
		if (sensor.mh_ppm_co2 > 0)
		{
			co2_ppm_avg[0] = co2_ppm_avg[1];
			co2_ppm_avg[1] = co2_ppm_avg[2];
			co2_ppm_avg[2] = sensor.mh_ppm_co2;
			ppm_p_avg = (co2_ppm_avg[0] + co2_ppm_avg[1] + co2_ppm_avg[2]) / 3;
			str += "Average PPM CO2 = " + String(ppm_p_avg) + "\r\n";
		}
#endif

#ifdef AMS2320_ENABLE
		ams_temp = sensor.ams_temp;
#endif

#ifdef MH_Z19_UART_ENABLE
		mh_temp = sensor.mh_temp;
#endif

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

#ifdef TM1637DISPLAY_ENABLE
		if (sw == 0)  //show CO2
		{
			if (ppm_u_avg > 0) display.showNumberDec(ppm_u_avg, false);
			else if (ppm_p_avg > 0) display.showNumberDec(ppm_p_avg, false);
			else sw++;
		}
		if (sw == 1)  //show Temperature
		{
			if (ams_temp > -200)
			{
				display.showNumberDec(round(ams_temp), false, 3, 0);
				display.setSegments(SEG_DEGREE, 1, 3);
			}
			else if (mh_temp > -200)
			{
				display.showNumberDec(round(mh_temp), false, 3, 0);
				display.setSegments(SEG_DEGREE, 1, 3);
			}
			else sw++;
		}
		if (sw == 2)  //show Humidity
		{
#ifdef AMS2320_ENABLE
			display.showNumberDec(round(sensor.ams_humidity), false, 3, 0);
			display.setSegments(SEG_HUMIDITY, 1, 3);
#else
			sw++;
#endif
		}
		if (sw == 3)  //show clock
		{
			display.showNumberDecEx(hour(), 0xff, true, 2, 0);
			display.showNumberDecEx(minute(), 0xff, true, 2, 2);
		}
		sw++;
		if (sw > 3) sw = 0;
#endif
	}

	//check if it's time to collect log
	if (millis() - logLastTime > logPeriod)
	{
		logLastTime = millis();
		history_log[history_record_number] = collectData();
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

		if (history_record_number >= (int)(LOG_SIZE * 0.7))
		{
#ifdef EMAIL_ENABLE
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
					SendEmail e(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 5000, false);
					sendOk = e.send(smtpLogin, smtpTo, deviceName + " log", tmpStr);
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
#endif
		}
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

#ifdef EVENTS_ENABLE
	if (EventsEnable)
	{
		//????????????????
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

	eeprom_size += INT1_MODE_size;
	eeprom_size += INT2_MODE_size;
	eeprom_size += INT3_MODE_size;
	eeprom_size += INT4_MODE_size;
	eeprom_size += INT5_MODE_size;
	eeprom_size += INT6_MODE_size;
	eeprom_size += INT7_MODE_size;
	eeprom_size += INT8_MODE_size;
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
	str += "WiFi reconnect timeout for SLEEP mode: " + String(reconnectTimeOut) + "\r\n";

	//Log settings
	str += "Device log size: " + String(LOG_SIZE) + "\r\n";
	str += "Log record period: " + String(logPeriod) + "\r\n";

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
	str += "TELEGRAM users number: " + String(telegramUsersNumber) + "\r\n";
	str += "Admin: " + uint64ToString(telegramUsers[0]) + "\r\n";
	int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
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

#ifdef SCHEDULER_ENABLE
	str += "SCHEDULER service enabled: " + readConfigString(ENABLE_SCHEDULER_addr, ENABLE_SCHEDULER_size) + "\r\n";
#endif

#ifdef SLEEP_ENABLE
	str += "SLEEP mode enabled: " + readConfigString(ENABLE_SLEEP_addr, ENABLE_SLEEP_size) + "\r\n";
#endif

#ifdef AMS2320_ENABLE
	str += "AMS2320 via i2c\r\n";
#endif

#ifdef TM1637DISPLAY_ENABLE
	str += "TM1637 display:\r\n";
	str += "pin TM1637_CLK: " + String(TM1637_CLK) + "\r\n";
	str += "pin TM1637_DIO: " + String(TM1637_DIO) + "\r\n";
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
#ifdef HTTP_SERVER_ENABLE
	str += "HTTP enabled: " + String(httpServerEnable) + "\r\n";
#endif
#ifdef NTP_TIME_ENABLE
	str += "NTP enabled: " + String(NTPenable) + "\r\n";
#endif
#ifdef TELEGRAM_ENABLE
	str += "TELEGRAM enabled: " + String(TelegramEnable) + "\r\n";
#endif
#ifdef EMAIL_ENABLE
	str += "E-Mail enabled: " + String(EMailEnable) + "\r\n";
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
	tmp += "[ADMIN] set_output?=0/1\r\n";
	tmp += "[ADMIN] set_interrupt_mode?=FALLING/RISING/CHANGE\r\n";
	//tmp += "[ADMIN] getLog = uart/telnet/email/telegram";
	//tmp += "[ADMIN] set_time=yyyy.mm.dd hh:mm:ss";
	tmp += "[ADMIN] get_config\r\n";

	tmp += "[ADMIN] autoreport=0/1\r\n";

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

	// set_event?=""
	tmp += "[ADMIN][FLASH] events_enable=1/0\r\n";

	// set_schedule?=""
	tmp += "[ADMIN][FLASH] scheduler_enable=1/0\r\n";

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
#ifdef MH_Z19_PPM_ENABLE
	sensorData.mh_ppm_co2 = co2PPMRead(MHZ19_PPM);
#endif
#ifdef AMS2320_ENABLE
	sensorData.ams_humidity = am2320.readHumidity();
	sensorData.ams_temp = am2320.readTemperature();
#endif
#ifdef MH_Z19_UART_ENABLE
	sensorData.mh_uart_co2 = co2SerialRead();
	sensorData.mh_temp = mhtemp_s;
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
	sensorData.input1 = digitalRead(INPUT1_ENABLE);
#endif
#ifdef INPUT2_ENABLE
	sensorData.input2 = digitalRead(INPUT2_ENABLE);
#endif
#ifdef INPUT3_ENABLE
	sensorData.input3 = digitalRead(INPUT3_ENABLE);
#endif
#ifdef INPUT4_ENABLE
	sensorData.input4 = digitalRead(INPUT4_ENABLE);
#endif
#ifdef INPUT5_ENABLE
	sensorData.input5 = digitalRead(INPUT5_ENABLE);
#endif
#ifdef INPUT6_ENABLE
	sensorData.input6 = digitalRead(INPUT6_ENABLE);
#endif
#ifdef INPUT7_ENABLE
	sensorData.input7 = digitalRead(INPUT7_ENABLE);
#endif
#ifdef INPUT8_ENABLE
	sensorData.input8 = digitalRead(INPUT8_ENABLE);
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
		if (tmp.startsWith("check_period="))
		{
			checkSensorPeriod = command.substring(command.indexOf('=') + 1).toInt();
			str = "New sensor check period = \"" + String(checkSensorPeriod) + "\"\r\n";
			writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(checkSensorPeriod));
		}
		else if (tmp.startsWith("device_name="))
		{
			deviceName = command.substring(command.indexOf('=') + 1);
			str = "New device name = \"" + deviceName + "\"\r\n";
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
		}
		else if (tmp.startsWith("ssid="))
		{
			ssid = command.substring(command.indexOf('=') + 1);
			str = "New SSID = \"" + ssid + "\"\r\n";
			writeConfigString(SSID_addr, SSID_size, ssid);
			WiFi.begin(ssid.c_str(), WiFiPass.c_str());
		}
		else if (tmp.startsWith("wifi_pass="))
		{
			WiFiPass = command.substring(command.indexOf('=') + 1);
			str = "New password = \"" + WiFiPass + "\"\r\n";
			writeConfigString(PASS_addr, PASS_size, WiFiPass);
			WiFi.begin(ssid.c_str(), WiFiPass.c_str());
		}
		else if (tmp.startsWith("wifi_enable="))
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

		else if (tmp.startsWith("telnet_port="))
		{
			telnetPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New telnet port = \"" + String(telnetPort) + "\"\r\n";
			writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith("telnet_enable="))
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

		else if (tmp.startsWith("set_output") && tmp.length() >= 13)
		{
			int t = command.indexOf('=');
			if (t >= 11 && t < tmp.length() - 1)
			{
				byte outNum = tmp.substring(10, t).toInt();
				String outStateStr = tmp.substring(t + 1);
				byte outState = outStateStr.toInt();
				bool pwm_mode = false;
				//Serial.println("Out" + String(outNum) + "=" + String(outState));
				if (outStateStr == "on") outState = OUT_ON;
				else if (outStateStr == "off") outState = OUT_OFF;
				else
				{
					outState = outStateStr.toInt();
					pwm_mode = true;
					if (outState > 1)
					{
						return "Incorrect value \"" + String(outState) + "\"";
					}
				}
#ifdef OUTPUT1_ENABLE
				if (outNum == 1)
				{
					if (pwm_mode) analogWrite(OUTPUT1_ENABLE, outState);
					else digitalWrite(OUTPUT1_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT2_ENABLE
				if (outNum == 2)
				{
					digitalWrite(OUTPUT2_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT3_ENABLE
				if (outNum == 3)
				{
					digitalWrite(OUTPUT3_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT4_ENABLE
				if (outNum == 4)
				{
					digitalWrite(OUTPUT4_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT5_ENABLE
				if (outNum == 5)
				{
					digitalWrite(OUTPUT5_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT6_ENABLE
				if (outNum == 6)
				{
					digitalWrite(OUTPUT6_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT7_ENABLE
				if (outNum == 7)
				{
					digitalWrite(OUTPUT7_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
#ifdef OUTPUT8_ENABLE
				if (outNum == 8)
				{
					digitalWrite(OUTPUT8_ENABLE, (bool)outState);
					str = "Output" + String(outNum) + "=" + String(int(outState));
				}
#endif
				if (str == "")	str = "Incorrect output #" + String(outNum) + "\r\n";
			}
		}
		else if (tmp.startsWith("set_interrupt_mode") && tmp.length() >= 21)
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

		else if (tmp == "reset")
		{
			Serial.println("Resetting...");
			ESP.restart();
		}

		else if (tmp.startsWith("autoreport="))
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


#ifdef SLEEP_ENABLE
		else if (tmp.startsWith("sleep_enable="))
		{
			char ar = command.substring(command.indexOf('=') + 1)[0];
			if (ar == '0')
			{
				SleepEnable = false;
				str = "SLEEP mode disabled\r\n";
			}
			else if (ar == '1')
			{
				SleepEnable = true;
				str = "SLEEP mode enabled\r\n";
			}
			else str = "Incorrect value: " + String(ar) + "\r\n";
		}
#endif

#ifdef HTTP_SERVER_ENABLE
		else if (tmp.startsWith("http_port="))
		{
			httpPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New HTTP port = \"" + String(httpPort) + "\"\r\n";
			writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
			if (httpServerEnable) http_server.begin(httpPort);
		}
		else if (tmp.startsWith("http_enable="))
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
		else if (tmp.startsWith("smtp_server="))
		{
			smtpServerAddress = command.substring(command.indexOf('=') + 1);
			str = "New SMTP server IP address = \"" + smtpServerAddress + "\"\r\n";
			writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, smtpServerAddress);
		}
		else if (tmp.startsWith("smtp_port="))
		{
			smtpServerPort = command.substring(command.indexOf('=') + 1).toInt();
			str = "New SMTP port = \"" + String(smtpServerPort) + "\"\r\n";
			writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
			WiFiServer server(telnetPort);
		}
		else if (tmp.startsWith("smtp_login="))
		{
			smtpLogin = command.substring(command.indexOf('=') + 1);
			str = "New SMTP login = \"" + smtpLogin + "\"\r\n";
			writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, smtpLogin);
		}
		else if (tmp.startsWith("smtp_pass="))
		{
			smtpPassword = command.substring(command.indexOf('=') + 1);
			str = "New SMTP password = \"" + smtpPassword + "\"\r\n";
			writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, smtpPassword);
		}
		else if (tmp.startsWith("smtp_to="))
		{
			smtpTo = command.substring(command.indexOf('=') + 1);
			str = "New SMTP TO address = \"" + smtpTo + "\"\r\n";
			writeConfigString(SMTP_TO_addr, SMTP_TO_size, smtpTo);
		}
		else if (tmp.startsWith("smtp_enable="))
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
		else if (tmp.startsWith("set_user"))
		{
			int t = command.indexOf('=');
			if (t >= 9 && t < tmp.length() - 1)
			{
				byte userNum = tmp.substring(8, t).toInt();
				uint64_t newUser = StringToUint64(tmp.substring(t + 1));
				if (userNum < telegramUsersNumber)
				{
					int m = TELEGRAM_USERS_TABLE_size / telegramUsersNumber;
					str = "User #" + String(userNum) + " is now: " + uint64ToString(newUser) + "\r\n";
					writeConfigString(TELEGRAM_USERS_TABLE_addr + userNum * m, m, uint64ToString(newUser));
					telegramUsers[0] = newUser;
				}
				else
				{
					str = "User #" + String(userNum) + " is out of range [0," + String(telegramUsersNumber - 1) + "]\r\n";
				}
			}
		}
		else if (tmp.startsWith("telegram_token="))
		{
			telegramToken = command.substring(command.indexOf('=') + 1);
			str = "New TELEGRAM token = \"" + telegramToken + "\"\r\n";
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, telegramToken);
			myBot.setTelegramToken(telegramToken);
		}
		else if (tmp.startsWith("telegram_enable="))
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
		// tmp += "[ADMIN][FLASH] gscript_enable=1/0\r\n";
		else if (tmp.startsWith("gscript_token="))
		{
			GScriptId = command.substring(command.indexOf('=') + 1);
			str = "New GScript token = \"" + GScriptId + "\"\r\n";
			writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, GScriptId);
		}
		else if (tmp.startsWith("gscript_enable="))
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
		else if (tmp.startsWith("pushingbox_token="))
		{
			pushingBoxId = command.substring(command.indexOf('=') + 1);
			str = "New PushingBox token = \"" + pushingBoxId + "\"\r\n";
			writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, pushingBoxId);
		}
		else if (tmp.startsWith("pushingbox_parameter="))
		{
			pushingBoxParameter = command.substring(command.indexOf('=') + 1);
			str = "New PUSHINGBOX parameter name = \"" + pushingBoxParameter + "\"\r\n";
			writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, pushingBoxParameter);
		}
		else if (tmp.startsWith("pushingbox_enable="))
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
		//config ntp_server
		else if (tmp.startsWith("ntp_server="))
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
		else if (tmp.startsWith("ntp_time_zone="))
		{
			timeZone = command.substring(command.indexOf('=') + 1).toInt();
			str = "New NTP time zone = \"" + String(timeZone) + "\"\r\n";
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
		}
		else if (tmp.startsWith("ntp_refresh_delay="))
		{
			ntpRefreshDelay = command.substring(command.indexOf('=') + 1).toInt();
			str = "New NTP refresh delay = \"" + String(ntpRefreshDelay) + "\"\r\n";
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);
		}
		else if (tmp.startsWith("ntp_enable="))
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

		// set_event?=""

		else if (tmp.startsWith("events_enable="))
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

		// set_schedule?=""

		else if (tmp.startsWith("scheduler_enable="))
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
		else
		{
			str = "Incorrect command: \"" + command + "\"\r\n";
			str += printHelp() + "\r\n";
		}
	}
	else
	{
		str = "Incorrect command: \"" + command + "\"\r\n";
		str += printHelp() + "\r\n";
	}
	return str;
}
