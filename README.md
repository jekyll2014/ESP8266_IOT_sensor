# ESP8266_IOT_sensor

IoT device based on ESP8266 for smart home.
Works over WiFi, UART and GSM.
Supports AP/Station/Mix Wi-Fi modes.

Sensors reporting and device control over (no SSL):
- Telnet
- SMTP
- MQTT
- Telegram
- GoogleScript
- PushingBox

NTP time update
Sleep management

Event management:
	input?=[on, off, change]
	output?=[on, off, change]
	counter>n
	adc<>n
	temperature<>n
	humidity<>n
	co2<>n

Schedule management:
	daily@hh:mm;action1;action2;...
	weekly@wd.hh:mm;action1;action2;...
	monthly@md.hh:mm;action1;action2;...
	once@yyyy.mm.dd.hh:mm;action1;action2;...

Event/Schedule actions:
	command=[any command]
	set_output?=[on, off, 0..1023]
	reset_counter?
	set_event_flag?=0/1
	reset_flag?
	set_flag?
	send_telegram=[*/user#],message
	send_PushingBox=message
	send_mail=address,message
	send_GScript=message
	send_mqtt=message
	send_sms=[*/user#],message

Easy compile-time configuration to keep only desired sensors/modules.
Easy pin-out configuration on run-time.
