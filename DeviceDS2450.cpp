#include "OneWireBus.h"
#include "DeviceDS2450.h"
#include "dallas/ds2450.h"

DeviceDS2450::DeviceDS2450() : 
	OneWireDevice(DS2450_FAMILY)
{
	for (int i = 0; i < ChannelCount; ++i) {
		ranges[i] = DS2450_RANGE_2V;
		outputStates[i] = 0;
		resolutions[i] = 8;
		values[i] = 0;
	}
}

DeviceDS2450 &DeviceDS2450::operator=(const DeviceDS2450 &source)
{
	static_cast<OneWireDevice &>(*this) = static_cast<const OneWireDevice &>(source);
	memcpy(ranges, source.ranges, sizeof(ranges));
	memcpy(outputStates, source.outputStates, sizeof(outputStates));
	memcpy(resolutions, source.resolutions, sizeof(resolutions));
	memcpy(values, source.values, sizeof(values));
	return *this;
}

OneWireDevice *DeviceDS2450::clone() const
{
	DeviceDS2450 *device = new DeviceDS2450();
	*device = *this;
	return device;
}

DallasError DeviceDS2450::readConfiguration()
{
	QMutexLocker locker(busMutex);
	DallasError error = ds2450ReadAllSettings(&id, resolutions, ranges, outputStates);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	return error;
}

DallasError DeviceDS2450::writeConfiguration()
{
	QMutexLocker locker(busMutex);
	DallasError error = ds2450WriteAllSettings(&id, resolutions, ranges, outputStates);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	return error;
}

DallasError DeviceDS2450::readState()
{
	unsigned short oldValues[ChannelCount];
	memcpy(oldValues, values, sizeof(values));

	QMutexLocker locker(busMutex);
	DallasError error = ds2450StartAndResultAll(&id, values);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	else {
		locker.unlock();
	}

	for (int i = 0; i < ChannelCount; ++i) {
		if (values[i] != oldValues[i])
			emitChannelStateChanged(i, oldValues[i], values[i]);
	}

	return error;
}
