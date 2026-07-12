#pragma once

// MQTT
#ifdef MQTT_ENABLE
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

String mqttCommand = "";
bool mqttEnable = false;
uint32_t mqttLastReconnectAttempt = 0;

String getMqttServer()
{
	return readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
}

int getMqttPort()
{
	return readConfigString(MQTT_PORT_addr, MQTT_PORT_size).toInt();
}

String getMqttUser()
{
	return readConfigString(MQTT_USER_addr, MQTT_USER_size);
}

String getMqttPassword()
{
	return readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
}

String getMqttDeviceId()
{
	return readConfigString(MQTT_ID_addr, MQTT_ID_size);
}

String getMqttTopicIn()
{
	return readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
}

String getMqttTopicOut()
{
	return readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
}

bool getMqttClean()
{
	bool mqttClean = false;
	if (readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size) == SWITCH_ON_NUMBER)
		mqttClean = true;
	return mqttClean;
}

bool mqtt_connect()
{
	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		IPAddress mqtt_ip_server;
		String mqtt_server = getMqttServer();
		uint16_t mqtt_port = getMqttPort();
		String mqtt_User = getMqttUser();
		String mqtt_Password = getMqttPassword();
		String mqtt_device_id = getMqttDeviceId();
		String mqtt_topic_in = getMqttTopicIn();
		bool mqttClean = getMqttClean();

		if (mqtt_ip_server.fromString(mqtt_server))
		{
			mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else
		{
			mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
		}

		mqtt_client.setCallback(mqtt_callback);

		espClient.stop(); // release any half-open socket from a prior failed connect() before retrying

		if (mqtt_device_id.length() <= 0)
		{
			mqtt_device_id = deviceName;
			mqtt_device_id += F("_");
			mqtt_device_id += CompactMac();
		}

		if (mqtt_User.length() > 0)
		{
			result = mqtt_client.connect(mqtt_device_id.c_str(), mqtt_User.c_str(), mqtt_Password.c_str(), "", 0, false, "", mqttClean);
		}
		else
		{
			result = mqtt_client.connect(mqtt_device_id.c_str());
		}
		if (!result)
		{
			return result;
		}
		result = mqtt_client.subscribe(mqtt_topic_in.c_str());
	}
	return result;
}

bool mqtt_send(String& message, int dataLength, String& topic)
{
	if (!mqtt_client.connected())
	{
		mqtt_connect();
	}

	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		if (topic.length() <= 0)
			topic = getMqttTopicOut();

		if (dataLength > MQTT_MAX_PACKET)
		{
			result = mqtt_client.beginPublish(topic.c_str(), dataLength, false);
			for (uint16_t i = 0; i < dataLength; i++)
			{
				mqtt_client.write(message[i]);
				yield();
			}
			result = mqtt_client.endPublish();
		}
		else
		{
			result = mqtt_client.publish(topic.c_str(), message.c_str(), dataLength);
		}
	}
	return result;
}

void mqtt_callback(char* topic, uint8_t* payload, uint16_t dataLength)
{
	for (uint16_t i = 0; i < dataLength; i++)
	{
		mqttCommand += char(payload[i]);
		yield();
	}
}

bool tryHandleMqttCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	// send_mqtt=topic,message - to be implemented
	if (cmd.command == CMD_SEND_MQTT)
	{
		if (mqtt_send(cmd.arguments[1], cmd.arguments[1].length(), cmd.arguments[0]))
		{
			str += F("Message sent");
		}
		else
		{
			str += F("Message not sent");
		}
	}
	else if (cmd.command == CMD_MQTT_SERVER)
	{
		IPAddress tmpAddr;
		str += REPLY_MQTT_SERVER;
		str += eq;
		if (tmpAddr.fromString(cmd.arguments[0]))
		{
			IPAddress mqtt_ip_server = tmpAddr;
			str += mqtt_ip_server.toString();
		}
		else
		{
			str += cmd.arguments[0];
		}
		str += QUOTE;
		writeConfigString(MQTT_SERVER_addr, MQTT_SERVER_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_PORT)
	{
		uint16_t mqtt_port = cmd.arguments[0].toInt();
		str += REPLY_MQTT_PORT;
		str += eq;
		str += String(mqtt_port);
		str += QUOTE;
		writeConfigString(MQTT_PORT_addr, MQTT_PORT_size, String(mqtt_port));
		//mqtt_client.setServer(mqtt_ip_server, mqtt_port);
	}
	else if (cmd.command == CMD_MQTT_LOGIN)
	{
		str += REPLY_MQTT_LOGIN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(MQTT_USER_addr, MQTT_USER_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_PASS)
	{
		str += REPLY_MQTT_PASS;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(MQTT_PASS_addr, MQTT_PASS_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_ID)
	{
		str += REPLY_MQTT_ID;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(MQTT_ID_addr, MQTT_ID_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_TOPIC_IN)
	{
		str += REPLY_MQTT_TOPIC_IN;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_TOPIC_OUT)
	{
		str += REPLY_MQTT_TOPIC_OUT;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_MQTT_CLEAN)
	{
		str += REPLY_MQTT_CLEAN;
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size, SWITCH_OFF_NUMBER);
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size, SWITCH_ON_NUMBER);
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
		}
		str += cmd.arguments[0];
	}
	else if (cmd.command == CMD_MQTT_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
			mqttEnable = false;
			str += REPLY_MQTT_ENABLE;
			str += eq;
			str += SWITCH_OFF;
			mqtt_client.disconnect();
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_ON_NUMBER);
			mqttEnable = true;
			str += REPLY_MQTT_ENABLE;
			str += eq;
			str += SWITCH_ON;
			mqtt_connect();
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

bool tryHandleMqttAction(commandTokens& cmd, String& tmpAction)
{
	//send_MQTT=topic_to,message
	if (mqttEnable && cmd.command == ACTION_SEND_MQTT)
	{
		String delimiter = F("\",\r\n\"");
		String eq = F("\":\"");
		String tmpStr = F("{\"");
		tmpStr += F("Device name");
		tmpStr += eq;
		tmpStr += deviceName;
		tmpStr += delimiter;
		tmpStr += F("Message");
		tmpStr += eq;
		tmpStr += cmd.arguments[1];
		tmpStr += delimiter;
		tmpStr += F("\"}");

		bool sendOk = false;
		sendOk = mqtt_send(tmpStr, tmpStr.length(), cmd.arguments[0]);
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
