#pragma once
#define ACTION_COMMAND									F("command") //command=****
#define ACTION_SET_OUTPUT								F("set_output") //set_output?=x
#define ACTION_RESET_COUNTER						F("reset_counter") //reset_counter? - deprecated
#define ACTION_SET_COUNTER							F("set_counter") //set_counter?=x
#define ACTION_RESET_FLAG								F("reset_flag") //reset_flag?
#define ACTION_SET_FLAG									F("set_flag") //set_flag?
#define ACTION_SET_EVENT_FLAG						F("set_event_flag") //set_event_flag?=0/1/off/on
#define ACTION_CLEAR_SCHEDULE_EXEC_TIME	F("clear_schedule_exec_time") //clear_schedule_exec_time?
#define ACTION_SEND_SMS									F("send_sms") //send_sms=*/user#,message
#define ACTION_SEND_TELEGRAM						F("send_telegram") //send_telegram=* / user#,message
#define ACTION_SEND_PUSHINGBOX					F("send_pushingbox") //send_PushingBox=message
#define ACTION_SEND_MAIL								F("send_mail") //send_mail=address_to,subject,message
#define ACTION_SEND_GSCRIPT							F("send_gscript") //send_GScript=message
#define ACTION_SEND_MQTT								F("send_mqtt") //send_MQTT=topic_to,message
#define ACTION_SAVE_LOG									F("save_log") //save_log
