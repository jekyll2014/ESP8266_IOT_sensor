#pragma once
#include "configuration.h"

#define schedulesNumber  10
#define eventsNumber  10
#define telegramUsersNumber  10
#define gsmUsersNumber  10

// EEPROM config string position and size
#define DEVICE_NAME_addr	0
#define DEVICE_NAME_size	20

#define STA_SSID_addr	DEVICE_NAME_addr + DEVICE_NAME_size
#define STA_SSID_size	32

#define STA_PASS_addr	STA_SSID_addr + STA_SSID_size
#define STA_PASS_size	32

#define AP_SSID_addr	STA_PASS_addr + STA_PASS_size
#define AP_SSID_size	32

#define AP_PASS_addr	AP_SSID_addr + AP_SSID_size
#define AP_PASS_size	32

#define WIFI_STANDART_addr	AP_PASS_addr + AP_PASS_size
#define WIFI_STANDART_size	1

#define WIFI_POWER_addr	WIFI_STANDART_addr + WIFI_STANDART_size
#define WIFI_POWER_size	4

#define WIFI_MODE_addr	WIFI_POWER_addr + WIFI_POWER_size
#define WIFI_MODE_size	1

#define CONNECT_TIME_addr	WIFI_MODE_addr + WIFI_MODE_size
#define CONNECT_TIME_size	5

#define CONNECT_PERIOD_addr	CONNECT_TIME_addr + CONNECT_TIME_size
#define CONNECT_PERIOD_size	5

#define SENSOR_READ_DELAY_addr	CONNECT_PERIOD_addr + CONNECT_PERIOD_size
#define SENSOR_READ_DELAY_size	5

#define LOG_PERIOD_addr	SENSOR_READ_DELAY_addr + SENSOR_READ_DELAY_size
#define LOG_PERIOD_size	5

#define DISPLAY_REFRESH_addr	LOG_PERIOD_addr + LOG_PERIOD_size
#define DISPLAY_REFRESH_size	5

#define AUTOREPORT_addr  DISPLAY_REFRESH_addr + DISPLAY_REFRESH_size
#define AUTOREPORT_size  3

#define PIN_MODE_addr	AUTOREPORT_addr + AUTOREPORT_size
#define PIN_MODE_size	1 * PIN_NUMBER

#define OUTPUT_INIT_addr	PIN_MODE_addr + PIN_MODE_size
#define OUTPUT_INIT_size	4 * PIN_NUMBER

#define INTERRUPT_MODE_addr	OUTPUT_INIT_addr + OUTPUT_INIT_size
#define INTERRUPT_MODE_size	1 * PIN_NUMBER

#define SLEEP_ON_addr	INTERRUPT_MODE_addr + INTERRUPT_MODE_size
#define SLEEP_ON_size	5

#define SLEEP_OFF_addr	SLEEP_ON_addr + SLEEP_ON_size
#define SLEEP_OFF_size	5

#define SLEEP_ENABLE_addr	SLEEP_OFF_addr + SLEEP_OFF_size
#define SLEEP_ENABLE_size	1

#define EVENTS_TABLE_addr	SLEEP_ENABLE_addr + SLEEP_ENABLE_size
#define EVENTS_TABLE_size	100 * eventsNumber

#define EVENTS_ENABLE_addr	EVENTS_TABLE_addr + EVENTS_TABLE_size
#define EVENTS_ENABLE_size	1

#define SCHEDULER_TABLE_addr	EVENTS_ENABLE_addr + EVENTS_ENABLE_size
#define SCHEDULER_TABLE_size	100 * schedulesNumber

#define SCHEDULER_LASTRUN_TABLE_addr	SCHEDULER_TABLE_addr + SCHEDULER_TABLE_size
#define SCHEDULER_LASTRUN_TABLE_size	schedulesNumber * 4

#define SCHEDULER_ENABLE_addr	SCHEDULER_LASTRUN_TABLE_addr + SCHEDULER_LASTRUN_TABLE_size
#define SCHEDULER_ENABLE_size	1

#define NTP_SERVER_addr	SCHEDULER_ENABLE_addr + SCHEDULER_ENABLE_size
#define NTP_SERVER_size	100

#define NTP_TIME_ZONE_addr	NTP_SERVER_addr + NTP_SERVER_size
#define NTP_TIME_ZONE_size	2

#define NTP_REFRESH_DELAY_addr	NTP_TIME_ZONE_addr + NTP_TIME_ZONE_size
#define NTP_REFRESH_DELAY_size	5

#define NTP_ENABLE_addr	NTP_REFRESH_DELAY_addr + NTP_REFRESH_DELAY_size
#define NTP_ENABLE_size	1

#define TELNET_PORT_addr	NTP_ENABLE_addr + NTP_ENABLE_size
#define TELNET_PORT_size	5

#define TELNET_ENABLE_addr	TELNET_PORT_addr + TELNET_PORT_size
#define TELNET_ENABLE_size	1

#define HTTP_PORT_addr	TELNET_ENABLE_addr + TELNET_ENABLE_size
#define HTTP_PORT_size	5

#define HTTP_ENABLE_addr	HTTP_PORT_addr + HTTP_PORT_size
#define HTTP_ENABLE_size	1

#define MQTT_SERVER_addr  HTTP_ENABLE_addr + HTTP_ENABLE_size
#define MQTT_SERVER_size  100

#define MQTT_PORT_addr  MQTT_SERVER_addr + MQTT_SERVER_size
#define MQTT_PORT_size  5

#define MQTT_USER_addr  MQTT_PORT_addr + MQTT_PORT_size
#define MQTT_USER_size  100

#define MQTT_PASS_addr  MQTT_USER_addr + MQTT_USER_size
#define MQTT_PASS_size  100

#define MQTT_ID_addr  MQTT_PASS_addr + MQTT_PASS_size
#define MQTT_ID_size  100

#define MQTT_TOPIC_IN_addr  MQTT_ID_addr + MQTT_ID_size
#define MQTT_TOPIC_IN_size  100

#define MQTT_TOPIC_OUT_addr  MQTT_TOPIC_IN_addr + MQTT_TOPIC_IN_size
#define MQTT_TOPIC_OUT_size  100

#define MQTT_CLEAN_addr  MQTT_TOPIC_OUT_addr + MQTT_TOPIC_OUT_size
#define MQTT_CLEAN_size  1

#define MQTT_ENABLE_addr  MQTT_CLEAN_addr + MQTT_CLEAN_size
#define MQTT_ENABLE_size  1

#define TELEGRAM_TOKEN_addr	MQTT_ENABLE_addr + MQTT_ENABLE_size
#define TELEGRAM_TOKEN_size	50

#define TELEGRAM_USERS_TABLE_addr	TELEGRAM_TOKEN_addr + TELEGRAM_TOKEN_size
#define TELEGRAM_USERS_TABLE_size	telegramUsersNumber * 20

#define TELEGRAM_ENABLE_addr	TELEGRAM_USERS_TABLE_addr + TELEGRAM_USERS_TABLE_size
#define TELEGRAM_ENABLE_size	1

#define PUSHINGBOX_ID_addr	TELEGRAM_ENABLE_addr + TELEGRAM_ENABLE_size
#define PUSHINGBOX_ID_size	100

#define PUSHINGBOX_PARAM_addr	PUSHINGBOX_ID_addr + PUSHINGBOX_ID_size
#define PUSHINGBOX_PARAM_size	50

#define PUSHINGBOX_ENABLE_addr	PUSHINGBOX_PARAM_addr + PUSHINGBOX_PARAM_size
#define PUSHINGBOX_ENABLE_size	1

#define SMTP_SERVER_ADDRESS_addr	PUSHINGBOX_ENABLE_addr + PUSHINGBOX_ENABLE_size
#define SMTP_SERVER_ADDRESS_size	100

#define SMTP_SERVER_PORT_addr	SMTP_SERVER_ADDRESS_addr + SMTP_SERVER_ADDRESS_size
#define SMTP_SERVER_PORT_size	5

#define SMTP_LOGIN_addr	SMTP_SERVER_PORT_addr + SMTP_SERVER_PORT_size
#define SMTP_LOGIN_size	100

#define SMTP_PASSWORD_addr	SMTP_LOGIN_addr + SMTP_LOGIN_size
#define SMTP_PASSWORD_size	100

#define SMTP_TO_addr	SMTP_PASSWORD_addr + SMTP_PASSWORD_size
#define SMTP_TO_size	100

#define SMTP_ENABLE_addr  SMTP_TO_addr + SMTP_TO_size
#define SMTP_ENABLE_size  1

#define GSCRIPT_ID_addr	SMTP_ENABLE_addr + SMTP_ENABLE_size
#define GSCRIPT_ID_size	100

#define GSCRIPT_ENABLE_addr	GSCRIPT_ID_addr + GSCRIPT_ID_size
#define GSCRIPT_ENABLE_size	1

#define GSM_USERS_TABLE_addr	GSCRIPT_ENABLE_addr + GSCRIPT_ENABLE_size
#define GSM_USERS_TABLE_size	gsmUsersNumber * 15

#define GSM_ENABLE_addr	GSM_USERS_TABLE_addr + GSM_USERS_TABLE_size
#define GSM_ENABLE_size	1

#define RADIO_STOP 0
#define RADIO_CONNECTING 1
#define RADIO_CONNECTED 2
#define RADIO_WAITING 3

#define OUT_ON true
#define OUT_OFF false

#define HARD_UART 0
#define SOFT_UART 1

#define WIFI_MODE_OFF 0
#define WIFI_MODE_STA 1
#define WIFI_MODE_AP 2
#define WIFI_MODE_AP_STA 3
#define WIFI_MODE_AUTO 4
const String wifiModes[5] = { "Off", "Station", "Access point",  "Access point + Station", "Auto" };

#define CHANNELS_NUMBER 8
#define CHANNEL_UART 0
#define CHANNEL_TELNET 1
#define CHANNEL_MQTT 2
#define CHANNEL_TELEGRAM 3
#define CHANNEL_GSCRIPT 4
#define CHANNEL_PUSHINGBOX 5
#define CHANNEL_SMTP 6
#define CHANNEL_GSM 7
const String channels[CHANNELS_NUMBER] = { "UART", "TELNET", "MQTT", "TELEGRAM", "GSCRIPT", "PUSHINGBOX", "SMTP", "GSM" };

const String pinModeList[4] = { "INPUT","OUTPUT","INPUT_PULLUP", "OFF" };

const String intModeList[4] = { "OFF", "RISING", "FALLING" , "CHANGE" };

extern bool inputs[PIN_NUMBER];
extern uint16_t outputs[PIN_NUMBER];
extern uint32_t InterruptCounter[PIN_NUMBER];

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
	int adc;
#endif

	uint32_t InterruptCounters[PIN_NUMBER];

	bool inputs[PIN_NUMBER];

	bool outputs[PIN_NUMBER];
};

//D1=05, D2=04, D3=00, D4=02, D5=14, D6=12, D7=13, D8=15, reverse to binary order
//pins             D=87654321

#if defined(DS18B20_ENABLE) || defined(MH_Z19_UART_ENABLE) || defined(BME280_ENABLE) || defined(BMP180_ENABLE) || defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE)
#define TEMPERATURE_SENSOR
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(BME280_ENABLE) || defined(DHT_ENABLE)
#define HUMIDITY_SENSOR
#endif

#if defined(MH_Z19_UART_ENABLE) || defined(MH_Z19_PPM_ENABLE)
#define CO2_SENSOR
#endif

#if defined(BME280_ENABLE) || defined(BMP180_ENABLE)
#define PRESSURE_SENSOR
#endif

#if defined(TM1637DISPLAY_ENABLE) || defined(SSD1306DISPLAY_ENABLE)
#define DISPLAY_ENABLED
#endif

#if defined(GSM_M590_ENABLE) || defined(GSM_SIM800_ENABLE)
#define GSM_ENABLE
struct smsMessage
{
	bool IsAdmin;
	String PhoneNumber;
	String Message;
};
#endif

#if defined(GSM_M590_ENABLE)
#if GSM_M590_ENABLE == HARD_UART
#define HARD_UART_ENABLE
#elif GSM_M590_ENABLE == SOFT_UART
#define SOFT_UART_ENABLE
#endif
#endif

#if defined(GSM_SIM800_ENABLE)
#if GSM_SIM800_ENABLE == HARD_UART
#define HARD_UART_ENABLE
#elif GSM_SIM800_ENABLE == SOFT_UART
#define SOFT_UART_ENABLE
#endif
#endif

#if defined(MH_Z19_UART_ENABLE)
#if MH_Z19_UART_ENABLE == HARD_UART
#define HARD_UART_ENABLE
#elif MH_Z19_UART_ENABLE == SOFT_UART
#define SOFT_UART_ENABLE
#endif
#endif

#if defined(DEBUG_MODE)
#if DEBUG_MODE == HARD_UART
#define HARD_UART_ENABLE
#elif DEBUG_MODE == SOFT_UART
#define SOFT_UART_ENABLE
#endif
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(BME280_ENABLE) || defined(BMP180_ENABLE) || defined(SSD1306DISPLAY_ENABLE)
#define I2C_ENABLE
#endif

#if defined(DS18B20_ENABLE)
#define ONEWIRE_ENABLE
#endif

#if defined(GSM_M590_ENABLE) && defined(GSM_SIM800_ENABLE)
#error "Only one gms modem can use SoftUART"
#endif

#if defined(GSM_ENABLE) && defined(MH_Z19_UART_ENABLE)
#error "Only one device can use SoftUART"
#endif

#if defined(GSM_SIM800_ENABLE) && defined(MH_Z19_UART_ENABLE)
#error "Only one device can use SoftUART"
#endif
