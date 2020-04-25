#pragma once
//#define DEBUG_MODE

#define PIN_NUMBER 8
const uint8_t pins[PIN_NUMBER] = { 5, 4, 0, 2, 14, 12, 13, 15 }; //D1=05, D2=04, D3=00, D4=02, D5=14, D6=12, D7=13, D8=15, reverse to binary order

// **** SERVICES

//#define SLEEP_ENABLE //connect D0(16) and RST pins to start controller after sleep
#define NTP_TIME_ENABLE
#define HTTP_SERVER_ENABLE
#define EVENTS_ENABLE
#define SCHEDULER_ENABLE
#define MQTT_ENABLE
#define TELEGRAM_ENABLE
#define PUSHINGBOX_ENABLE
//#define SMTP_ENABLE
//#define GSCRIPT_ENABLE
//#define LOG_ENABLE

// **** SENSORS

//#define ADC_ENABLE
//#define AMS2320_ENABLE
//#define HTU21D_ENABLE
//#define BME280_ENABLE
#define BMP180_ENABLE
#define DS18B20_ENABLE

//#define DHT_ENABLE								DHT11 //DHT11, DHT21, DHT22 
//#define DHT_PIN										14 //D5

//#define MH_Z19_UART_ENABLE

//#define MH_Z19_PPM_ENABLE					12 //D6

//#define TM1637DISPLAY_ENABLE
//#define TM1637_CLK								0 //D3
//#define TM1637_DIO								2 //D4

#define SSD1306DISPLAY_ENABLE

#define GSM_M590_ENABLE
//#define GSM_SIM800_ENABLE

// **** PIN-OUT

//old Esp 1-3
//#define SIGNALS_PINOUT						0b00000011 //old
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old

//Esp4
//#define SIGNALS_PINOUT						0b00001001 //new Esp 4
//#define PIN_WIRE_SCL							05 //D1 new
//#define PIN_WIRE_SDA							02 //D4 new
//#define ONEWIRE_DATA							00 //D3

//Esp+4digit display
//#define SIGNALS_PINOUT						0b00110011 //old
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old
//#define UART2_TX									14 //D5 old
//#define UART2_RX									12 //D6 old

//Esp office
//#define SIGNALS_PINOUT						0b00110011 //Esp office
//#define PIN_WIRE_SCL							05 //D1 old
//#define PIN_WIRE_SDA							04 //D2 old
//#define UART2_TX									14 //D5 old
//#define UART2_RX									12 //D6 old

//Esp+GSM
#define SIGNALS_PINOUT						0b01101001 //new
#define PIN_WIRE_SCL							05 //D1 new
#define PIN_WIRE_SDA							02 //D4 new
#define ONEWIRE_DATA							00 //D3
#define UART2_TX									12 //D6 new
#define UART2_RX									13 //D7 new

//Esp-EPAM
//#define SIGNALS_PINOUT						0b00001001 //new
//#define PIN_WIRE_SCL							05 //D1 new
//#define PIN_WIRE_SDA							02 //D4 new
//#define ONEWIRE_DATA							00 //D3

//new pinout
//Pin - Pin - Function
// 05 -  D1 -   + I2C_SCL
// 04 - ~D2 -   + IN/OUT2
// 00 -  D3 -   - keep HIGH to boot FLASH, LOW to boot UART, 1-Wire, TM1637_CLK
// 02 -  D4 -   + keep HIGH on boot, I2C_SDA, Serial1_TX, TM1637_DIO
// 14 - ~D5 -   + IN/OUT5
// 12 - ~D6 -   + SoftUART_TX
// 13 -  D7 -   + SoftUART_RX
// 15 - ~D8 -   - keep LOW on boot, OUT8

//old pinout
//Pin - Pin - Function
// 05 -  D1 - I2C_SCL
// 04 - ~D2 - I2C_SDA
// 00 -  D3 - TM1637_CLK
// 02 -  D4 - TM1637_DIO
// 14 - ~D5 - SoftUART_TX
// 12 - ~D6 - SoftUART_RX
// 13 -  D7 - 
// 15 - ~D8 - 
