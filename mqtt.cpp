#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "configuration.h"
#include "datastructures.h"
#include "ESP8266_IOT_sensor.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

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
	if (readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size) == SWITCH_ON_NUMBER) mqttClean = true;
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
			mqtt_server = "";
			mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else
		{
			mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
		}

		mqtt_client.setCallback(mqtt_callback);

		if (mqtt_device_id.length() <= 0)
		{
			mqtt_device_id = deviceName;
			mqtt_device_id += "_";
			mqtt_device_id += WiFi.macAddress();
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

bool mqtt_send(String& message, int dataLength, String topic)
{
	if (!mqtt_client.connected())
	{
		mqtt_connect();
	}

	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		if (topic.length() <= 0) topic = getMqttTopicOut();

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

