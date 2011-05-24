#ifndef ONEWIREBUSMODEL_H
#define ONEWIREBUSMODEL_H

#include <QAbstractItemModel>

class OneWireBus;
class OneWireDevice;

class OneWireBusModel : public QAbstractItemModel {
	Q_OBJECT

public:
	OneWireBusModel(QObject *parent = 0) : QAbstractItemModel(parent) { bus = 0; }
	~OneWireBusModel() { }

	void setBus(OneWireBus *bus);

	QModelIndex	index(int row, int column, const QModelIndex &parent) const;
	QModelIndex	parent(const QModelIndex &index) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	OneWireDevice *deviceFromIndex(const QModelIndex &index);
	int channelFromIndex(const QModelIndex &index);

signals:
	void errorOccured(QString message);

public slots:
	void channelChanged(int channel);

private:
	OneWireBus *bus;
};

#endif //ONEWIREBUSMODEL_H