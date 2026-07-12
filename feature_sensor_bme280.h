#pragma once

// BME280 I2C temperature + humidity + pressure sensor
#ifdef BME280_ENABLE
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

bool bme280_enable;
#endif

#ifdef BME280_ENABLE
void readBme280(sensorDataCollection& sensorData)
{
	Adafruit_BME280 bme280;
	if (!bme280.begin(BME280_ENABLE))
	{
		bme280_enable = false;
#ifdef DEBUG_MODE
		Serial.println(F("Can't find BME280 sensor!"));
#endif
	}
	else
	{
		bme280_enable = true;
		sensorData.bme280_humidity = bme280.readHumidity();
		sensorData.bme280_temp = bme280.readTemperature();
		sensorData.bme280_pressure = bme280.readPressure();
	}
}
#endif
