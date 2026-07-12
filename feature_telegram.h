#pragma once

// TELEGRAM
#ifdef TELEGRAM_ENABLE
#include <ArduinoJson.h>
#include <CTBot.h>
//#include <ArduinoJson.hpp>
//#include <CTBotDataStructures.h>
//#include <CTBotDefines.h>
//#include <CTBotInlineKeyboard.h>
//#include <CTBotReplyKeyboard.h>
//#include <CTBotSecureConnection.h>
//#include <CTBotStatusPin.h>
//#include <CTBotWifiSetup.h>

CTBot myBot;
telegramMessage telegramOutboundBuffer[TELEGRAM_MESSAGE_BUFFER_SIZE];
uint8_t telegramOutboundBufferPos = 0;
uint32_t telegramLastTime = 0;
bool telegramEnable = false;

String uint64ToString(uint64_t input)
{
	String result = "";
	uint8_t base = 10;
	do
	{
		char c = input % base;
		input /= base;
		if (c < 10)
			c += '0';
		else
			c += 'A' - 10;
		result = c + result;
		yield();
	} while (input);
	return result;
}

uint64_t StringToUint64(String& value)
{
	for (uint8_t i = 0; i < value.length(); i++)
	{
		if (value[i] < '0' || value[i] > '9')
			return 0;
	}
	uint64_t result = 0;
	const char* p = value.c_str();
	const char* q = p + value.length();
	while (p < q)
	{
		result = (result << 1) + (result << 3) + *(p++) - '0';
		yield();
	}
	return result;
}

bool sendToTelegramServer(int64_t user, String& message)
{
	bool result = false;
	if (telegramEnable && WiFi.status() == WL_CONNECTED)
	{
		result = myBot.testConnection();
		if (result)
		{
			result = myBot.sendMessage(user, message);
			if (result)
			{
				result = true;
			}
		}
	}
	return result;
}

void addMessageToTelegramOutboundBuffer(int64_t user, String message, uint8_t retries)
{
	if (telegramOutboundBufferPos >= TELEGRAM_MESSAGE_BUFFER_SIZE)
	{
		for (uint8_t i = 0; i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
			yield();
		}
		telegramOutboundBufferPos = TELEGRAM_MESSAGE_BUFFER_SIZE - 1;
	}
	telegramOutboundBuffer[telegramOutboundBufferPos].user = user;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = message;
	telegramOutboundBuffer[telegramOutboundBufferPos].retries = retries;
	telegramOutboundBufferPos++;
}

void removeMessageFromTelegramOutboundBuffer()
{
	if (telegramOutboundBufferPos <= 0)
		return;
	for (uint8_t i = 0; i < telegramOutboundBufferPos; i++)
	{
		if (i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1)
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		else
		{
			telegramOutboundBuffer[i].message.clear();
			telegramOutboundBuffer[i].user = 0;
		}
		yield();
	}
	telegramOutboundBufferPos--;
}

void sendToTelegram(int64_t user, String& message, uint8_t retries)
{
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= TELEGRAM_MESSAGE_MAX_SIZE)
		{
			addMessageToTelegramOutboundBuffer(user, message.substring(0, TELEGRAM_MESSAGE_MAX_SIZE), retries);
			message = message.substring(TELEGRAM_MESSAGE_MAX_SIZE);
		}
		else
		{
			addMessageToTelegramOutboundBuffer(user, message, retries);
			message.clear();
		}
		yield();
	}
}

void sendBufferToTelegram()
{
	if (telegramEnable && WiFi.status() == WL_CONNECTED && myBot.testConnection())
	{
		bool isRegisteredUser = false;
		bool isAdmin = false;
		TBMessage msg;
		while (CTBotMessageText == myBot.getNewMessage(msg))
		{
			yield();
			//consider 1st sender an admin if admin is empty
			if (getTelegramUser(0) != 0)
			{
				//check if it's registered user
				for (uint8_t i = 0; i < TELEGRAM_USERS_NUMBER; i++)
				{
					yield();
					if (getTelegramUser(i) == msg.sender.id)
					{
						isRegisteredUser = true;
						if (i == 0)
							isAdmin = true;
						break;
					}
				}
			}
			yield();
			//process data from TELEGRAM
			if (isRegisteredUser && msg.text.length() > 0)
			{
				//sendToTelegram(msg.sender.id, deviceName + ": " + msg.text, telegramRetries);
				String tmpCmd = msg.text;
				String str = processCommand(tmpCmd, CHANNEL_TELEGRAM, isAdmin);
				String message = deviceName;
				message += F(": ");
				message += str;
				sendToTelegram(msg.sender.id, message, TELEGRAM_RETRIES);
			}
		}

		if (telegramOutboundBuffer[0].message.length() != 0)
		{
			if (sendToTelegramServer(telegramOutboundBuffer[0].user, telegramOutboundBuffer[0].message))
			{
				removeMessageFromTelegramOutboundBuffer();
			}
			else
			{
				telegramOutboundBuffer[0].retries--;
				if (telegramOutboundBuffer[0].retries <= 0)
				{
					removeMessageFromTelegramOutboundBuffer();
				}
			}
		}
	}
}

uint64_t getTelegramUser(uint8_t n)
{
	uint16_t singleUserLength = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
	String userStr = readConfigString((TELEGRAM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return StringToUint64(userStr);
}

bool tryHandleTelegramCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	// send_telegram=[*/user#],message
	if (cmd.command == CMD_SEND_TELEGRAM)
	{
		uint8_t i = 0;
		uint8_t j = 0;
		if (cmd.arguments[0] == STAR) //if * then all
		{
			i = 0;
			j = TELEGRAM_USERS_NUMBER;
		}
		else
		{
			i = cmd.arguments[0].toInt();
			j = i + 1;
		}

		if (i <= TELEGRAM_USERS_NUMBER)
		{
			for (; i < j; i++)
			{
				int64_t gsmUser = getTelegramUser(i);
				if (gsmUser != 0)
				{
					if (sendToTelegramServer(gsmUser, cmd.arguments[1]))
					{
						str += F("Message sent");
					}
					else
					{
						str += F("Message not sent");
					}
				}
				yield();
			}
		}
	}
	else if (cmd.command == CMD_TELEGRAM_USER)
	{
		uint64_t newUser = StringToUint64(cmd.arguments[0]);
		if (cmd.index >= 0 && cmd.index < TELEGRAM_USERS_NUMBER)
		{
			uint16_t m = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
			writeConfigString(TELEGRAM_USERS_TABLE_addr + cmd.index * m, m, uint64ToString(newUser));
			str += REPLY_TELEGRAM_USER;
			str += uint64ToString(cmd.index);
			str += F(" is now: ");
			str += uint64ToString(newUser);
		}
		else
		{
			str += F("Telegram user #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(TELEGRAM_USERS_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_TELEGRAM_TOKEN)
	{
		str += REPLY_TELEGRAM_TOKEN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, cmd.arguments[0]);
		//myBot.setTelegramToken(telegramToken);
	}
	else if (cmd.command == CMD_TELEGRAM_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_OFF_NUMBER);
			//telegramEnable = false;
			str += REPLY_TELEGRAM_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_ON_NUMBER);
			//telegramEnable = true;
			//myBot.wifiConnect(STA_Ssid, STA_Password);
			//myBot.setTelegramToken(telegramToken);
			//myBot.testConnection();
			str += REPLY_TELEGRAM_ENABLE;
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

bool tryHandleTelegramAction(commandTokens& cmd, String& tmpAction)
{
	//send_telegram=* / user#,message
	if (telegramEnable && cmd.command == ACTION_SEND_TELEGRAM)
	{
		uint8_t i = 0;
		uint8_t j = 0;
		if (cmd.arguments[0] == STAR) //if * then all
		{
			i = 0;
			j = TELEGRAM_USERS_NUMBER;
		}
		else
		{
			i = cmd.arguments[0].toInt();
			j = i + 1;
		}
		sensorDataCollection sensorData = collectData();
		for (; i < j; i++)
		{
			uint64_t user = getTelegramUser(i);
			if (user > 0)
			{
				String tmpStr = deviceName;
				tmpStr += COLON;
				tmpStr += cmd.arguments[1];
				tmpStr += EOL;
				tmpStr += parseSensorReport(sensorData, EOL);
				bool sendOk = sendToTelegramServer(user, tmpStr);
				if (!sendOk)
				{
					/*#ifdef EVENTS_ENABLE
											eventsFlags[eventNum] = false;
					#endif*/
				}
			}
			yield();
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif
