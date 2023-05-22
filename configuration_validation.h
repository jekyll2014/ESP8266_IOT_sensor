#pragma once

#include "configuration.h"

#if defined(DS18B20_ENABLE) || defined(MH_Z19_UART_ENABLE) || defined(BME280_ENABLE) || defined(BMP180_ENABLE) || defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(DHT_ENABLE) || defined(AHTx0_ENABLE)
#define TEMPERATURE_SENSOR
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(BME280_ENABLE) || defined(DHT_ENABLE) || defined(AHTx0_ENABLE)
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

#if defined(TM1637DISPLAY_ENABLE)
#if !defined(TM1637_CLK) || !defined(TM1637_DIO)
#error "TM1637 display pins not defined"
#endif
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
#if !defined(HARD_UART_ENABLE) || !defined(HARD_UART_TX)||!defined(HARD_UART_RX)||!defined(HARD_UART_SPEED)
#error "Hardware UART not enabled or pins not defined"
#endif
#elif GSM_M590_ENABLE == SOFT_UART
#if !defined(SOFT_UART_ENABLE) || !defined(SOFT_UART_TX)||!defined(SOFT_UART_RX)||!defined(SOFT_UART_SPEED)
#error "Software UART not enabled or pins not defined"
#endif
#endif
#endif

#if defined(GSM_SIM800_ENABLE)
#if GSM_SIM800_ENABLE == HARD_UART		
#if !defined(HARD_UART_ENABLE) || !defined(HARD_UART_TX)||!defined(HARD_UART_RX)||!defined(HARD_UART_SPEED)
#error "Hardware UART not enabled or pins not defined"
#endif
#elif GSM_SIM800_ENABLE == SOFT_UART
#if !defined(SOFT_UART_ENABLE) || !defined(SOFT_UART_TX)||!defined(SOFT_UART_RX)||!defined(SOFT_UART_SPEED)
#error "Software UART not enabled or pins not defined"
#endif
#endif
#endif

#if defined(MH_Z19_UART_ENABLE)
#if MH_Z19_UART_ENABLE == HARD_UART		
#if !defined(HARD_UART_ENABLE) || !defined(HARD_UART_TX)||!defined(HARD_UART_RX)||!defined(HARD_UART_SPEED)
#error "Hardware UART not enabled or pins not defined"
#endif
#elif MH_Z19_UART_ENABLE == SOFT_UART
#if !defined(SOFT_UART_ENABLE) || !defined(SOFT_UART_TX)||!defined(SOFT_UART_RX)||!defined(SOFT_UART_SPEED)
#error "Software UART not enabled or pins not defined"
#endif
#endif
#endif

#if defined(DEBUG_MODE)
#if DEBUG_MODE == HARD_UART		
#if !defined(HARD_UART_ENABLE) || !defined(HARD_UART_TX)||!defined(HARD_UART_RX)||!defined(HARD_UART_SPEED)
#error "Hardware UART not enabled or pins not defined"
#endif
#elif DEBUG_MODE == SOFT_UART
#if !defined(SOFT_UART_ENABLE) || !defined(SOFT_UART_TX)||!defined(SOFT_UART_RX)||!defined(SOFT_UART_SPEED)
#error "Software UART not enabled or pins not defined"
#endif
#endif
#endif

#if defined(AMS2320_ENABLE) || defined(HTU21D_ENABLE) || defined(BME280_ENABLE) || defined(BMP180_ENABLE) || defined(SSD1306DISPLAY_ENABLE)
#define I2C_ENABLE
#endif

#if defined(I2C_ENABLE) && (!defined(PIN_WIRE_SCL) || !defined(PIN_WIRE_SDA))
#error "I2C pins not defined"
#endif

#if defined(DS18B20_ENABLE)
#define ONEWIRE_ENABLE
#endif

#if defined(ONEWIRE_ENABLE) && !defined(ONEWIRE_DATA)
#error "1Wire pin not defined"
#endif

#if defined(GSM_M590_ENABLE) && defined(GSM_SIM800_ENABLE)
#error "Only one gms modem can use SoftUART"
#endif

#if defined(GSM_ENABLE) && defined(MH_Z19_UART_ENABLE)
#error "Only one device can use SoftUART"
#endif

/*#if FINAL_CRC_addr > 4090
#error "EEPROM size can't be more than 4096 bytes'"
#endif*/