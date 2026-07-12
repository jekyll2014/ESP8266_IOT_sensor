#pragma once

// BMP180 I2C temperature + pressure sensor
#ifdef BMP180_ENABLE
#include <Adafruit_BMP085.h>

bool bmp180_enable;
#endif

#ifdef BMP180_ENABLE
void readBmp180(sensorDataCollection& sensorData)
{
	Adafruit_BMP085 bmp180;
	if (!bmp180.begin())
	{
		bmp180_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find BMP180 sensor!"));
#endif
	}
	else
	{
		bmp180_enable = true;
		sensorData.bmp180_temp = bmp180.readTemperature();
		sensorData.bmp180_pressure = bmp180.readPressure();
	}
}
#endif
