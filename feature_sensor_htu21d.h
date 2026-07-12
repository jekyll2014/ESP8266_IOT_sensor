#pragma once

// HTU21D I2C temperature + humidity sensor
#ifdef HTU21D_ENABLE
#include <Adafruit_HTU21DF.h>

bool htu21d_enable;
#endif

#ifdef HTU21D_ENABLE
void readHtu21d(sensorDataCollection& sensorData)
{
	Adafruit_HTU21DF htu21d;
	if (!htu21d.begin())
	{
		htu21d_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find HTU21D sensor!"));
#endif
	}
	else
	{
		htu21d_enable = true;
		sensorData.htu21d_humidity = htu21d.readHumidity();
		sensorData.htu21d_temp = htu21d.readTemperature();
	}
}
#endif
