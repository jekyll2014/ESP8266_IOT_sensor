#pragma once

// TELNET
#ifdef TELNET_ENABLE
WiFiServer telnetServer(23);
WiFiClient serverClient[TELNET_MAX_CONNECTIONS];
bool telnetEnable = true;

void sendToTelnet(String& str, uint8_t clientN)
{
	if (serverClient[clientN] && serverClient[clientN].connected())
	{
		serverClient[clientN].print(str);
		EspDelay(1);
	}
}

bool tryHandleTelnetCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_TELNET_PORT)
	{
		uint16_t telnetPort = cmd.arguments[0].toInt();
		str += REPLY_TELNET_PORT;
		str += eq;
		str += String(telnetPort);
		str += QUOTE;
		writeConfigString(TELNET_PORT_addr, TELNET_PORT_size, String(telnetPort));
		//WiFiServer server(telnetPort);
	}
	else if (cmd.command == CMD_TELNET_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(TELNET_ENABLE_addr, TELNET_ENABLE_size, SWITCH_OFF_NUMBER);
			telnetEnable = false;
			telnetServer.stop();
			str += REPLY_TELNET_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(TELNET_ENABLE_addr, TELNET_ENABLE_size, SWITCH_ON_NUMBER);
			telnetEnable = true;
			telnetServer.begin();
			telnetServer.setNoDelay(true);
			str += REPLY_TELNET_ENABLE;
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
