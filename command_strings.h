#pragma once

#define CMD_AP_PASS							F("ap_pass") //ap_pass
#define REPLY_AP_PASS						F("WiFi AP password") //

#define CMD_AP_SSID							F("ap_ssid") //ap_ssid
#define REPLY_AP_SSID						F("WiFi AP SSID") //

#define CMD_AUTOREPORT						F("autoreport") //autoreport
#define REPLY_AUTOREPORT					F("Autoreport channels") //

#define CMD_BUZZ							F("buzz") //buzz

#define CMD_CHECK_PERIOD					F("check_period") //check_period
#define REPLY_CHECK_PERIOD					F("Sensor read period")

#define CMD_CLEAR_SCHEDULE_EXEC_TIME		F("clear_schedule_exec_time") //clear_schedule_exec_time
#define REPLY_CLEAR_SCHEDULE_EXEC_TIME		F("Schedule #") //

#define CMD_DEVICE_NAME						F("device_name") //device_name
#define REPLY_DEVICE_NAME					F("Device name") //

#define CMD_DISPLAY_REFRESH					F("display_refresh") //display_refresh
#define REPLY_DISPLAY_REFRESH				F("Display refresh period") //

#define CMD_EVENTS_ENABLE					F("events_enable") //events_enable
#define REPLY_EVENTS_ENABLE					F("EVENTS service") //

#define CMD_GETLOG							F("getlog") //getlog

#define CMD_GET_CONFIG						F("get_config") //get_config

#define CMD_GET_SENSOR						F("get_sensor") //get_sensor

#define CMD_GET_STATUS						F("get_status") //get_status

#define CMD_GSCRIPT_ENABLE					F("gscript_enable") //gscript_enable
#define REPLY_GSCRIPT_ENABLE				F("GScript service") //

#define CMD_GSCRIPT_TOKEN					F("gscript_token") //gscript_token
#define REPLY_GSCRIPT_TOKEN					F("GScript token") //

#define CMD_GSM_ENABLE						F("gsm_enable") //gsm_enable
#define REPLY_GSM_ENABLE					F("GSM service") //

#define CMD_GSM_USER						F("gsm_user") //gsm_user
#define REPLY_GSM_USER						F("Phone #") //

#define CMD_HELP							F("help") //help

#define CMD_HELP_ACTION						F("help_action") //help_action

#define CMD_HELP_EVENT						F("help_event") //help_event

#define CMD_HELP_SCHEDULE					F("help_schedule") //help_schedule

#define CMD_HTTP_ENABLE						F("http_enable") //http_enable
#define REPLY_HTTP_ENABLE					F("HTTP service") //

#define CMD_HTTP_PORT						F("http_port") //http_port
#define REPLY_HTTP_PORT						F("HTTP port") //

#define CMD_LOG_PERIOD						F("log_period") //log_period
#define REPLY_LOG_PERIOD					F("Log save period") //

#define CMD_MQTT_CLEAN						F("mqtt_clean") //mqtt_clean
#define REPLY_MQTT_CLEAN					F("MQTT clean session") //

#define CMD_MQTT_ENABLE						F("mqtt_enable") //mqtt_enable
#define REPLY_MQTT_ENABLE					F("MQTT service") //

#define CMD_MQTT_ID							F("mqtt_id") //mqtt_id
#define REPLY_MQTT_ID						F("MQTT ID") //

#define CMD_MQTT_LOGIN						F("mqtt_login") //mqtt_login
#define REPLY_MQTT_LOGIN					F("MQTT login") //

#define CMD_MQTT_PASS						F("mqtt_pass") //mqtt_pass
#define REPLY_MQTT_PASS						F("MQTT password") //

#define CMD_MQTT_PORT						F("mqtt_port") //mqtt_port
#define REPLY_MQTT_PORT						F("MQTT port") //

#define CMD_MQTT_SERVER						F("mqtt_server") //mqtt_server
#define REPLY_MQTT_SERVER					F("MQTT server") //

#define CMD_MQTT_TOPIC_IN					F("mqtt_topic_in") //mqtt_topic_in
#define REPLY_MQTT_TOPIC_IN					F("MQTT receive topic") //

#define CMD_MQTT_TOPIC_OUT					F("mqtt_topic_out") //mqtt_topic_out
#define REPLY_MQTT_TOPIC_OUT				F("MQTT post topic") //

#define CMD_NTP_ENABLE						F("ntp_enable") //ntp_enable
#define REPLY_NTP_ENABLE					F("NTP service") //

#define CMD_NTP_REFRESH_DELAY				F("ntp_refresh_delay") //ntp_refresh_delay
#define REPLY_NTP_REFRESH_DELAY				F("NTP refresh delay") //

#define CMD_NTP_REFRESH_PERIOD				F("ntp_refresh_period") //ntp_refresh_period
#define REPLY_NTP_REFRESH_PERIOD			F("NTP refresh period") //

#define CMD_NTP_SERVER						F("ntp_server") //ntp_server
#define REPLY_NTP_SERVER					F("NTP server") //

#define CMD_NTP_TIME_ZONE					F("ntp_time_zone") //ntp_time_zone
#define REPLY_NTP_TIME_ZONE					F("NTP time zone") //

#define CMD_OTA_ENABLE						F("ota_enable") //display_refresh
#define REPLY_OTA_ENABLE					F("OTA service") //

#define CMD_OTA_PASSWORD					F("ota_pass") //display_refresh
#define REPLY_OTA_PASSWORD					F("OTA password") //

#define CMD_OTA_PORT						F("ota_port") //display_refresh
#define REPLY_OTA_PORT						F("OTA port") //

#define CMD_PUSHINGBOX_ENABLE				F("pushingbox_enable") //pushingbox_enable
#define REPLY_PUSHINGBOX_ENABLE				F("PushingBox service") //

#define CMD_PUSHINGBOX_PARAMETER			F("pushingbox_parameter") //pushingbox_parameter
#define REPLY_PUSHINGBOX_PARAMETER			F("PUSHINGBOX parameter name") //

#define CMD_PUSHINGBOX_TOKEN				F("pushingbox_token") //pushingbox_token
#define REPLY_PUSHINGBOX_TOKEN				F("PushingBox token") //

#define CMD_RESET							F("reset") //reset
#define CFG_RESET							F("Resetting...") //reset

#define CMD_SCHEDULER_ENABLE				F("scheduler_enable") //scheduler_enable
#define REPLY_SCHEDULER_ENABLE				F("SCHEDULER service") //

#define CMD_SEND_MAIL						F("send_mail") //send_mail=address,subject,message

#define CMD_SEND_MQTT						F("send_mqtt") //send_mqtt

#define CMD_SEND_SMS						F("send_sms") //send_sms

#define CMD_SEND_TELEGRAM					F("send_telegram") //send_telegram

#define CMD_EVENT_SET						F("set_event") //set_event
#define REPLY_EVENT_SET						F("EVENT #") //

#define CMD_EVENT_FLAG_SET					F("set_event_flag") //set_event_flag
#define REPLY_EVENT_FLAG_SET				F("EVENT flag #") //

#define CMD_OUTPUT_INIT_SET					F("set_init_output") //set_init_output
#define REPLY_OUTPUT_INIT_SET				F("Init OUT") //

#define CMD_INTERRUPT_MODE_SET				F("set_interrupt_mode") //set_interrupt_mode
#define REPLY_INTERRUPT_MODE_SET			F("INT") //

#define CMD_OUTPUT_SET						F("set_output") //set_output
#define REPLY_OUTPUT_SET					F("OUT") //

#define CMD_PIN_MODE_SET					F("set_pin_mode") //set_pin_mode
#define REPLY_PIN_MODE_SET					F("Pin") //

#define CMD_SCHEDULE_SET					F("set_schedule") //set_schedule
#define REPLY_SCHEDULE_SET					F("SCHEDULE #") //

#define CMD_TIME_SET						F("set_time") //set_time
#define REPLY_TIME_SET						F("Time:")

#define CMD_SLEEP_ENABLE					F("sleep_enable") //sleep_enable
#define REPLY_SLEEP_ENABLE					F("SLEEP mode ") //

#define CMD_SLEEP_OFF						F("sleep_off") //sleep_off
#define REPLY_SLEEP_OFF						F("Sleep OFF period") //

#define CMD_SLEEP_ON						F("sleep_on") //sleep_on
#define REPLY_SLEEP_ON						F("Sleep ON period") //

#define CMD_SMTP_ENABLE						F("smtp_enable") //smtp_enable
#define REPLY_SMTP_ENABLE					F("SMTP service") //

#define CMD_SMTP_LOGIN						F("smtp_login") //smtp_login
#define REPLY_SMTP_LOGIN					F("SMTP login") //

#define CMD_SMTP_PASS						F("smtp_pass") //smtp_pass
#define REPLY_SMTP_PASS						F("SMTP password") //

#define CMD_SMTP_PORT						F("smtp_port") //smtp_port
#define REPLY_SMTP_PORT						F("SMTP port") //

#define CMD_SMTP_SERVER						F("smtp_server") //smtp_server
#define REPLY_SMTP_SERVER					F("SMTP server IP address") //

#define CMD_SMTP_TO							F("smtp_to") //smtp_to
#define REPLY_SMTP_TO						F("SMTP TO address") //

#define CMD_STA_PASS						F("sta_pass") //sta_pass
#define REPLY_STA_PASS						F("WiFi STA password") //

#define CMD_STA_SSID						F("sta_ssid") //sta_ssid
#define REPLY_STA_SSID						F("WiFi STA SSID") //

#define CMD_TELEGRAM_ENABLE					F("telegram_enable") //telegram_enable
#define REPLY_TELEGRAM_ENABLE				F("Telegram service") //

#define CMD_TELEGRAM_TOKEN					F("telegram_token") //telegram_token
#define REPLY_TELEGRAM_TOKEN				F("TELEGRAM token") //

#define CMD_TELEGRAM_USER					F("telegram_user") //telegram_user
#define REPLY_TELEGRAM_USER					F("Telegram user #") //

#define CMD_TELNET_ENABLE					F("telnet_enable") //telnet_enable
#define REPLY_TELNET_ENABLE					F("Telnet service") //

#define CMD_TELNET_PORT						F("telnet_port") //telnet_port
#define REPLY_TELNET_PORT					F("Telnet port") //

#define CMD_WIFI_CONNECT_TIME				F("wifi_connect_time") //wifi_connect_time
#define REPLY_WIFI_CONNECT_TIME				F("WiFi connect time limit") //

#define CMD_WIFI_ENABLE						F("wifi_enable") //wifi_enable
#define REPLY_WIFI_ENABLE					F("WiFi service") //

#define CMD_WIFI_MODE						F("wifi_mode") //wifi_mode
#define REPLY_WIFI_MODE						F("WiFi mode") //

#define CMD_WIFI_POWER						F("wifi_power") //wifi_power
#define REPLY_WIFI_POWER					F("WiFi power") //

#define CMD_WIFI_RECONNECT_PERIOD			F("wifi_reconnect_period") //wifi_reconnect_period
#define REPLY_WIFI_RECONNECT_PERIOD			F("WiFi disconnect period") //

#define CMD_WIFI_STANDART					F("wifi_standart") //wifi_standart
#define REPLY_WIFI_STANDART					F("WiFi standart") //

#define REPLY_INCORRECT_VALUE				F("Incorrect value: ")
#define REPLY_INCORRECT						F("Incorrect ")

#define REPLY_ENABLED						F("enabled")
#define REPLY_DISABLED						F("disabled")
