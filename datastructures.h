#include <Arduino.h>

// EEPROM config strings size
#define SSID_addr	0
#define SSID_size	32

#define PASS_addr	SSID_addr + SSID_size
#define PASS_size	32

#define TELNET_PORT_addr	PASS_addr + PASS_size
#define TELNET_PORT_size	5

#define SENSOR_READ_DELAY_addr	TELNET_PORT_addr + TELNET_PORT_size
#define SENSOR_READ_DELAY_size	6

#define DEVICE_NAME_addr	SENSOR_READ_DELAY_addr + SENSOR_READ_DELAY_size
#define DEVICE_NAME_size	20

#define ENABLE_SLEEP_addr	DEVICE_NAME_addr + DEVICE_NAME_size
#define ENABLE_SLEEP_size	1

#define EVENTS_TABLE_addr	ENABLE_SLEEP_addr + ENABLE_SLEEP_size
#define EVENTS_TABLE_size	100 * eventsNumber

#define ENABLE_EVENTS_addr	EVENTS_TABLE_addr + EVENTS_TABLE_size
#define ENABLE_EVENTS_size	1

#define SCHEDULER_TABLE_addr	ENABLE_EVENTS_addr + ENABLE_EVENTS_size
#define SCHEDULER_TABLE_size	100 * schedulesNumber

#define ENABLE_SCHEDULER_addr	SCHEDULER_TABLE_addr + SCHEDULER_TABLE_size
#define ENABLE_SCHEDULER_size	1

#define SMTP_SERVER_ADDRESS_addr	ENABLE_SCHEDULER_addr + ENABLE_SCHEDULER_size
#define SMTP_SERVER_ADDRESS_size	100

#define SMTP_SERVER_PORT_addr	SMTP_SERVER_ADDRESS_addr + SMTP_SERVER_ADDRESS_size
#define SMTP_SERVER_PORT_size	5

#define SMTP_LOGIN_addr	SMTP_SERVER_PORT_addr + SMTP_SERVER_PORT_size
#define SMTP_LOGIN_size	100

#define SMTP_PASSWORD_addr	SMTP_LOGIN_addr + SMTP_LOGIN_size
#define SMTP_PASSWORD_size	100

#define SMTP_TO_addr	SMTP_PASSWORD_addr + SMTP_PASSWORD_size
#define SMTP_TO_size	100

#define ENABLE_EMAIL_addr  SMTP_TO_addr + SMTP_TO_size
#define ENABLE_EMAIL_size  1

#define TELEGRAM_TOKEN_addr	ENABLE_EMAIL_addr + ENABLE_EMAIL_size
#define TELEGRAM_TOKEN_size	50

#define TELEGRAM_USERS_TABLE_addr	TELEGRAM_TOKEN_addr + TELEGRAM_TOKEN_size
#define TELEGRAM_USERS_TABLE_size	telegramUsersNumber * 20

#define ENABLE_TELEGRAM_addr	TELEGRAM_USERS_TABLE_addr + TELEGRAM_USERS_TABLE_size
#define ENABLE_TELEGRAM_size	1

#define GSCRIPT_ID_addr	ENABLE_TELEGRAM_addr + ENABLE_TELEGRAM_size
#define GSCRIPT_ID_size	100

#define ENABLE_GSCRIPT_addr	GSCRIPT_ID_addr + GSCRIPT_ID_size
#define ENABLE_GSCRIPT_size	1

#define PUSHINGBOX_ID_addr	ENABLE_GSCRIPT_addr + ENABLE_GSCRIPT_size
#define PUSHINGBOX_ID_size	100

#define PUSHINGBOX_PARAM_addr	PUSHINGBOX_ID_addr + PUSHINGBOX_ID_size
#define PUSHINGBOX_PARAM_size	50

#define ENABLE_PUSHINGBOX_addr	PUSHINGBOX_PARAM_addr + PUSHINGBOX_PARAM_size
#define ENABLE_PUSHINGBOX_size	1

#define HTTP_PORT_addr	ENABLE_PUSHINGBOX_addr + ENABLE_PUSHINGBOX_size
#define HTTP_PORT_size	5

#define ENABLE_HTTP_addr	HTTP_PORT_addr + HTTP_PORT_size
#define ENABLE_HTTP_size	1

#define NTP_SERVER_addr	ENABLE_HTTP_addr + ENABLE_HTTP_size
#define NTP_SERVER_size	15

#define NTP_TIME_ZONE_addr	NTP_SERVER_addr + NTP_SERVER_size
#define NTP_TIME_ZONE_size	3

#define NTP_REFRESH_DELAY_addr	NTP_TIME_ZONE_addr + NTP_TIME_ZONE_size
#define NTP_REFRESH_DELAY_size	5

#define ENABLE_NTP_addr	NTP_REFRESH_DELAY_addr + NTP_REFRESH_DELAY_size
#define ENABLE_NTP_size	1

#define INT1_MODE_addr	ENABLE_NTP_addr + ENABLE_NTP_size
#define INT1_MODE_size	1

#define INT2_MODE_addr	INT1_MODE_addr + INT1_MODE_size
#define INT2_MODE_size	1

#define INT3_MODE_addr	INT2_MODE_addr + INT2_MODE_size
#define INT3_MODE_size	1

#define INT4_MODE_addr	INT3_MODE_addr + INT3_MODE_size
#define INT4_MODE_size	1

#define INT5_MODE_addr	INT4_MODE_addr + INT4_MODE_size
#define INT5_MODE_size	1

#define INT6_MODE_addr	INT5_MODE_addr + INT5_MODE_size
#define INT6_MODE_size	1

#define INT7_MODE_addr	INT6_MODE_addr + INT6_MODE_size
#define INT7_MODE_size	1

#define INT8_MODE_addr	INT7_MODE_addr + INT7_MODE_size
#define INT8_MODE_size	1

#define UART_CHANNEL 0
#define TELNET_CHANNEL 1
#define TELEGRAM_CHANNEL 2

#define WIFI_STOP 0
#define WIFI_CONNECTING 1
#define WIFI_CONNECTED 2

#define OUT_ON false
#define OUT_OFF true

typedef struct sensorDataCollection
{
	int logyear;
	byte logmonth;
	byte logday;
	byte loghour;
	byte logminute;
	byte logsecond;

#ifdef AMS2320_ENABLE
	int ams_temp;
	byte ams_humidity;
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

#ifdef INTERRUPT_COUNTER1_ENABLE
	unsigned long int InterruptCounter1;
#endif
#ifdef INTERRUPT_COUNTER2_ENABLE
	unsigned long int InterruptCounter2;
#endif
#ifdef INTERRUPT_COUNTER3_ENABLE
	unsigned long int InterruptCounter3;
#endif
#ifdef INTERRUPT_COUNTER4_ENABLE
	unsigned long int InterruptCounter4;
#endif
#ifdef INTERRUPT_COUNTER5_ENABLE
	unsigned long int InterruptCounter5;
#endif
#ifdef INTERRUPT_COUNTER6_ENABLE
	unsigned long int InterruptCounter6;
#endif
#ifdef INTERRUPT_COUNTER7_ENABLE
	unsigned long int InterruptCounter7;
#endif
#ifdef INTERRUPT_COUNTER8_ENABLE
	unsigned long int InterruptCounter8;
#endif

#ifdef INPUT1_ENABLE
	bool input1;
#endif
#ifdef INPUT2_ENABLE
	bool input2;
#endif
#ifdef INPUT3_ENABLE
	bool input3;
#endif
#ifdef INPUT4_ENABLE
	bool input4;
#endif
#ifdef INPUT5_ENABLE
	bool input5;
#endif
#ifdef INPUT6_ENABLE
	bool input6;
#endif
#ifdef INPUT7_ENABLE
	bool input7;
#endif
#ifdef INPUT8_ENABLE
	bool input8;
#endif

#ifdef OUTPUT1_ENABLE
	bool output1;
#endif
#ifdef OUTPUT2_ENABLE
	bool output2;
#endif
#ifdef OUTPUT3_ENABLE
	bool output3;
#endif
#ifdef OUTPUT4_ENABLE
	bool output4;
#endif
#ifdef OUTPUT5_ENABLE
	bool output5;
#endif
#ifdef OUTPUT6_ENABLE
	bool output6;
#endif
#ifdef OUTPUT7_ENABLE
	bool output7;
#endif
#ifdef OUTPUT8_ENABLE
	bool output8;
#endif
};
