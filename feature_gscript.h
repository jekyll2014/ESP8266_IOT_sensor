#pragma once

// Google script
#ifdef GSCRIPT_ENABLE
#include <HTTPSRedirect.h>

bool gScriptEnable = false;

bool sendValueToGoogle(String& value)
{
	bool flag = false;
	if (gScriptEnable && WiFi.status() == WL_CONNECTED)
	{
		const String host = F("script.google.com");
		//const char* googleRedirHost = "script.googleusercontent.com";
		//const char* fingerprint = "C2 00 62 1D F7 ED AF 8B D1 D5 1D 24 D6 F0 A1 3A EB F1 B4 92";
		//String[] valuesNames = {"?tag", "?value="};
		String gScriptId = readConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size);
		const uint16_t httpsPort = 443;

		HTTPSRedirect gScriptClient(httpsPort);
		gScriptClient.setInsecure();
		gScriptClient.setPrintResponseBody(true);

		for (uint8_t i = 0; i < 5; i++)
		{
			int retval = gScriptClient.connect(host.c_str(), httpsPort);
			if (retval == 1)
			{
				flag = true;
				break;
			}
#ifdef DEBUG_MODE
			//else Serial.println(F("Connection failed. Retrying..."));
#endif
			yield();
		}
		if (flag)
		{
			String urlFinal = F("/macros/s/");
			urlFinal += gScriptId;
			urlFinal += F("/exec");
			urlFinal += F("?value=");
			urlFinal += value;
			flag = gScriptClient.GET(urlFinal, host.c_str(), true);
		}
		gScriptClient.stopAll();
	}
	return flag;
}

bool tryHandleGscriptCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	//send_gscript - to be implemented
	if (cmd.command == CMD_GSCRIPT_TOKEN)
	{
		str += REPLY_GSCRIPT_TOKEN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(GSCRIPT_ID_addr, GSCRIPT_ID_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_GSCRIPT_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_OFF_NUMBER);
			//gScriptEnable = false;
			str += REPLY_GSCRIPT_ENABLE;
			str += eq;
			str += REPLY_DISABLED;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(GSCRIPT_ENABLE_addr, GSCRIPT_ENABLE_size, SWITCH_ON_NUMBER);
			//gScriptEnable = true;
			str += REPLY_GSCRIPT_ENABLE;
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

bool tryHandleGscriptAction(commandTokens& cmd, String& tmpAction)
{
	//send_GScript=message
	if (gScriptEnable && cmd.command == ACTION_SEND_GSCRIPT)
	{
		sensorDataCollection sensorData = collectData();
		String tmpStr = deviceName;
		tmpStr += F(": ");
		tmpStr += cmd.arguments[0];
		tmpStr += EOL;
		tmpStr += parseSensorReport(sensorData, EOL);
		bool sendOk = sendValueToGoogle(tmpStr);
		if (!sendOk)
		{
			/*#ifdef EVENTS_ENABLE
							eventDisabled[eventNum] = false;
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
