#pragma once

// SMTP
#ifdef SMTP_ENABLE
#include <sendemail.h>

bool smtpEnable = false;

bool sendMail(String& subject, String& message, String& addressTo)
{
	String smtpServerAddress = readConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size);
	uint16_t smtpServerPort = readConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size).toInt();
	String smtpLogin = readConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size);
	String smtpPassword = readConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size);
	if (addressTo.length() <= 0)
		addressTo = readConfigString(SMTP_TO_addr, SMTP_TO_size);

	bool result = false;
	if (smtpEnable && WiFi.status() == WL_CONNECTED)
	{
		SendEmail smtp(smtpServerAddress, smtpServerPort, smtpLogin, smtpPassword, 20000, true);
		result = smtp.send(smtpLogin, addressTo, subject, message);
	}
	return result;
}

bool tryHandleSmtpCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	// send_mail=address,subject,message
	if (cmd.command == CMD_SEND_MAIL)
	{
		if (cmd.arguments[0].length() > 0)
		{
			if (cmd.arguments[1].length() > 0)
			{
				if (sendMail(cmd.arguments[1], cmd.arguments[2], cmd.arguments[0]))
				{
					str += F("Mail sent");
				}
				else
				{
					str += F("Mail send failed");
				}
			}
			else
				str += F("Message empty");
		}
		else
			str += F("Address empty");
	}
	else if (cmd.command == CMD_SMTP_SERVER)
	{
		str += REPLY_SMTP_SERVER;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(SMTP_SERVER_ADDRESS_addr, SMTP_SERVER_ADDRESS_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_SMTP_PORT)
	{
		uint16_t smtpServerPort = cmd.arguments[0].toInt();
		str += REPLY_SMTP_PORT;
		str += eq;
		str += String(smtpServerPort);
		str += QUOTE;
		writeConfigString(SMTP_SERVER_PORT_addr, SMTP_SERVER_PORT_size, String(smtpServerPort));
	}
	else if (cmd.command == CMD_SMTP_LOGIN)
	{
		str += REPLY_SMTP_LOGIN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(SMTP_LOGIN_addr, SMTP_LOGIN_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_SMTP_PASS)
	{
		str += REPLY_SMTP_PASS;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(SMTP_PASSWORD_addr, SMTP_PASSWORD_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_SMTP_TO)
	{
		str += REPLY_SMTP_TO;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(SMTP_TO_addr, SMTP_TO_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_SMTP_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_OFF_NUMBER);
			//smtpEnable = false;
			str += REPLY_SMTP_ENABLE;
			str += eq;
			str += REPLY_DISABLED;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(SMTP_ENABLE_addr, SMTP_ENABLE_size, SWITCH_ON_NUMBER);
			//smtpEnable = true;
			str += REPLY_SMTP_ENABLE;
			str += eq;
			str += REPLY_ENABLED;
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

bool tryHandleSmtpAction(commandTokens& cmd, String& tmpAction)
{
	//send_mail=address_to,subject,message
	if (smtpEnable && cmd.command == ACTION_SEND_MAIL)
	{
		sensorDataCollection sensorData = collectData();
		String tmpStr = cmd.arguments[2];
		tmpStr += EOL;
		tmpStr += parseSensorReport(sensorData, EOL);
		String subj = deviceName;
		subj += F(" alert");
		bool sendOk = sendMail(cmd.arguments[1], tmpStr, cmd.arguments[0]);
		if (!sendOk)
		{
			/*#ifdef EVENTS_ENABLE
							eventsFlags[eventNum] = false;
			#endif*/
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif
