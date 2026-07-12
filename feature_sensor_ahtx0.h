#pragma once

// HTU21D I2C temperature + humidity sensor
#ifdef AHTx0_ENABLE
#include <Adafruit_AHTX0.h>

bool ahtx0_enable;
#endif

#ifdef AHTx0_ENABLE
void readAhtx0(sensorDataCollection& sensorData)
{
	Adafruit_AHTX0 ahtx0;
	if (!ahtx0.begin())
	{
		ahtx0_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find AHTx0 sensor!"));
#endif
	}
	else
	{
		ahtx0_enable = true;
		sensors_event_t humidity, temp;
		ahtx0.getEvent(&humidity, &temp);
		sensorData.ahtx0_humidity = humidity.relative_humidity;
		sensorData.ahtx0_temp = temp.temperature;
	}
}
#endif
