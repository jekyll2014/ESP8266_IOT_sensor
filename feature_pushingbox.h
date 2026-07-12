#pragma once

// PushingBox
#ifdef PUSHINGBOX_ENABLE
#define PUSHINGBOX_SUBJECT F("device_name")

bool pushingBoxEnable = false;

bool sendToPushingBoxServer(String message)
{
	bool flag = false;
	if (pushingBoxEnable && WiFi.status() == WL_CONNECTED)
	{
		WiFiClient client;

		const String pushingBoxServer = String(F("api.pushingbox.com"));
		if (client.connect(pushingBoxServer, 80))
		{
			String pushingBoxId = readConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size);
			String pushingBoxParameter = readConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size);

			String postStr = F("devid=");
			postStr += pushingBoxId;
			postStr += F("&");
			postStr += PUSHINGBOX_SUBJECT;
			postStr += F("=");
			postStr += deviceName;
			postStr += F("&");
			postStr += pushingBoxParameter;
			postStr += F("=");
			postStr += String(message);
			postStr += F("\r\n\r\n");

			client.print(F("POST /pushingbox HTTP/1.1\n"));
			client.print(F("Host: "));
			client.print(pushingBoxServer);
			client.print(F("\nConnection: close\n"));
			client.print(F("Content-Type: application/x-www-form-urlencoded\n"));
			client.print(F("Content-Length: "));
			client.print(postStr.length());
			client.print(F("\n\n"));
			client.print(postStr);
			// check server reply
			flag = true;
		}
		client.stop();
	}
	return flag;
}

bool sendToPushingBox(String& message)
{
	const uint16_t pushingBoxMessageMaxSize = 1000;
	bool result = true;
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= pushingBoxMessageMaxSize)
		{
			result &= sendToPushingBoxServer(message.substring(0, pushingBoxMessageMaxSize));
			message = message.substring(pushingBoxMessageMaxSize);
		}
		else
		{
			result &= sendToPushingBoxServer(message);
			message.clear();
		}
		yield();
	}
	return result;
}

bool tryHandlePushingboxCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	//send_pushingbox - to be implemented
	if (cmd.command == CMD_PUSHINGBOX_TOKEN)
	{
		str += REPLY_PUSHINGBOX_TOKEN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(PUSHINGBOX_ID_addr, PUSHINGBOX_ID_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_PUSHINGBOX_PARAMETER)
	{
		str += REPLY_PUSHINGBOX_PARAMETER;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(PUSHINGBOX_PARAM_addr, PUSHINGBOX_PARAM_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_PUSHINGBOX_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_OFF_NUMBER);
			//pushingBoxEnable = false;
			str += REPLY_PUSHINGBOX_ENABLE;
			str += eq;
			str += REPLY_DISABLED;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(PUSHINGBOX_ENABLE_addr, PUSHINGBOX_ENABLE_size, SWITCH_ON_NUMBER);
			//pushingBoxEnable = true;
			str += REPLY_PUSHINGBOX_ENABLE;
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

bool tryHandlePushingboxAction(commandTokens& cmd, String& tmpAction)
{
	//send_PushingBox=message
	if (pushingBoxEnable && cmd.command == ACTION_SEND_PUSHINGBOX)
	{
		sensorDataCollection sensorData = collectData();
		String tmpStr = deviceName;
		tmpStr += F(": ");
		tmpStr += cmd.arguments[0];
		tmpStr += EOL;
		tmpStr += parseSensorReport(sensorData, EOL);
		bool sendOk = sendToPushingBox(tmpStr);
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
