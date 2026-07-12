#pragma once

// Core / generic device commands extracted from processCommand().
// These are not tied to any single optional transport feature.

bool tryHandleCoreCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_GET_CONFIG)
	{
		str += printConfig();
	}
	else if (cmd.command == CMD_TIME_SET)
	{
		uint8_t _hr = cmd.arguments[0].substring(11, 13).toInt();
		uint8_t _min = cmd.arguments[0].substring(14, 16).toInt();
		uint8_t _sec = cmd.arguments[0].substring(17, 19).toInt();
		uint8_t _day = cmd.arguments[0].substring(8, 10).toInt();
		uint8_t _month = cmd.arguments[0].substring(5, 7).toInt();
		uint16_t _yr = cmd.arguments[0].substring(0, 4).toInt();
		setTime(_hr, _min, _sec, _day, _month, _yr);
		str += REPLY_TIME_SET;
		str += eq;
		str += String(_yr);
		str += F(".");
		str += String(_month);
		str += F(".");
		str += String(_day);
		str += SPACE;
		str += String(_hr);
		str += COLON;
		str += String(_min);
		str += COLON;
		str += String(_sec);
		str += QUOTE;
		timeIsSet = true;
#ifdef NTP_TIME_ENABLE
		NTPenable = false;
#endif
	}
	else if (cmd.command == CMD_CHECK_PERIOD)
	{
		checkSensorPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
		str += REPLY_CHECK_PERIOD;
		str += eq;
		str += String(uint16_t(checkSensorPeriod / 1000UL));
		str += QUOTE;
		writeConfigString(SENSOR_READ_DELAY_addr, SENSOR_READ_DELAY_size, String(uint16_t(checkSensorPeriod / 1000UL)));
	}
	else if (cmd.command == CMD_DEVICE_NAME)
	{
		deviceName = cmd.arguments[0];
		str += REPLY_DEVICE_NAME;
		str += eq;
		str += deviceName;
		str += QUOTE;
		writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);
	}
	else if (cmd.command == CMD_RESET)
	{
		if (channel == CHANNEL_TELEGRAM)
		{
			uartCommand = F("reset");
			uartReady = true;
		}
		else
		{
#ifdef DEBUG_MODE
			//Serial.println(F("Resetting..."));
			//Serial.flush();
#endif
			ESP.restart();
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandleWifiCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_STA_SSID)
	{
		str += REPLY_STA_SSID;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(STA_SSID_addr, STA_SSID_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_STA_PASS)
	{
		str += REPLY_STA_PASS;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(STA_PASS_addr, STA_PASS_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_AP_SSID)
	{
		str += REPLY_AP_SSID;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(AP_SSID_addr, AP_SSID_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_AP_PASS)
	{
		str += REPLY_AP_PASS;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(AP_PASS_addr, AP_PASS_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_WIFI_STANDART)
	{
		str += REPLY_WIFI_STANDART;
		str += eq;
		if (cmd.arguments[0] == F("B") || cmd.arguments[0] == F("G") || cmd.arguments[0] == F("N"))
		{
			str += cmd.arguments[0];
			str += QUOTE;
			writeConfigString(WIFI_STANDART_addr, WIFI_STANDART_size, cmd.arguments[0]);
		}
		else
		{
			str += REPLY_INCORRECT;
			str += cmd.arguments[0];
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_WIFI_POWER)
	{
		str += REPLY_WIFI_POWER;
		str += eq;
		float power = cmd.arguments[0].toFloat();
		if (power >= 0 && power <= 20.5)
		{
			str += String(power);
			writeConfigFloat(WIFI_POWER_addr, power);
		}
		else
		{
			str += REPLY_INCORRECT;
			str += cmd.arguments[0];
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_WIFI_MODE)
	{
		cmd.arguments[0].toUpperCase();
		uint8_t wifi_mode = 255;
		str += REPLY_WIFI_MODE;
		str += eq;
		if (cmd.arguments[0] == wifiModes[WIFI_OFF])
		{
			wifi_mode = WIFI_OFF;
		}
		else if (cmd.arguments[0] == wifiModes[WIFI_STA])
		{
			wifi_mode = WIFI_STA;
		}
		else if (cmd.arguments[0] == wifiModes[WIFI_AP])
		{
			wifi_mode = WIFI_AP;
		}
		else if (cmd.arguments[0] == wifiModes[WIFI_AP_STA])
		{
			wifi_mode = WIFI_AP_STA;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
		if (wifi_mode != 255)
		{
			str += wifiModes[wifi_mode];
			writeConfigString(WIFI_MODE_addr, WIFI_MODE_size, String(wifi_mode));
		}
		else
		{
			str += REPLY_INCORRECT;
			str += cmd.arguments[0];
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_WIFI_ENABLE)
	{
		str += REPLY_WIFI_ENABLE;
		str += eq;
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			wifiEnable = false;
			Start_OFF_Mode();
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			wifiEnable = true;
			startWiFi();
			str += SWITCH_ON;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
		str += QUOTE;
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandlePinCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_AUTOREPORT)
	{
		autoReport = 0;
		str += REPLY_AUTOREPORT;
		str += eq;
		for (uint8_t i = 0; i < cmd.tokensNumber; i++)
		{
			for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
			{
				if (cmd.arguments[i].indexOf(channels[b]) >= 0)
				{
					bitSet(autoReport, b);
					str += channels[b];
					str += F(",");
				}
				yield();
			}
		}
		writeConfigString(AUTOREPORT_addr, AUTOREPORT_size, String(autoReport));
		str += QUOTE;
	}
	else if (cmd.command == CMD_PIN_MODE_SET)
	{
		str += REPLY_PIN_MODE_SET;
		str += eq;
		if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
		{
			cmd.arguments[0].toUpperCase();
			uint8_t intModeNum = 255;
			if (cmd.arguments[0] == pinModeList[INPUT])
				intModeNum = INPUT;
			else if (cmd.arguments[0] == pinModeList[OUTPUT])
				intModeNum = OUTPUT;
			else if (cmd.arguments[0] == pinModeList[INPUT_PULLUP])
				intModeNum = INPUT_PULLUP;
			else if (cmd.arguments[0] == pinModeList[3])
				intModeNum = 3; //off
			else
			{
				str += REPLY_INCORRECT;
				str += F("pin mode: ");
				str += cmd.arguments[0];
			}

			if (intModeNum != 255)
			{
				uint16_t l = PIN_MODE_size / PIN_NUMBER;
				writeConfigString(PIN_MODE_addr + (cmd.index - 1) * l, l, String(intModeNum));

				str += String(cmd.index);
				str += F(" mode: ");
				str += pinModeList[intModeNum];
			}
		}
		else
		{
			str += REPLY_INCORRECT;
			str += F("pin: ");
			str += String(cmd.index);
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_INTERRUPT_MODE_SET)
	{
		str += REPLY_INTERRUPT_MODE_SET;
		str += eq;
		if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
		{
			cmd.arguments[0].toUpperCase();
			uint8_t intModeNum = 255;
			if (cmd.arguments[0] == intModeList[0])
				intModeNum = 0;
			else if (cmd.arguments[0] == intModeList[RISING])
				intModeNum = RISING;
			else if (cmd.arguments[0] == intModeList[FALLING])
				intModeNum = FALLING;
			else if (cmd.arguments[0] == intModeList[CHANGE])
				intModeNum = CHANGE;
			else
			{
				str += REPLY_INCORRECT;
				str += F("INT mode: ");
				str += cmd.arguments[0];
			}
			if (intModeNum != 255)
			{
				uint16_t l = INTERRUPT_MODE_size / PIN_NUMBER;
				writeConfigString(INTERRUPT_MODE_addr + (cmd.index - 1) * l, l, String(intModeNum));
				str += String(cmd.index);
				str += F(" mode: ");
				str += intModeList[intModeNum];
			}
		}
		else
		{
			str += REPLY_INCORRECT;
			str += F("INT: ");
			str += String(cmd.index);
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_OUTPUT_INIT_SET)
	{
		str += REPLY_OUTPUT_INIT_SET;
		str += eq;
		if (cmd.index >= 1 && cmd.index <= PIN_NUMBER && !bitRead(SIGNAL_PINS, cmd.index - 1))
		{
			if (cmd.arguments[0] != SWITCH_ON && cmd.arguments[0] != SWITCH_OFF)
			{
				uint16_t outState = 0;
				outState = cmd.arguments[0].toInt();
				cmd.arguments[0] = String(outState);
			}
			uint16_t l = OUTPUT_INIT_size / PIN_NUMBER;
			writeConfigString(OUTPUT_INIT_addr + (cmd.index - 1) * l, l, cmd.arguments[0]);
			str += String(cmd.index);
			str += F(" initial state: ");
			str += cmd.arguments[0];
		}
		else
		{
			str += REPLY_INCORRECT;
			str += F("OUT");
			str += String(cmd.index);
		}
		str += QUOTE;
	}
	else if (cmd.command == CMD_OUTPUT_SET)
	{
		str += set_output(cmd.index, cmd.arguments[0]);
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandleSensorOffsetCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_TEMERATURE_OFFSET)
	{
		str += REPLY_TEMERATURE_OFFSET;
		str += eq;
		float t = cmd.arguments[0].toFloat();
		str += String(t);
		writeConfigString(TEMPERATURE_OFFSET_addr, TEMPERATURE_OFFSET_size, String(t));
		str += QUOTE;
	}
	else if (cmd.command == CMD_HUMIDITY_OFFSET)
	{
		str += REPLY_HUMIDITY_OFFSET;
		str += eq;
		int t = cmd.arguments[0].toInt();
		str += String(t);
		writeConfigString(HUMIDITY_OFFSET_addr, HUMIDITY_OFFSET_size, String(t));
		str += QUOTE;
	}
	else if (cmd.command == CMD_CO2_OFFSET)
	{
		str += REPLY_CO2_OFFSET;
		str += eq;
		int t = cmd.arguments[0].toInt();
		str += String(t);
		writeConfigString(CO2_OFFSET_addr, CO2_OFFSET_size, String(t));
		str += QUOTE;
	}
	else
	{
		return false;
	}
	return true;
}

#ifdef BUZZER_ENABLE
bool tryHandleBuzzerCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_BUZZ)
	{
		int f = uint32_t(cmd.arguments[0].toInt());
		long d = cmd.arguments[1].toInt();
		tone(BUZZER_ENABLE, f, d);
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef SLEEP_ENABLE
bool tryHandleSleepCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_SLEEP_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			//sleepEnable = false;
			writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_OFF_NUMBER);
			str += REPLY_SLEEP_ENABLE;
			str += eq;
			str += REPLY_DISABLED;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			//sleepEnable = true;
			writeConfigString(SLEEP_ENABLE_addr, SLEEP_ENABLE_size, SWITCH_ON_NUMBER);
			str += REPLY_SLEEP_ENABLE;
			str += eq;
			str += REPLY_ENABLED;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
	}
	else if (cmd.command == CMD_SLEEP_ON)
	{
		activeTimeOut_ms = uint32_t(cmd.arguments[0].toInt() * 1000UL);
		if (activeTimeOut_ms == 0)
			activeTimeOut_ms = uint32_t(2 * connectTimeLimit);

		str += REPLY_SLEEP_ON;
		str += eq;
		str += String(uint16_t(activeTimeOut_ms / 1000UL));
		str += QUOTE;
		writeConfigString(SLEEP_ON_addr, SLEEP_ON_size, String(uint16_t(activeTimeOut_ms / 1000UL)));
	}
	else if (cmd.command == CMD_SLEEP_OFF)
	{
		sleepTimeOut_us = uint32_t(cmd.arguments[0].toInt() * 1000000UL);
		if (sleepTimeOut_us == 0)
			sleepTimeOut_us = uint32_t(20UL * connectTimeLimit * 1000UL); //max 71 minutes (Uint32_t / 1000 / 1000 /60)

		str += REPLY_SLEEP_OFF;
		str += eq;
		str += String(uint16_t(sleepTimeOut_us / 1000000UL));
		str += QUOTE;
		writeConfigString(SLEEP_OFF_addr, SLEEP_OFF_size, String(uint16_t(sleepTimeOut_us / 1000000UL)));
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef EVENTS_ENABLE
bool tryHandleEventCommand(commandTokens& cmd, String& command, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_EVENT_SET)
	{
		if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
		{
			int t = command.indexOf('=');
			command = command.substring(t + 1);
			uint16_t m = EVENTS_TABLE_size / EVENTS_NUMBER;
			writeConfigString(EVENTS_TABLE_addr + cmd.index * m, m, command);
			str += REPLY_EVENT_SET;
			str += String(cmd.index);
			str += F(" is now: ");
			str += cmd.arguments[0];
		}
		else
		{
			str += F("Event #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(EVENTS_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_EVENTS_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
			eventsEnable = false;
			str += REPLY_EVENTS_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
			eventsEnable = true;
			str += REPLY_EVENTS_ENABLE;
			str += eq;
			str += SWITCH_ON;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
	}
	else if (cmd.command == CMD_EVENT_FLAG_SET)
	{
		if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
				eventsFlags[cmd.index] = false;
				str += REPLY_EVENT_FLAG_SET;
				str += String(cmd.index);
				str += F(" cleared");
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
				eventsFlags[cmd.index] = true;
				str += REPLY_EVENT_FLAG_SET;
				str += String(cmd.index);
				str += F(" set");
			}
		}
		else
		{
			str += F("Event #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(EVENTS_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_HELP_EVENT)
	{
		str += printHelpEvent();
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef SCHEDULER_ENABLE
bool tryHandleScheduleCommand(commandTokens& cmd, String& command, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_SCHEDULE_SET)
	{
		if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
		{
			int t = command.indexOf('=');
			command = command.substring(t + 1);
			uint16_t m = SCHEDULER_TABLE_size / SCHEDULES_NUMBER;
			writeConfigString(SCHEDULER_TABLE_addr + cmd.index * m, m, command);
			writeScheduleExecTime(cmd.index, 0);
			str += REPLY_SCHEDULE_SET;
			str += String(cmd.index);
			str += F(" is now: ");
			str += cmd.arguments[0];
		}
		else
		{
			str += F("Schedule #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(SCHEDULES_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_SCHEDULER_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_OFF_NUMBER);
			schedulerEnable = false;
			str += REPLY_SCHEDULER_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(SCHEDULER_ENABLE_addr, SCHEDULER_ENABLE_size, SWITCH_ON_NUMBER);
			schedulerEnable = true;
			str += REPLY_SCHEDULER_ENABLE;
			str += eq;
			str += SWITCH_ON;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
	}
	else if (cmd.command == CMD_CLEAR_SCHEDULE_EXEC_TIME)
	{
		if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
		{
			writeScheduleExecTime(cmd.index, 0);
			str += REPLY_CLEAR_SCHEDULE_EXEC_TIME;
			str += String(cmd.index);
			str += F(" reset");
		}
		else
		{
			str += F("Schedule #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(SCHEDULES_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_HELP_SCHEDULE)
	{
		str += printHelpSchedule();
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#if defined(EVENTS_ENABLE) || defined(SCHEDULER_ENABLE)
bool tryHandleHelpActionCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_HELP_ACTION)
	{
		str += printHelpAction();
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef DISPLAY_ENABLED
bool tryHandleDisplayCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_DISPLAY_REFRESH)
	{
		displaySwitchPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
		str += REPLY_DISPLAY_REFRESH;
		str += eq;
		str += String(uint16_t(displaySwitchPeriod));
		str += QUOTE;
		writeConfigString(DISPLAY_REFRESH_addr, DISPLAY_REFRESH_size, String(uint16_t(displaySwitchPeriod / 1000UL)));
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef OTA_UPDATE
bool tryHandleOtaCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_OTA_PORT)
	{
		uint16_t ota_port = cmd.arguments[0].toInt();
		str += REPLY_OTA_PORT;
		str += eq;
		str += String(ota_port);
		str += QUOTE;
		writeConfigString(OTA_PORT_addr, OTA_PORT_size, String(ota_port));
	}
	else if (cmd.command == CMD_OTA_PASSWORD)
	{
		str += REPLY_OTA_PASSWORD;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_OTA_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_OFF_NUMBER);
			otaEnable = false;
			str += REPLY_OTA_ENABLE;
			str += eq;
			str += SWITCH_OFF_NUMBER;
			ArduinoOTA.begin();
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_ON_NUMBER);
			otaEnable = true;
			str += REPLY_OTA_ENABLE;
			str += eq;
			str += SWITCH_ON;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif
