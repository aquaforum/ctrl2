#include "OneWireBusModel.h"
#include "dallas/dallas.h"
#include "DeviceDS2408.h"
#include "DeviceDS2450.h"
#include "DeviceDS18B20.h"

QModelIndex OneWireBusModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!parent.isValid())
		return createIndex(row, column);									// device index
	else if (!parent.internalPointer())
		return createIndex(row, column, bus->devices().at(parent.row()));	// device channel index
	else
		return QModelIndex();
}

QModelIndex	OneWireBusModel::parent(const QModelIndex &index) const
{
	if (!index.internalPointer())
		return QModelIndex();
	else
		return createIndex(bus->devices().indexOf(static_cast<OneWireDevice *>(index.internalPointer())),
			index.column());
}

int OneWireBusModel::rowCount(const QModelIndex &parent) const
{
	if (!bus)
		return 0;
	if (!parent.isValid())
		return bus->devices().count();
	if (!parent.internalPointer()) {
		OneWireDevice *device = bus->devices().at(parent.row());
		switch (device->family()) {
			case DS2408_FAMILY: return DeviceDS2408::ChannelCount;
			case DS2450_FAMILY: return DeviceDS2450::ChannelCount;
			case DS18B20_FAMILY: return DeviceDS18B20::ChannelCount;
			default: return 0;
		}
	}
	return 0;
}

int OneWireBusModel::columnCount(const QModelIndex &/*parent*/) const
{
	return 2;
}

QVariant OneWireBusModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();
	if (!index.internalPointer()) {
		OneWireDevice *device = bus->devices().at(index.row());
		if (index.column() == 0)
			return OneWireDevice::dallasFamilyString(device->romId());
		else if (index.column() == 1)
			return OneWireDevice::dallasRomIdString(device->romId());
	}
	else {
		OneWireDevice *device = static_cast<OneWireDevice *>(index.internalPointer());
		DeviceDS2450 *adc = 0;
		DeviceDS2408 *switch8 = 0;
		DeviceDS18B20 *thermometer = 0;
		switch (device->family()) {
			case DS2408_FAMILY: switch8 = static_cast<DeviceDS2408 *>(device); break;
			case DS2450_FAMILY: adc = static_cast<DeviceDS2450 *>(device); break;
			case DS18B20_FAMILY: thermometer = static_cast<DeviceDS18B20 *>(device); break;
		}

		if (index.column() == 0) {
			if (adc) {
				return QString("#") + QString::number(index.row() + 1) + (adc->isOutputActivated(index.row()) ? " ON" : " OFF");
			}
			else if (switch8) {
				return QString("#") + QString::number(index.row() + 1) + (switch8->isOutputActivated(index.row()) ? " ON" : " OFF");
			}
			else {
				return QString("#") + QString::number(index.row() + 1);
			}
		}
		else if (index.column() == 1) {
			if (adc) {
				return adc->milliVoltsText(adc->milliVolts(index.row())).rightJustified(11, QChar('0')) + 
					" (" + QString::number(adc->value(index.row())).rightJustified(5, QChar('0')) + ")";
			}
			else if (switch8) {
				return QString("#") + QString::number(index.row() + 1) + (switch8->isInputHigh(index.row()) ? " HIGH" : " LOW");
			}
			else if (thermometer) {
				return DeviceDS18B20::temperatureText(thermometer->temperature());
			}
			else {
				return QString("");
			}
		}
	}
	return QVariant();
}

QVariant OneWireBusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section == 0) {
			return tr("Òèï");
		} else if (section == 1) {
			return tr("ID");
		}
	}
	return QVariant();
}

OneWireDevice *OneWireBusModel::deviceFromIndex(const QModelIndex &index)
{
	if (!index.isValid())
		return 0;
	if (!index.internalPointer())
		return bus->devices().at(index.row());
	else
		return static_cast<OneWireDevice *>(index.internalPointer());
}

int OneWireBusModel::channelFromIndex(const QModelIndex &index)
{
	if (!index.isValid())
		return -1;
	if (!index.internalPointer())
		return -1;
	else
		return index.row();
}

void OneWireBusModel::setBus(OneWireBus *bus)
{
	this->bus = bus; 
	reset();
	if (bus) {
		foreach(OneWireDevice *device, bus->devices()) {
			connect(device, SIGNAL(channelStateChanged(int, unsigned short, unsigned short)),
				this, SLOT(channelChanged(int)));
			connect(device, SIGNAL(errorOccured(QString)),
				this, SIGNAL(errorOccured(QString)));
		}
	}
}

void OneWireBusModel::channelChanged(int channel)
{
	OneWireDevice *device = static_cast<OneWireDevice *>(sender());
	dataChanged(createIndex(channel, 0, device), createIndex(channel, 1, device));
}
