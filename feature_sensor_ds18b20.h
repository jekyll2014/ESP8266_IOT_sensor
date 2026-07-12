#pragma once

// DS18B20 OneWire temperature sensor
#ifdef DS18B20_ENABLE
#include <DallasTemperature.h>

bool ds1820_enable;
#endif

#ifdef DS18B20_ENABLE
void readDs18b20(sensorDataCollection& sensorData)
{
	DallasTemperature ds1820(&oneWire);
	ds1820.begin();
	if (ds1820.getDS18Count() <= 0)
	{
		ds1820_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find DS18B20 sensor!"));
#endif
	}
	else
	{
		ds1820_enable = true;
		ds1820.requestTemperatures();
		EspDelay(5);
		sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
		if (sensorData.ds1820_temp > 100)
		{
			ds1820.requestTemperatures();
			EspDelay(5);
			sensorData.ds1820_temp = ds1820.getTempCByIndex(0);
		}
	}
}
#endif
