#pragma once

// Log history
#ifdef LOG_ENABLE
// Log settings for ~6.5 hours of storage at the default 10-minute sample period.
// ~125 bytes/record with this project's current sensor set (AMS2320+HTU21D+BME280+BMP180+AHTx0+DS18B20)
// -> 40 * 125B =~ 5000B. Raise only with a matching check of free heap; this buffer is permanent, not lazy.
#define LOG_SIZE 40

sensorDataCollection history_log[LOG_SIZE];
uint16_t history_record_number = 0;
uint32_t logLastTime = 0;
//uint32_t logPeriod = 60 / (LOG_SIZE / 32) * 60 * 1000 / 5; // 60minutes/hour / ( 192pcs / 32hours ) * 60seconds/minute * 1000 msec/second
uint32_t logPeriod = 600000UL;

void addToLog(sensorDataCollection& record)
{
	history_log[history_record_number] = record;
	history_record_number++;

	//if log is full - roll back to 0
	if (history_record_number >= LOG_SIZE)
	{
		history_record_number = 0;
	}
}

bool tryHandleLogCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_LOG_PERIOD)
	{
		logPeriod = uint32_t(cmd.arguments[0].toInt() * 1000UL);
		str += REPLY_LOG_PERIOD;
		str += eq;
		str += String(uint16_t(logPeriod / 1000UL));
		str += QUOTE;
		writeConfigString(LOG_PERIOD_addr, LOG_PERIOD_size, String(uint16_t(logPeriod / 1000UL)));
	}
	//getLog - to be implemented
	else if (cmd.command == CMD_GETLOG)
	{
		for (int i = 0; i < LOG_SIZE; i++)
		{
			if (history_log[i].year > 0)
			{
				str += parseSensorReport(history_log[i], EOL, true);
				switch (channel)
				{
#ifdef DEBUG_MODE
				case CHANNEL_UART:
					Serial.println(str);
					break;
#endif
				case CHANNEL_TELNET:

					break;
#ifdef MQTT_ENABLE
				case CHANNEL_MQTT:

					break;
#endif
#ifdef TELEGRAM_ENABLE
				case CHANNEL_TELEGRAM:

					break;
#endif
#ifdef GSCRIPT_ENABLE
				case CHANNEL_GSCRIPT:

					break;
#endif
#ifdef PUSHINGBOX_ENABLE
				case CHANNEL_PUSHINGBOX:

					break;
#endif
#ifdef SMTP_ENABLE
				case CHANNEL_SMTP:

					break;
#endif
#ifdef GSM_ENABLE
				case CHANNEL_GSM:

					break;
#endif
				default:
					break;
				};
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandleLogAction(commandTokens& cmd, String& tmpAction)
{
	//save_log
	if (cmd.command == ACTION_SAVE_LOG)
	{
		sensorDataCollection sensorData = collectData();
		addToLog(sensorData);
	}
	else
	{
		return false;
	}
	return true;
}
#endif
