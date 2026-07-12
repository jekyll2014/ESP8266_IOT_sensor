#pragma once

// GSM
#ifdef GSM_ENABLE
#include <SoftwareSerial.h>

uint32_t gsmTimeOut = 0;
bool gsmEnable = false;

// NOTE (pre-existing, not introduced by this file split): another global
// `SoftwareSerial uart2(...)` is declared under `#ifdef MH_Z19_UART_ENABLE`
// in ESP8266_IOT_sensor.ino. Both are gated by mutually-exclusive-in-practice
// but not mutually-exclusive-in-code #ifdefs, so enabling GSM_ENABLE and
// MH_Z19_UART_ENABLE at the same time would cause a duplicate-symbol build
// error. Left as-is per instructions; not fixed here.
SoftwareSerial uart2(SOFT_UART_TX, SOFT_UART_RX);

String getGsmUser(uint8_t n)
{
	const uint8_t singleUserLength = GSM_USERS_TABLE_size / GSM_USERS_NUMBER;
	String userStr = readConfigString((GSM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}

String sendATCommand(String cmd, bool waiting)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	uart2.flush();
	String _resp = "";
	uart2.println(cmd);
#ifdef DEBUG_MODE
	Serial.print(F(">>"));
	Serial.println(cmd);
#endif
	if (waiting)
	{
		const uint32_t _timeout = millis() + SMS_TIME_OUT;
		while (!uart2.available() && millis() < _timeout)
		{
			yield();
		}
		while (uart2.available())
		{
			_resp += uart2.readString();
			yield();
		}
#ifdef DEBUG_MODE
		Serial.println("<<" + _resp);
#endif
		if (_resp.length() <= 0)
		{
#ifdef DEBUG_MODE
			Serial.println("Timeout...");
#endif
		}

		if (stopSerial)
			uart2.end();
		return _resp;
	}
	return _resp;
}

bool initModem()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	bool resp = true;
	String response = sendATCommand(AT_COMMAND_INIT1, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT2, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT3, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	response = sendATCommand(AT_COMMAND_INIT4, true);
	if (response.lastIndexOf(F("OK")) <= 0)
		resp &= false;
	if (stopSerial)
		uart2.end();

	return resp;
}

bool sendSMS(String& phone, String& smsText)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	String cmdTmp;
	cmdTmp.reserve(175);
	while (smsText.length() > 0)
	{
		yield();
		initModem();
		cmdTmp = AT_COMMAND_SEND;
		cmdTmp.reserve(175);
		cmdTmp += phone;
		cmdTmp += F("\"");
		sendATCommand(cmdTmp, true);
		cmdTmp.clear();
		if (smsText.length() > 150)
		{
			cmdTmp += smsText.substring(0, 150);
			smsText = smsText.substring(151, smsText.length());
		}
		else
		{
			cmdTmp += smsText;
			smsText.clear();
		}

#ifdef DEBUG_MODE
		Serial.print(F("Sending SMS to "));
		Serial.print(phone);
		Serial.print(F(":\'"));
		Serial.print(cmdTmp);
		Serial.println(F("\'"));
		Serial.println(String(cmdTmp.length()));
#endif
		for (uint16_t i = 0; i < cmdTmp.length(); i++)
		{
			uart2.write((char)cmdTmp[i]);
			EspDelay(50);
			yield();
		}

		EspDelay(500);
		yield();
		uart2.flush();
		uart2.write((char)26);
		yield();
		cmdTmp.clear();
		const uint32_t _timeout = millis() + SMS_TIME_OUT;
		while (!uart2.available() && millis() < _timeout)
		{
			yield();
		}

		while (uart2.available())
		{
			cmdTmp += uart2.readString();
			yield();
		}

		if (cmdTmp.length() <= 0)
		{
#ifdef DEBUG_MODE
			//Serial.println("Timeout...");
#endif
		}

		if (cmdTmp.lastIndexOf(F("OK")) > 0)
		{
#ifdef DEBUG_MODE
			//Serial.println(F("SMS sent..."));
#endif
		}
		else
		{
#ifdef DEBUG_MODE
			//Serial.println(F("SMS not sent..."));
#endif
		}
	}

	if (stopSerial)
		uart2.end();

	if (cmdTmp.lastIndexOf(F("OK")) > 0)
		return true;
	return false;
}

smsMessage parseSMS(String& msg)
{
	String msgheader = "";
	String msgbody = "";
	String msgphone = "";

	msg = msg.substring(msg.indexOf(F("+CMGR: ")));
	msgheader = msg.substring(0, msg.indexOf("\r"));

	msgbody = msg.substring(msgheader.length() + 2);
	msgbody = msgbody.substring(0, msgbody.lastIndexOf(F("OK")));
	msgbody.trim();

	int firstIndex = msgheader.indexOf(F("\",\"")) + 3;
	int secondIndex = msgheader.indexOf(F("\",\""), firstIndex);
	msgphone = msgheader.substring(firstIndex, secondIndex);
	if (msgphone.length() <= 0)
		msgphone = SPACE;

	smsMessage response;
	response.PhoneNumber = msgphone;
	for (uint8_t i = 0; i < GSM_USERS_NUMBER; i++)
	{
		String gsmUser = getGsmUser(i);
		if (msgphone == gsmUser)
		{
			if (i == 0)
				response.IsAdmin = true;
			response.Message = msgbody;
			break;
		}
		yield();
	}

	return response;
}

smsMessage getSMS()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	initModem();
	//getModemStatus();
	smsMessage sms;
	for (int i = 1; i <= 15; i++)
	{
		const String cmd = String(AT_COMMAND_GET_SMS) + String(i);
		String _response = sendATCommand(cmd, true);
		_response.trim();
		if (_response.lastIndexOf(F("OK")) > 0)
		{
			deleteSMS(i);
			sms = parseSMS(_response);
			break;
		}
		yield();
	}
#ifdef DEBUG_MODE
	//Serial.println("Sms: " + sms.Message);
	//Serial.println("From: " + sms.PhoneNumber);
#endif
	if (stopSerial)
		uart2.end();

	return sms;
}

bool deleteSMS(int n)
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	const String cmd = String(AT_COMMAND_DELETE_SMS) + String(n);
	const String response = sendATCommand(cmd, true);
	if (stopSerial)
		uart2.end();
	if (response.lastIndexOf(F("OK")) > 0)
		return true;
	else
		return false;
}

String getModemStatus()
{
	bool stopSerial = false;
	if (!uart2.isListening())
	{
		uart2.begin(SOFT_UART_SPEED, SWSERIAL_8N1, SOFT_UART_TX, SOFT_UART_RX, false, 64);
		stopSerial = true;
	}

	String response = sendATCommand(AT_COMMAND_STATUS1, true);
	response += sendATCommand(AT_COMMAND_STATUS2, true);
	response += sendATCommand(AT_COMMAND_STATUS3, true);
	response += sendATCommand(AT_COMMAND_STATUS4, true);

	if (stopSerial)
		uart2.end();

	//if (response.lastIndexOf(F("OK")) > 0) return true;
	//else return false;
	return response;
}

bool tryHandleGsmCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	// send_sms=[*/user#],message
	if (cmd.command == CMD_SEND_SMS)
	{
		uint8_t i = 0;
		uint8_t j = 0;
		if (cmd.arguments[0] == STAR) //if * then all
		{
			i = 0;
			j = GSM_USERS_NUMBER;
		}
		else
		{
			i = cmd.arguments[0].toInt();
			j = i + 1;
		}

		if (i <= GSM_USERS_NUMBER)
		{
			for (; i < j; i++)
			{
				String gsmUser = getGsmUser(i);
				if (gsmUser.length() > 0)
				{
					if (sendSMS(gsmUser, cmd.arguments[1]))
					{
						str += F("SMS sent");
					}
					else
					{
						str += F("SMS not sent");
					}
				}
				yield();
			}
		}
	}
	else if (cmd.command == CMD_GSM_USER)
	{
		if (cmd.index >= 0 && cmd.index < GSM_USERS_NUMBER)
		{
			uint16_t m = GSM_USERS_TABLE_size / GSM_USERS_NUMBER;
			writeConfigString(GSM_USERS_TABLE_addr + cmd.index * m, m, cmd.arguments[0]);
			str += REPLY_GSM_USER;
			str += String(cmd.index);
			str += F(" is now: ");
			str += cmd.arguments[0];
		}
		else
		{
			str += F("User #");
			str += String(cmd.index);
			str += F(" is out of range [0,");
			str += String(GSM_USERS_NUMBER - 1);
			str += F("]");
		}
	}
	else if (cmd.command == CMD_GSM_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_OFF_NUMBER);
			gsmEnable = false;
			str += REPLY_GSM_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(GSM_ENABLE_addr, GSM_ENABLE_size, SWITCH_ON_NUMBER);
			gsmEnable = true;
			str += REPLY_GSM_ENABLE;
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

bool tryHandleGsmAction(commandTokens& cmd, String& tmpAction)
{
	//send_sms=* / user#,message
	if (gsmEnable && cmd.command == ACTION_SEND_SMS)
	{
		uint8_t i = 0;
		uint8_t j = 0;
		if (cmd.arguments[0] == STAR) //if * then all
		{
			i = 0;
			j = GSM_USERS_NUMBER;
		}
		else
		{
			i = cmd.arguments[0].toInt();
			j = i + 1;
		}

		tmpAction = tmpAction.substring(tmpAction.indexOf(',') + 1);
		sensorDataCollection sensorData = collectData();
		for (; i < j; i++)
		{
			String gsmUser = getGsmUser(i);
			if (gsmUser.length() > 0)
			{
				String tmpStr = deviceName;
				tmpStr += COLON;
				tmpStr += cmd.arguments[1];
				tmpStr += EOL;
				tmpStr += parseSensorReport(sensorData, EOL);
				bool sendOk = sendSMS(gsmUser, tmpStr);
				if (!sendOk)
				{
					/*#ifdef EVENTS_ENABLE
										eventDisabled[eventNum] = false;
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
