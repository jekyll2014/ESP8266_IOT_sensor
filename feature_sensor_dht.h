#pragma once

// DHT11/DHT21(AM2301)/DHT22(AM2302, AM2321) temperature + humidity sensor
#ifdef DHT_ENABLE
#include <DHT.h>

bool dht_enable;
#endif

#ifdef DHT_ENABLE
void readDht(sensorDataCollection& sensorData)
{
	DHT dht(DHT_PIN, DHT_ENABLE);
	dht.begin();
	if (!dht.read())
	{
		dht_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find DHT sensor!"));
#endif
	}
	else
	{
		dht_enable = true;
		sensorData.dht_humidity = dht.readHumidity();
		sensorData.dht_temp = dht.readTemperature();
	}
}
#endif
