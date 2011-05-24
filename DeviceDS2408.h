#ifndef DEVICEDS2408_H
#define DEVICEDS2408_H

#include "OneWireBus.h"
#include "dallas/ds2408.h"

class DeviceDS2408 : public OneWireDevice {
public:
	static const int ChannelCount = 8;

public:
	DeviceDS2408() : OneWireDevice(DS2408_FAMILY) { inputStates = outputStates = 0; }
	~DeviceDS2408() { }
	
	OneWireDevice *clone() const;
	DeviceDS2408 &operator=(const DeviceDS2408 &source);

	// configuration

	bool isOutputActivated(int channel)						{ return outputStates & (1 << channel) ? false : true; }
	void setOutputActivated(int channel, bool isActivated)	{ outputStates = (outputStates & ~(1 << channel)) | (isActivated ? 0 : (1 << channel) ); }

	DallasError readConfiguration();
	DallasError writeConfiguration();

	// state

	bool isInputHigh(int channel)							{ return inputStates & (1 << channel) ? true : false; }

	DallasError readState();

private:
	unsigned char inputStates;
	unsigned char outputStates;
};

#endif //DEVICEDS2408_H
