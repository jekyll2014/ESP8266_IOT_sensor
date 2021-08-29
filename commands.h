#pragma once
#define CMD_GET_SENSOR									F("get_sensor") //get_sensor
#define CMD_GET_STATUS									F("get_status") //get_status
#define CMD_GET_CONFIG									F("get_config") //get_config
#define CMD_HELP												F("help") //help
#define CMD_HELP_EVENT									F("help_event") //help_event
#define CMD_HELP_SCHEDULE								F("help_schedule") //help_schedule
#define CMD_HELP_ACTION									F("help_action") //help_action
#define CMD_SET_TIME										F("set_time") //set_time
#define CMD_CHECK_PERIOD								F("check_period") //check_period
#define CMD_LOG_PERIOD									F("log_period") //log_period
#define CMD_GETLOG											F("getlog") //getlog
#define CMD_DEVICE_NAME									F("device_name") //device_name
#define CMD_STA_SSID										F("sta_ssid") //sta_ssid
#define CMD_STA_PASS										F("sta_pass") //sta_pass
#define CMD_AP_SSID											F("ap_ssid") //ap_ssid
#define CMD_AP_PASS											F("ap_pass") //ap_pass
#define CMD_WIFI_STANDART								F("wifi_standart") //wifi_standart
#define CMD_WIFI_POWER									F("wifi_power") //wifi_power
#define CMD_WIFI_MODE										F("wifi_mode") //wifi_mode
#define CMD_WIFI_CONNECT_TIME						F("wifi_connect_time") //wifi_connect_time
#define CMD_WIFI_RECONNECT_PERIOD				F("wifi_reconnect_period") //wifi_reconnect_period
#define CMD_WIFI_ENABLE									F("wifi_enable") //wifi_enable
#define CMD_TELNET_PORT									F("telnet_port") //telnet_port
#define CMD_TELNET_ENABLE								F("telnet_enable") //telnet_enable
#define CMD_RESET												F("reset") //reset
#define CMD_AUTOREPORT									F("autoreport") //autoreport
#define CMD_SET_PIN_MODE								F("set_pin_mode") //set_pin_mode
#define CMD_SET_INTERRUPT_MODE					F("set_interrupt_mode") //set_interrupt_mode
#define CMD_SET_INIT_OUTPUT							F("set_init_output") //set_init_output
#define CMD_SET_OUTPUT									F("set_output") //set_output
#define CMD_BUZZ												F("buzz") //buzz
#define CMD_SLEEP_ENABLE								F("sleep_enable") //sleep_enable
#define CMD_SLEEP_ON										F("sleep_on") //sleep_on
#define CMD_SLEEP_OFF										F("sleep_off") //sleep_off
#define CMD_HTTP_PORT										F("http_port") //http_port
#define CMD_HTTP_ENABLE									F("http_enable") //http_enable
#define CMD_SEND_MQTT										F("send_mqtt") //send_mqtt
#define CMD_MQTT_SERVER									F("mqtt_server") //mqtt_server
#define CMD_MQTT_PORT										F("mqtt_port") //mqtt_port
#define CMD_MQTT_LOGIN									F("mqtt_login") //mqtt_login
#define CMD_MQTT_PASS										F("mqtt_pass") //mqtt_pass
#define CMD_MQTT_ID											F("mqtt_id") //mqtt_id
#define CMD_MQTT_TOPIC_IN								F("mqtt_topic_in") //mqtt_topic_in
#define CMD_MQTT_TOPIC_OUT							F("mqtt_topic_out") //mqtt_topic_out
#define CMD_MQTT_CLEAN									F("mqtt_clean") //mqtt_clean
#define CMD_MQTT_ENABLE									F("mqtt_enable") //mqtt_enable
#define CMD_SEND_MAIL										F("send_mail") //send_mail=address,subject,message
#define CMD_SMTP_SERVER									F("smtp_server") //smtp_server
#define CMD_SMTP_PORT										F("smtp_port") //smtp_port
#define CMD_SMTP_LOGIN									F("smtp_login") //smtp_login
#define CMD_SMTP_PASS										F("smtp_pass") //smtp_pass
#define CMD_SMTP_TO											F("smtp_to") //smtp_to
#define CMD_SMTP_ENABLE									F("smtp_enable") //smtp_enable
#define CMD_SEND_SMS										F("send_sms") //send_sms
#define CMD_GSM_USER										F("gsm_user") //gsm_user
#define CMD_GSM_ENABLE									F("gsm_enable") //gsm_enable
#define CMD_SEND_TELEGRAM								F("send_telegram") //send_telegram
#define CMD_TELEGRAM_USER								F("telegram_user") //telegram_user
#define CMD_TELEGRAM_TOKEN							F("telegram_token") //telegram_token
#define CMD_TELEGRAM_ENABLE							F("telegram_enable") //telegram_enable
#define CMD_GSCRIPT_TOKEN								F("gscript_token") //gscript_token
#define CMD_GSCRIPT_ENABLE							F("gscript_enable") //gscript_enable
#define CMD_PUSHINGBOX_TOKEN						F("pushingbox_token") //pushingbox_token
#define CMD_PUSHINGBOX_PARAMETER				F("pushingbox_parameter") //pushingbox_parameter
#define CMD_PUSHINGBOX_ENABLE						F("pushingbox_enable") //pushingbox_enable
#define CMD_NTP_SERVER									F("ntp_server") //ntp_server
#define CMD_NTP_TIME_ZONE								F("ntp_time_zone") //ntp_time_zone
#define CMD_NTP_REFRESH_DELAY						F("ntp_refresh_delay") //ntp_refresh_delay
#define CMD_NTP_REFRESH_PERIOD					F("ntp_refresh_period") //ntp_refresh_period
#define CMD_NTP_ENABLE									F("ntp_enable") //ntp_enable
#define CMD_SET_EVENT										F("set_event") //set_event
#define CMD_EVENTS_ENABLE								F("events_enable") //events_enable
#define CMD_SET_EVENT_FLAG							F("set_event_flag") //set_event_flag
#define CMD_SET_SCHEDULE								F("set_schedule") //set_schedule
#define CMD_SCHEDULER_ENABLE						F("scheduler_enable") //scheduler_enable
#define CMD_CLEAR_SCHEDULE_EXEC_TIME		F("clear_schedule_exec_time") //clear_schedule_exec_time
#define CMD_DISPLAY_REFRESH							F("display_refresh") //display_refresh

#define REPLY_INCORRECT_VALUE						F("Incorrect value: ")
#define REPLY_INCORRECT									F("Incorrect ")

#define REPLY_ENABLED										F("enabled")
#define REPLY_DISABLED									F("disabled")

#define REPLY_SET_TIME									F("New time:")
#define REPLY_CHECK_PERIOD							F("New sensor check period = \"")
#define REPLY_LOG_PERIOD								F("New log save period = \"") //
#define REPLY_DEVICE_NAME								F("New device name = \"") //
#define REPLY_STA_SSID									F("New STA SSID = \"") //
#define REPLY_STA_PASS									F("New STA password = \"") //
#define REPLY_AP_SSID										F("New AP SSID = \"") //
#define REPLY_AP_PASS										F("New AP password = \"") //
#define REPLY_WIFI_STANDART							F("New WiFi standart  = \"") //
#define REPLY_WIFI_POWER								F("New WiFi power  = \"") //
#define REPLY_WIFI_MODE									F("New WiFi mode  = \"") //
#define REPLY_WIFI_CONNECT_TIME					F("New WiFi connect time limit = \"") //
#define REPLY_WIFI_RECONNECT_PERIOD			F("New WiFi disconnect period = \"") //
#define REPLY_WIFI_ENABLE								F("WiFi ") //
#define REPLY_TELNET_PORT								F("New telnet port = \"") //
#define REPLY_TELNET_ENABLE							F("Telnet ") //
#define REPLY_AUTOREPORT								F("New autoreport channels=") //
#define REPLY_SET_PIN_MODE							F("New pin") //
#define REPLY_SET_INTERRUPT_MODE				F("New INT") //
#define REPLY_SET_INIT_OUTPUT						F("New OUT") //
#define REPLY_SET_OUTPUT								F("Output") //
#define REPLY_SLEEP_ENABLE							F("SLEEP mode ") //
#define REPLY_SLEEP_ON									F("New sleep ON period = \"") //
#define REPLY_SLEEP_OFF									F("New sleep OFF period = \"") //
#define REPLY_HTTP_PORT									F("New HTTP port = \"") //
#define REPLY_HTTP_ENABLE								F("HTTP server ") //
#define REPLY_MQTT_SERVER								F("New MQTT server = \"") //
#define REPLY_MQTT_PORT									F("New MQTT port = \"") //
#define REPLY_MQTT_LOGIN								F("New MQTT login = \"") //
#define REPLY_MQTT_PASS									F("New MQTT password = \"") //
#define REPLY_MQTT_ID										F("New MQTT ID = \"") //
#define REPLY_MQTT_TOPIC_IN							F("New MQTT SUBSCRIBE topic = \"") //
#define REPLY_MQTT_TOPIC_OUT						F("New MQTT POST topic = \"") //
#define REPLY_MQTT_CLEAN								F("New MQTT clean session: ") //
#define REPLY_MQTT_ENABLE								F("MQTT ") //
#define REPLY_SMTP_SERVER								F("New SMTP server IP address = \"") //
#define REPLY_SMTP_PORT									F("New SMTP port = \"") //
#define REPLY_SMTP_LOGIN								F("New SMTP login = \"") //
#define REPLY_SMTP_PASS									F("New SMTP password = \"") //
#define REPLY_SMTP_TO										F("New SMTP TO address = \"") //
#define REPLY_SMTP_ENABLE								F("SMTP reporting ") //
#define REPLY_GSM_USER									F("New GSM user #") //
#define REPLY_GSM_ENABLE								F("GSM ") //
#define REPLY_TELEGRAM_USER							F("New Telegram user #") //
#define REPLY_TELEGRAM_TOKEN						F("New TELEGRAM token = \"") //
#define REPLY_TELEGRAM_ENABLE						F("Telegram ") //
#define REPLY_GSCRIPT_TOKEN							F("New GScript token = \"") //
#define REPLY_GSCRIPT_ENABLE						F("GScript ") //
#define REPLY_PUSHINGBOX_TOKEN					F("New PushingBox token = \"") //
#define REPLY_PUSHINGBOX_PARAMETER			F("New PUSHINGBOX parameter name = \"") //
#define REPLY_PUSHINGBOX_ENABLE					F("PushingBox ") //
#define REPLY_NTP_SERVER								F("New NTP server = \"") //
#define REPLY_NTP_TIME_ZONE							F("New NTP time zone = \"") //
#define REPLY_NTP_REFRESH_DELAY					F("New NTP refresh delay = \"") //
#define REPLY_NTP_REFRESH_PERIOD				F("New NTP refresh period = \"") //
#define REPLY_NTP_ENABLE								F("NTP ") //
#define REPLY_SET_EVENT									F("New event #") //
#define REPLY_EVENTS_ENABLE							F("Events ") //
#define REPLY_SET_EVENT_FLAG						F("Event #") //
#define REPLY_SET_SCHEDULE							F("New schedule #") //
#define REPLY_SCHEDULER_ENABLE					F("Scheduler ") //
#define REPLY_CLEAR_SCHEDULE_EXEC_TIME	F("Schedule #") //
#define REPLY_DISPLAY_REFRESH						F("New display refresh period = \"") //
