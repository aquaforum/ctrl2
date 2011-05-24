#include "OneWireBus.h"
#include "dallas/ds18x20.h"
#include "dallas/ds2408.h"
#include "dallas/ds2450.h"

#include "platform.h"

#if defined(_WINDOWS_)
const char *portNameTemplate = "COM%1";
const char portNameBase = 1;
#elif defined(_WINDOWS_CE_)
const char *portNameTemplate = "COM%1:";
const char portNameBase = 1;
#elif defined(_LINUX_)
//const char *portNameTemplate = "/dev/ttyS%1";
const char *portNameTemplate = "/dev/ttyUSB%1";
const char portNameBase = 0;
#elif defined(_LINUX_EMBEDDED_)
const char *portNameTemplate = "/dev/ttySAC%1";
const char portNameBase = 0;
#else
#error _WINDOWS_ / _WINDOWS_CE_ / _LINUX_ / _LINUX_EMBEDDED_ must be defined in "platform.h"
#endif

// static
QString OneWireDevice::dallasRomIdString(const dallas_rom_id_T &id)
{
	return QString("%1-%2-%3")
		.arg(id.byte[7], 2, 16, QChar('0'))
		.arg(id.id >> 8 & 0xFFFFFFFFFFFFLL, 12, 16, QChar('0'))
		.arg(id.byte[0], 2, 16, QChar('0'));
}

// static
QString OneWireDevice::dallasFamilyString(const dallas_rom_id_T &id)
{
	switch(id.byte[0]) {
		case DS2408_FAMILY: return "DS2408"; break;
		case DS2450_FAMILY: return "DS2450"; break;
		case DS18B20_FAMILY: return "DS18B20"; break;
		case DS18S20_FAMILY: return "DS18S20"; break;
		default: return QString("0x%1").arg(id.byte[0], 2, 16, QChar('0')); break;
	}
}

OneWireBus::OneWireBus()
{
	started = false;
	m_portNumber = 0;
	dallasLibraryInitialized = false;
	memset(prototypes, 0, sizeof(prototypes));
}
 
OneWireBus::~OneWireBus()
{
	for (int i = 0; i <= UCHAR_MAX; ++i) {
		if (prototypes[i])
			delete prototypes[i];
	}
	qDeleteAll(m_devices.begin(), m_devices.end());
}

void OneWireBus::addFamilyPrototype(OneWireDevice *prototype)
{
	prototypes[prototype->family()] = prototype;
	prototype->setMutex(&mutex);
}

void OneWireBus::start()
{
	if (!started) {
		started = true;
		QThread::start();
	}
}

void OneWireBus::stop()
{
	if (started) {
		started = false;
		wait();
	}
}

void OneWireBus::run()
{
	while (started) {
		pollDevices();
	}
}

void OneWireBus::pollDevices()
{
	foreach(OneWireDevice *device, m_devices) {
		device->readState();
		yieldCurrentThread();
	}
	emit pollDevicesCompleted();
}

void OneWireBus::setPortNumber(unsigned int portNumber)
{
    m_portNumber = portNumber;
    setPortName(QString(portNameTemplate).arg(m_portNumber - 1 + portNameBase));
}

DallasError OneWireBus::searchDevices()
{
	DallasError error;

	stop();

	qDeleteAll(m_devices);
	m_devices.clear();

	if (dallasLibraryInitialized) {
		dallasDeinit();
		dallasLibraryInitialized = false;
	}

	error = dallasInit(m_portName.toLatin1().data());
	if (error != DALLAS_NO_ERROR)
		return error;

	dallasLibraryInitialized = true;

	dallas_rom_id_T id;
	dallasFindInit();
	while (dallasFindNextDevice(&id, &error)) {
		if (prototypes[id.byte[0]]) {
			OneWireDevice *device = prototypes[id.byte[0]]->clone();
			m_devices.append(device);
			device->setRomId(id);
			device->readConfiguration();
			device->readState();
		}
	}
	return error;
}
