#ifndef ONEWIREBUS_H
#define ONEWIREBUS_H

#include <QString>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <limits.h>
#include "dallas/dallas.h"

typedef unsigned char DallasError;

class OneWireDevice;

class OneWireBus : public QThread {
	Q_OBJECT
public:
	OneWireBus();
	~OneWireBus();

	void addFamilyPrototype(OneWireDevice *prototype);

    QString portName() const                    { return m_portName; }
    void setPortName(const QString &portName)   { m_portName = portName; }
    
	unsigned int portNumber() const				{ return m_portNumber; }
    void setPortNumber(unsigned int portNumber);

	DallasError searchDevices();
	const QVector<OneWireDevice*> &devices() const	{ return m_devices; }

	void start();
	void stop();

	void pollDevices();

signals:
	void pollDevicesCompleted();

protected:
	void run();

private:
    QString m_portName;
	unsigned int m_portNumber;
	bool dallasLibraryInitialized;
	QVector<OneWireDevice*> m_devices;
	OneWireDevice *prototypes[UCHAR_MAX + 1];
	volatile bool started;
	QMutex mutex;
};

class OneWireDevice : public QObject {
	Q_OBJECT

public:
	OneWireDevice(unsigned char family) { busMutex = 0; id.byte[0] = family; }
	~OneWireDevice() { }

	// identification

	dallas_rom_id_T romId() const					{ return id; }
	void setRomId(const dallas_rom_id_T &romId)		{ id = romId; }
	unsigned char family() const					{ return id.byte[0]; }

	static QString dallasRomIdString(const dallas_rom_id_T &id);
	static QString dallasFamilyString(const dallas_rom_id_T &id);

	// configuration and state

	virtual DallasError readConfiguration() { return DALLAS_NO_ERROR; }
	virtual DallasError writeConfiguration() { return DALLAS_NO_ERROR; }
	virtual DallasError readState() { return DALLAS_NO_ERROR; }

	// bus synchronization 

	QMutex *mutex() const					{ return busMutex; }
	void setMutex(QMutex *mutex)			{ busMutex = mutex; }

	// copying

	virtual OneWireDevice *clone() const = 0;
	OneWireDevice &operator=(const OneWireDevice &source) { id = source.id; busMutex = source.busMutex; return *this; }

	void emitError(const QString &message) { emit errorOccured(message); }

signals:
	void errorOccured(QString message);
	void channelStateChanged(int channel, unsigned short oldState, unsigned short newState);

protected:
	void emitChannelStateChanged(int channel, unsigned short oldState, unsigned short newState) { emit channelStateChanged(channel, oldState, newState); }

	dallas_rom_id_T id;
	QMutex *busMutex;
};

#endif // ONEWIREBUS_H
