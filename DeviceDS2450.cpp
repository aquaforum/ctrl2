#include "OneWireBus.h"
#include "DeviceDS2450.h"
#include "dallas/ds2450.h"
#include <QTime>

int DeviceDS2450::SamplingSeriesLength = 4;

static const double DefaultDiscreteness = 0.1;
static const int DefaultMedianWindowSize = 15;

DeviceDS2450::DeviceDS2450() : 
	OneWireDevice(DS2450_FAMILY)
{
	for (int i = 0; i < ChannelCount; ++i) {
		ranges[i] = DS2450_RANGE_2V;
		outputStates[i] = 0;
		resolutions[i] = 8;
		values[i] = 0;
		filters[i] = 0;
		setFilterType(i, MedianLowPass);
		setDiscreteness(i, DefaultDiscreteness);
	}
}

DeviceDS2450::~DeviceDS2450()
{
	for (int i = 0; i < ChannelCount; ++i) {
		if (filters[i])
			delete filters[i];
	}
}

DeviceDS2450 &DeviceDS2450::operator=(const DeviceDS2450 &source)
{
	QMutexLocker locker(busMutex);
	static_cast<OneWireDevice &>(*this) = static_cast<const OneWireDevice &>(source);
	memcpy(ranges, source.ranges, sizeof(ranges));
	memcpy(outputStates, source.outputStates, sizeof(outputStates));
	memcpy(resolutions, source.resolutions, sizeof(resolutions));
	memcpy(values, source.values, sizeof(values));
	for (int i = 0; i < ChannelCount; ++i) {
		setFilterType(i, source.filterType(i));
		setDiscreteness(i, source.discreteness(i));
	}
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
		emitError(message);
	}
	return error;
}

DallasError DeviceDS2450::readState()
{
	unsigned short newValues[ChannelCount];

	QMutexLocker locker(busMutex);
	for (int k = 0; k < SamplingSeriesLength; ++k) {
		unsigned short rawValues[ChannelCount];

		DallasError error = ds2450StartAll(0);		// to save 5.6 ms (actually 8-9 ms), send SKIP_ROM command instead of MATCH_ROM command and 8 bytes of rom id
		if (error == DALLAS_NO_ERROR)
			error = ds2450ResultAll(&id, rawValues);
		if (error != DALLAS_NO_ERROR) {
			QString message(dallasGetErrorText(error));
			emitError(message);
			return error;
		}
		for (int i = 0; i < ChannelCount; ++i) {
			newValues[i] = filters[i]->filter(rawValues[i]);
		}
	}
	for (int i = 0; i < ChannelCount; ++i) {
		unsigned short oldValue = values[i];
		values[i] = newValues[i];
		if (values[i] != oldValue)
			emitChannelStateChanged(i, oldValue, values[i]);
	}
	return DALLAS_NO_ERROR;
}

void DeviceDS2450::setFilterType(int channel, FilterType t)
{
	if (filterTypes[channel] != t || !filters[channel]) {
		if (filters[channel])
			delete filters[channel];
		switch(t) {
		case MovingAverage16:
			filters[channel] = new MovingAverageFilter_u16(4);	// moving average of 2^4 = 16 last samples
			break;
		case LowPass16:
			filters[channel] = new LowPassRCFilter_u16(4);	// low-pass RC filter with tau = 2^4 * delta_t
			break;
		case MedianLowPass:
			filters[channel] = new MedianLowPassRCFilter_u16(DefaultMedianWindowSize, 
				DeviceDS2450::MaximalResolution - DeviceDS2450::MinimalNoiseResolution + 1);  // combination of median filter and low-pass RC filter with variable tau
			break;
		default:
			t = NoFilter;
			filters[channel] = new DigitalFilter_u16();		// no filtering
		}
		filterTypes[channel] = t;
	}
}
