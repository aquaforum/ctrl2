#include "DeviceDS18B20.h"
#include "dallas/ds18x20.h"

OneWireDevice *DeviceDS18B20::clone() const
{
	DeviceDS18B20 *device = new DeviceDS18B20();
	*device = *this;
	return device;
}

DeviceDS18B20 &DeviceDS18B20::operator=(const DeviceDS18B20 &source)
{
	QMutexLocker locker(busMutex);
	static_cast<OneWireDevice &>(*this) = static_cast<const OneWireDevice &>(source);
	m_temperature = source.m_temperature;
	m_resolution = source.m_resolution;
	return *this;
}

DallasError DeviceDS18B20::readConfiguration()
{
	QMutexLocker locker(busMutex);
	DallasError error = ds18b20GetResolution(&id, &m_resolution);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		emitError(message);
	}
	return error;
}

DallasError DeviceDS18B20::writeConfiguration()
{
	QMutexLocker locker(busMutex);
	DallasError error = ds18b20Setup(&id, m_resolution, DS18B20_NO_ALARM_LOW, DS18B20_NO_ALARM_HIGH);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		emitError(message);
	}
	return error;
}

DallasError DeviceDS18B20::readState()
{
	DallasError error = prepareState(&id);
	if (error == DALLAS_NO_ERROR)
		error = readPreparedState();
	return error;
}

DallasError DeviceDS18B20::prepareState(dallas_rom_id_T *rom_id)
{
	QMutexLocker locker(busMutex);
	DallasError error = ds18b20Start(rom_id);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		emitError(message);
	}
	else {
		dallasWaitUntilDone();					// wait until conversion done
	}
	return error;
}

DallasError DeviceDS18B20::readPreparedState()
{
	unsigned short oldTemperature = m_temperature;

	QMutexLocker locker(busMutex);
	DallasError error = ds18b20Result(&id, &m_temperature);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		emitError(message);
		return error;
	}

	if (m_temperature != oldTemperature)
		emitChannelStateChanged(0, oldTemperature, m_temperature);

	return error;
}
