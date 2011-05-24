#ifndef DEVICEDS18B20_H
#define DEVICEDS18B20_H

#include "OneWireBus.h"
#include "dallas/ds18x20.h"

class DeviceDS18B20 : public OneWireDevice {
public:
	static const int ChannelCount = 1;

public:
	DeviceDS18B20() : OneWireDevice(DS18B20_FAMILY) { m_temperature = 85; }
	~DeviceDS18B20() { }
	
	OneWireDevice *clone() const;
	DeviceDS18B20 &operator=(const DeviceDS18B20 &source);

	// configuration

	unsigned char resolution()						{ return m_resolution; }
	void setResolution(unsigned char resolution)	{ m_resolution = resolution; }

	DallasError readConfiguration();
	DallasError writeConfiguration();

	// state

	unsigned short value()							{ return m_temperature; }
	double temperature()							{ return valueToTemperature(m_temperature); }
	double valueToTemperature(unsigned short value)	{ return double(value) / 16; }

	static QString temperatureText(double deg)		{ return QString::number(deg, 'f', 4) + " °Ñ"; }

	DallasError readState();

private:
	unsigned char m_resolution;
	unsigned short m_temperature;
};

#endif //DEVICEDS18B20_H