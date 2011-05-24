#include <QtGui>
#include "OneWireTestMainWindow.h"
#include "OneWireBusModel.h"
#include "DeviceDS2408.h"
#include "DS2450SettingsDialog.h"
#include "DS18B20SettingsDialog.h"
#include "platform.h"

OneWireTestMainWindow::OneWireTestMainWindow(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	setupUi(this);

    on_portSpinBox_valueChanged(portSpinBox->value());

#if defined(_WINDOWS_) || defined(_WINDOWS_CE_)
    portLineEdit->hide();
#elif defined(_LINUX_) || defined(_LINUX_EMBEDDED_)
    portSpinBox->hide();
#endif
	
	model = new OneWireBusModel(parent);
	model->setBus(&bus);
	connect(model, SIGNAL(errorOccured(QString)),
		this, SLOT(busErrorOccured(QString)));

	connect(&bus, SIGNAL(pollDevicesCompleted()),
		this, SLOT(busPollDevicesCompleted()));

	devicesTreeView->setModel(model);
	devicesTreeView->header()->setStretchLastSection(true);
	started = false;

	bus.addFamilyPrototype(new DeviceDS2408());
	bus.addFamilyPrototype(new DeviceDS2450());
	bus.addFamilyPrototype(new DeviceDS18B20());

	errorLabel = new QLabel(statusbar);				// left widget on status bar
	statusbar->addWidget(errorLabel, 1);

	pollingPeriodLabel = new QLabel(statusbar);
	pollingPeriodLabel->setAlignment(Qt::AlignRight);
	pollingPeriodLabel->setText("9999 ms");
	pollingPeriodLabel->setMinimumSize(pollingPeriodLabel->sizeHint());
	pollingPeriodLabel->setText("");
	statusbar->addWidget(pollingPeriodLabel);	// right widget on status bar
}

OneWireTestMainWindow::~OneWireTestMainWindow()
{

}

bool OneWireTestMainWindow::search()
{
	setCursor(Qt::WaitCursor);
	model->setBus(0);
	if (portSpinBox->isVisible()) {
    	bus.setPortNumber(portSpinBox->value());
    }
    else {
    	bus.setPortName(portLineEdit->text());
    }
	DallasError error = bus.searchDevices();
	unsetCursor();
	if (error != DALLAS_NO_ERROR) {
		showDallasError(error);
		return false;
	}
	else if (bus.devices().isEmpty()) {
		QMessageBox::warning(this, tr("OneWireTestMainWindow"), tr("No devices found"));
		return false;
	}
	else {
		model->setBus(&bus);
		devicesTreeView->header()->setStretchLastSection(true);
		devicesTreeView->expandAll();
		devicesTreeView->resizeColumnToContents(0);
		devicesTreeView->resizeColumnToContents(1);
		return true;
	}
}

void OneWireTestMainWindow::start()
{
	lastPollingTime = QTime::currentTime();
	bus.start();
}

void OneWireTestMainWindow::stop()
{
	bus.stop();
}

void OneWireTestMainWindow::on_startStopPushButton_clicked()
{
	if (started) {
		stop();
		startStopPushButton->setText(tr("Старт"));
	}
	else {
		if (!search())
			return;
		start();
		startStopPushButton->setText(tr("Стоп"));
	}
	started = !started;
}

void OneWireTestMainWindow::on_devicesTreeView_clicked(const QModelIndex & index)
{
	OneWireDevice *device = model->deviceFromIndex(index);
	if (!device)
		return;
	if (device->family() == DS2450_FAMILY) {
		DeviceDS2450 *adc = static_cast<DeviceDS2450 *>(device);
		int channel = model->channelFromIndex(index);
		if (channel < 0) {
			DS2450SettingsDialog dialog;
			dialog.setDevice(adc);
			dialog.showMaximized();
			dialog.exec();
			this->activateWindow();
		}
		else {
			adc->setOutputActivated(channel, !adc->isOutputActivated(channel));
			adc->writeConfiguration();
		}
	}
	else if (device->family() == DS2408_FAMILY) {
		DeviceDS2408 *switch8 = static_cast<DeviceDS2408 *>(device);
		int channel = model->channelFromIndex(index);
		if (channel >= 0) {
			switch8->setOutputActivated(channel, !switch8->isOutputActivated(channel));
			switch8->writeConfiguration();
		}
	}
	else if (device->family() == DS18B20_FAMILY) {
		DeviceDS18B20 *thermometer = static_cast<DeviceDS18B20 *>(device);
		int channel = model->channelFromIndex(index);
		if (channel < 0) {
			DS18B20SettingsDialog dialog;
			dialog.setDevice(thermometer);
			dialog.showMaximized();
			dialog.exec();
			//this->activateWindow();
		}
	}
}

void OneWireTestMainWindow::on_portSpinBox_valueChanged(int)
{
   	bus.setPortNumber(portSpinBox->value());
   	portLineEdit->setText(bus.portName());
}

void OneWireTestMainWindow::showDallasError(int error)
{
	QMessageBox::warning(this, tr("Dallas error"), QString(dallasGetErrorText(error)));
}

void OneWireTestMainWindow::busErrorOccured(QString message)
{
	lastErrorTime = QTime::currentTime();
	errorLabel->setText(message);
}

void OneWireTestMainWindow::busPollDevicesCompleted()
{
	QTime now = QTime::currentTime();
	int msecs = lastPollingTime.msecsTo(now);
	if (msecs < 0)
		msecs += 86400000;
	pollingPeriodLabel->setText(QString::number(msecs) + " ms");

	msecs = lastErrorTime.msecsTo(now);
	if (msecs < 0)
		msecs += 86400000;
	if (msecs > 5000) {
		errorLabel->setText("");
	}
	lastPollingTime = now;
}

void OneWireTestMainWindow::closeEvent(QCloseEvent *event)
{
	if (started)
		stop();
	event->accept();
}
