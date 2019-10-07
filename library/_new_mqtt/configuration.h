#pragma once
//#define DEBUG_MODE

//#define SLEEP_ENABLE //connect D0 and EN pins to start controller after sleep
#define NTP_TIME_ENABLE
//#define HTTP_SERVER_ENABLE
//#define EVENTS_ENABLE
//#define SCHEDULER_ENABLE

#define AMS2320_ENABLE
#define HTU21D_ENABLE
#define BME280_ENABLE
#define DS18B20_ENABLE
//#define DHT_ENABLE
//#define MH_Z19_UART_ENABLE
//#define MH_Z19_PPM_ENABLE
//#define TM1637DISPLAY_ENABLE
//#define SSD1306DISPLAY_ENABLE

//#define MQTT_ENABLE
//#define TELEGRAM_ENABLE
//#define PUSHINGBOX
//#define EMAIL_ENABLE
//#define GSCRIPT

//#define ADC_ENABLE

//#define INTERRUPT_COUNTER1_ENABLE 5		// D1 - I2C_SCL
//#define INTERRUPT_COUNTER2_ENABLE 4		//~D2 - I2C_SDA
//#define INTERRUPT_COUNTER3_ENABLE 0		// D3 - FLASH, TM1637_CLK, keep HIGH on boot to boot from FLASH or LOW to boot from UART
//#define INTERRUPT_COUNTER4_ENABLE 2		// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INTERRUPT_COUNTER5_ENABLE 14	//~D5 - SoftUART_TX
//#define INTERRUPT_COUNTER6_ENABLE 12	//~D6 - SoftUART_RX
//#define INTERRUPT_COUNTER7_ENABLE 13	// D7
//#define INTERRUPT_COUNTER8_ENABLE 15	//~D8 - keep LOW on boot

//#define INPUT1_ENABLE 5					// D1 - I2C_SCL
//#define INPUT2_ENABLE 4					//~D2 - I2C_SDA
//#define INPUT3_ENABLE 0					// D3 - FLASH, TM1637_CLK
//#define INPUT4_ENABLE 2					// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define INPUT5_ENABLE 14				//~D5 - SoftUART_TX
//#define INPUT6_ENABLE 12				//~D6 - SoftUART_RX
//#define INPUT7_ENABLE 13				// D7
//#define INPUT8_ENABLE 15				//~D8 - keep LOW on boot

//#define OUTPUT1_ENABLE 5				// D1 - I2C_SCL
//#define OUTPUT2_ENABLE 4				//~D2 - I2C_SDA
//#define OUTPUT3_ENABLE 0				// D3 - FLASH, TM1637_CLK
//#define OUTPUT4_ENABLE 2				// D4 - Serial1_TX, TM1637_DIO, keep HIGH on boot
//#define OUTPUT5_ENABLE 14				//~D5 - SoftUART_TX
//#define OUTPUT6_ENABLE 12				//~D6 - SoftUART_RX
//#define OUTPUT7_ENABLE 13				// D7
//#define OUTPUT8_ENABLE 15				//~D8 - keep LOW on boot
