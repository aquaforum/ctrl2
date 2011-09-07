#ifndef DEVICEDS2450_H
#define DEVICEDS2450_H

#include <QString>
#include "dallas/dallas.h"
#include "dallas/ds2450.h"
#include "OneWireBus.h"
#include "DigitalFilters.h"

class DeviceDS2450 : public OneWireDevice {
public:
	static const int ChannelCount = 4;
	static const int MinimalResolution = 1;
	static const int MaximalResolution = 16;
	static const int MinimalNoiseResolution = 9;

	static int SamplingSeriesLength;	// read and filter SamplingSeriesLength samples in single readState call

	enum VoltageRange { Voltage2560mV = DS2450_RANGE_2V, Voltage5120mV = DS2450_RANGE_5V };
	enum FilterType { NoFilter, MovingAverage16, LowPass16, MedianLowPass };

private:
	enum { ValueVoltage2560mV = 2560, ValueVoltage5120mV = 5120 };

public:
	DeviceDS2450();
	~DeviceDS2450();

	OneWireDevice *clone() const;
	DeviceDS2450 &operator=(const DeviceDS2450 &source);

	// configuration

	VoltageRange range(int channel) const 			{ return VoltageRange(ranges[channel]); }
	void setRange(int channel, VoltageRange range)	{ if (isValidRange(range)) ranges[channel] = range; }

	int resolution(int channel) const				{ return resolutions[channel]; }
	void setResolution(int channel, int resolution)	{ if (isValidResolution(resolution)) resolutions[channel] = resolution; }

	double discreteness(int channel) const			{ return voltageFilters[channel].discreteness(); }
	void setDiscreteness(int channel, double discreteness)	{ voltageFilters[channel].setDiscreteness(discreteness); }

	FilterType filterType(int channel) const		{ return filterTypes[channel]; }
	void setFilterType(int channel, FilterType t);

	bool isValidRange(VoltageRange range)			{ return (range == Voltage2560mV) || (range == Voltage5120mV); }
	bool isValidResolution(int resolution)			{ return (resolution >= MinimalResolution) && (resolution <= MaximalResolution); }
	bool isResolutionWithNoise(int resolution)		{ return (resolution >= MinimalNoiseResolution); }

	bool isOutputActivated(int channel)				{ return outputStates[channel] == DS2450_OUTPUT_LOW; }
	void setOutputActivated(int channel, bool isActivated)	{ outputStates[channel] = isActivated ? DS2450_OUTPUT_LOW : DS2450_OUTPUT_HIGH; }
	
	DallasError readConfiguration();
	DallasError writeConfiguration();
	DallasError readState();

	// value
	unsigned int value(int channel)					{ return values[channel] >> (MaximalResolution - resolutions[channel]); }
	double milliVolts(int channel)					{ return voltageFilters[channel].filter(voltage(values[channel], VoltageRange(ranges[channel]))); }
	double mvFromValue(int channel, unsigned int value)	{ return voltageFilters[channel].filter(voltage(value << (MaximalResolution - resolutions[channel]), VoltageRange(ranges[channel]))); }

	unsigned int maximalValue(int channel)			{ return (1 << resolutions[channel]) - 1; }
	double milliVoltsStep(int channel)				{ return voltage(1 << (MaximalResolution - resolutions[channel]), VoltageRange(ranges[channel])); }
	double maximalMilliVolts(int channel)			{ return voltage(maximalValue(channel) << (MaximalResolution - resolutions[channel]), VoltageRange(ranges[channel])); }

	static QString milliVoltsText(double mv)		{ return QString::number(mv, 'f', 3) + " mV"; }

private:
	static double voltage(unsigned short value, VoltageRange range) { return double(range == Voltage2560mV ? ValueVoltage2560mV : ValueVoltage5120mV) * value / (1 << MaximalResolution); }

	unsigned char ranges[ChannelCount];
	unsigned char outputStates[ChannelCount];
	unsigned char resolutions[ChannelCount];
	unsigned short values[ChannelCount];
	DigitalFilter_u16 *filters[ChannelCount];
	FilterType filterTypes[ChannelCount];
	HysteresisFilter_double voltageFilters[ChannelCount];
};

#endif // DEVICEDS2450_H
