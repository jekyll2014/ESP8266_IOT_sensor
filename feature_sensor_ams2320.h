#pragma once

//AM2320 I2C temperature + humidity sensor
#ifdef AMS2320_ENABLE
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

bool am2320_enable;
#endif

#ifdef AMS2320_ENABLE
void readAms2320(sensorDataCollection& sensorData)
{
	Adafruit_AM2320 am2320(&Wire);
	if (!am2320.begin())
	{
		am2320_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find AMS2320 sensor!"));
#endif
	}
	else
	{
		am2320_enable = true;
		sensorData.ams_humidity = am2320.readHumidity();
		sensorData.ams_temp = am2320.readTemperature();
		if (isnan(sensorData.ams_temp))
			am2320_enable = false;
	}
}
#endif
