# ESP8266_IOT_sensor

IoT device based on ESP8266 for smart home.

Works over WiFi, UART and GSM.

Supports AP/Station/Mix Wi-Fi modes.

Sensors reporting and device control over (no SSL):
- UART
- Telnet
- HTTP
- SMTP (e-mail)
- MQTT
- Telegram
- GoogleScript
- PushingBox

NTP time update

Sleep management

Event management:
- input?=[on, off, change]
- output?=[on, off, change]
- counter>n
- adc<>n
- temperature<>n
- humidity<>n
- co2<>n

Schedule management:
- daily@hh:mm;action1;action2;...
- weekly@wd.hh:mm;action1;action2;...
- monthly@md.hh:mm;action1;action2;...
- once@yyyy.mm.dd.hh:mm;action1;action2;...

Event/Schedule actions:
- command=[any command]
- set_output?=[on, off, 0..1023]
- reset_counter?
- set_event_flag?=0/1
- reset_flag?
- set_flag?
- send_telegram=[*/user#],message
- send_PushingBox=message
- send_mail=address,message
- send_GScript=message
- send_mqtt=message
- send_sms=[*/user#],message

Peripheral devices supported:

Sensors
- ADC (<=1.2V)
- AMS2320
- HTU21D
- BME280
- BMP180
- DS18B20
- DHT11/DHT21/DHT22 
- MH Z19 (UART or PPM)

Displays
- TM1637
- SSD1306

GSM modems (SMS management only)
- GSM_M590_ENABLE
- SIM800 (not tested yet)
  
Compile-time configuration to support desired sensors/modules only and reduce RAM/FLASH size.

Run-time pin-out configuration.
