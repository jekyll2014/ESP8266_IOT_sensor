#pragma once

//#define DEBUG_MODE		HARD_UART //SOFT_UART //not implemented yet
#define HARD_UART_ENABLE 115200

#define PIN_NUMBER 10
const uint8_t pins[PIN_NUMBER] = { 5, 4, 0, 2, 14, 12, 13, 15, 3, 1 }; //D1=05, D2=04, D3=00, D4=02, D5=14, D6=12, D7=13, D8=15, reverse to binary order

// **** SERVICES

//#define SLEEP_ENABLE //connect D0(16) and RST pins to start controller after sleep
#define NTP_TIME_ENABLE
#define TELNET_ENABLE							5 // max. connections
//#define HTTP_ENABLE
#define EVENTS_ENABLE
#define SCHEDULER_ENABLE
#define MQTT_ENABLE
#define TELEGRAM_ENABLE
//#define PUSHINGBOX_ENABLE
//#define SMTP_ENABLE
//#define GSCRIPT_ENABLE
//#define LOG_ENABLE
//#define BUZZER_ENABLE							12 // pin for the buzzer

// **** SENSORS

#define ADC_ENABLE
#define AMS2320_ENABLE						0x5C
#define HTU21D_ENABLE							0x40
#define BME280_ENABLE							0xF6
#define BMP180_ENABLE							0x77
#define DS18B20_ENABLE						0x28
#define AHTx0_ENABLE							0x38 //not tested yet

//#define DHT_ENABLE								DHT11 //DHT11, DHT21, DHT22
//#define DHT_PIN										14 //D5

//#define MH_Z19_UART_ENABLE				SOFT_UART //SOFT_UART, HARD_UART //not implemented yet

//#define MH_Z19_PPM_ENABLE					12 //D6

//#define TM1637DISPLAY_ENABLE
//#define TM1637_CLK								04 //D2
//#define TM1637_DIO								14 //D5

//#define SSD1306DISPLAY_ENABLE			0x3C

//#define GSM_M590_ENABLE						SOFT_UART //HARD_UART //not implemented yet
//#define GSM_SIM800_ENABLE					SOFT_UART //HARD_UART //not implemented yet

// **** PIN-OUT

//old Esp 1-3
//#define SIGNALS_PINOUT						0b00000011 //old
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old

//Esp4
//#define SIGNALS_PINOUT						0b00001001 //new Esp 4
//#define PIN_WIRE_SCL							05 //D1 new
//#define PIN_WIRE_SDA							04 //D4 new
//#define ONEWIRE_DATA							00 //D3

//Esp+4digit display
//#define SIGNALS_PINOUT						0b00110011 //old
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old
//#define SOFT_UART_TX							14 //D5 old
//#define SOFT_UART_RX							12 //D6 old

//Esp ClimateMonitor
//#define SIGNALS_PINOUT						0b00110011 //Esp office
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old
//#define SOFT_UART_TX							14 //D5 old
//#define SOFT_UART_RX							12 //D6 old

//Esp+GSM
//#define SIGNALS_PINOUT						0b0001101001 //new
//#define PIN_WIRE_SCL							05 //D1 new
//#define PIN_WIRE_SDA							02 //D4 new
//#define ONEWIRE_DATA							00 //D3
//#define HARD_UART_TX							01 //D10 new
//#define HARD_UART_RX							03 //D9 new
//#define SOFT_UART_TX							12 //D6 new
//#define SOFT_UART_RX							13 //D7 new

//Esp+GSM SIM800L
//#define SIGNALS_PINOUT						0b0001101001 //new
#define PIN_WIRE_SCL							14 //D5 new
#define PIN_WIRE_SDA							02 //D4 new
#define ONEWIRE_DATA							00 //D3
#define HARD_UART_TX							01 //D10 new
#define HARD_UART_RX							03 //D9 new
//#define SOFT_UART_TX									12 //D6 new
//#define SOFT_UART_RX									13 //D7 new

//Esp-EPAM
//#define SIGNALS_PINOUT						0b00001001 //new
//#define PIN_WIRE_SCL							05 //D1 new
//#define PIN_WIRE_SDA							02 //D4 new
//#define ONEWIRE_DATA							00 //D3

//pinout v2
//Pin - Pin - Function
// 05 -  D1 -   + IN/OUT1
// 04 - ~D2 -   + IN/OUT2
// 00 -  D3 -   - keep HIGH to boot FLASH, LOW to boot UART, 1-Wire, TM1637_CLK
// 02 -  D4 -   + keep HIGH on boot, I2C_SDA, Serial1_TX, TM1637_DIO
// 14 - ~D5 -   + I2C_SCL
// 12 - ~D6 -   + SoftUART_TX
// 13 -  D7 -   + SoftUART_RX
// 15 - ~D8 -   - keep LOW on boot, OUT8

//pinout v1
//Pin - Pin - Function
// 05 -  D1 -   + I2C_SCL
// 04 - ~D2 -   + IN/OUT2
// 00 -  D3 -   - keep HIGH to boot FLASH, LOW to boot UART, 1-Wire, TM1637_CLK
// 02 -  D4 -   + keep HIGH on boot, I2C_SDA, Serial1_TX, TM1637_DIO
// 14 - ~D5 -   + IN/OUT5
// 12 - ~D6 -   + SoftUART_TX
// 13 -  D7 -   + SoftUART_RX
// 15 - ~D8 -   - keep LOW on boot, OUT8

//pinout v0
//Pin - Pin - Function
// 05 -  D1 - I2C_SCL
// 04 - ~D2 - I2C_SDA
// 00 -  D3 - TM1637_CLK
// 02 -  D4 - TM1637_DIO
// 14 - ~D5 - SoftUART_TX
// 12 - ~D6 - SoftUART_RX
// 13 -  D7 - 
// 15 - ~D8 - 
