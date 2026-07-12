#pragma once

// HTTP server
#ifdef HTTP_ENABLE
#include <ESP8266WebServer.h>

uint16_t httpPort = 80;
ESP8266WebServer http_server(httpPort);
bool httpServerEnable = false;

void handleRoot()
{
	sensorDataCollection sensorData = collectData();
	String str = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: ");
	str += String(uint16_t(checkSensorPeriod / 1000UL));
	str += F("\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
	str += parseSensorReport(sensorData, F("<br>"), false);
	str += F("<br />\r\n</html>\r\n");
	http_server.sendContent(str);
}

/*void handleRoot()
{
	sensorDataCollection sensorData = collectData();
	String str = F("Connection: close\r\nRefresh: ");
	str += String(uint16_t(checkSensorPeriod / 1000UL));
	str += F("\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
	str += parseSensorReport(sensorData, F("<br>"), false);
	str += F("<br />\r\n</html>\r\n");
	http_server.send(200, F("text/html"), str);
}*/

void handleNotFound()
{
	String message = F("File Not Found\n\nURI: ");
	message += http_server.uri();
	message += F("\nMethod: ");
	message += (http_server.method() == HTTP_GET) ? F("GET") : F("POST\nArguments: ");
	message += http_server.args();
	message += EOL;
	for (uint8_t i = 0; i < http_server.args(); i++)
	{
		message += SPACE;
		message += http_server.argName(i);
		message += F(": ");
		message += http_server.arg(i);
		message += EOL;
		yield();
	}
	http_server.send(404, F("text/plain"), message);
}

bool tryHandleHttpCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_HTTP_PORT)
	{
		httpPort = cmd.arguments[0].toInt();
		str += REPLY_HTTP_PORT;
		str += eq;
		str += String(httpPort);
		str += QUOTE;
		writeConfigString(HTTP_PORT_addr, HTTP_PORT_size, String(httpPort));
		//if (httpServerEnable) http_server.begin(httpPort);
	}
	else if (cmd.command == CMD_HTTP_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_OFF_NUMBER);
			httpServerEnable = false;
			http_server.stop();
			str += REPLY_HTTP_ENABLE;
			str += eq;
			str += REPLY_DISABLED;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(HTTP_ENABLE_addr, HTTP_ENABLE_size, SWITCH_ON_NUMBER);
			httpServerEnable = true;
			http_server.begin(httpPort);
			str += REPLY_HTTP_ENABLE;
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
#endif
