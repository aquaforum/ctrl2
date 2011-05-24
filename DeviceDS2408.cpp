#include "DeviceDS2408.h"
#include "dallas/ds2408.h"

OneWireDevice *DeviceDS2408::clone() const
{
	DeviceDS2408 *device = new DeviceDS2408();
	*device = *this;
	return device;
}

DeviceDS2408 &DeviceDS2408::operator=(const DeviceDS2408 &source)
{
	static_cast<OneWireDevice &>(*this) = static_cast<const OneWireDevice &>(source);
	inputStates = source.inputStates;
	outputStates = source.outputStates;
	return *this;
}

DallasError DeviceDS2408::readConfiguration()
{
	QMutexLocker locker(busMutex);
	DallasError error = ds2408_read_output(&id, &outputStates);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	return error;
}

DallasError DeviceDS2408::writeConfiguration()
{
	QMutexLocker locker(busMutex);
	ds2408_write_register(&id, CONTROL_STATUS, RSTZ_STRB); // debug!! don't forget to remove!!
	DallasError error = ds2408_write_output(&id, outputStates);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	return error;
}

DallasError DeviceDS2408::readState()
{
	unsigned char oldInputStates = inputStates;

	QMutexLocker locker(busMutex);
	DallasError error = ds2408_read_input(&id, &inputStates);
	if (error != DALLAS_NO_ERROR) {
		QString message(dallasGetErrorText(error));
		locker.unlock();
		emitError(message);
	}
	else {
		locker.unlock();
	}

	for (int i = 0; i < ChannelCount; ++i) {
		if ((inputStates ^ oldInputStates) & (1 << i))
			emitChannelStateChanged(i, (oldInputStates >> i) & 1, (inputStates >> i) & 1);
	}

	return error;
}
