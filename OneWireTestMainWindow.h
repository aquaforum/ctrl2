#ifndef ONEWIRETESTMAINWINDOW_H
#define ONEWIRETESTMAINWINDOW_H

#include <QDateTime>
#include <QtGui/QMainWindow>
#include "ui_OneWireTestMainWindow.h"
#include "OneWireBus.h"

class OneWireBusModel;
class QModelIndex;
class QSettings;

class OneWireTestMainWindow : public QMainWindow, private Ui::OneWireTestMainWindow
{
	Q_OBJECT

public:
	OneWireTestMainWindow(QSettings & settings, QWidget *parent = 0, Qt::WFlags flags = 0);
	~OneWireTestMainWindow();

	bool search();
	void start();
	void stop();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void on_startStopPushButton_clicked();
	void on_devicesTreeView_clicked(const QModelIndex &index);
	void on_portSpinBox_valueChanged(int);
	void showDallasError(int error);
	void busErrorOccured(QString message);
	void busPollDevicesCompleted();

private:
	OneWireBusModel *model;
	OneWireBus bus;
	bool started;

	QTime lastPollingTime;
	QTime lastErrorTime;

	QLabel *pollingPeriodLabel;
	QLabel *errorLabel;

	QSettings &settings;
};

#endif // ONEWIRETESTMAINWINDOW_H
