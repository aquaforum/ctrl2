#include <QMessageBox>
#include "DS18B20SettingsDialog.h"

DS18B20SettingsDialog::DS18B20SettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
}

void DS18B20SettingsDialog::setDevice(DeviceDS18B20 *device)
{
	m_device = device;
	idLabel->setText(OneWireDevice::dallasRomIdString(device->romId()));
	tempDevice = *device;
	resolutionSpinBox->setValue(tempDevice.resolution());
}

void DS18B20SettingsDialog::accept()
{
	DallasError error;
	setCursor(Qt::WaitCursor);
	error = tempDevice.writeConfiguration();
	unsetCursor();
	if (error != DALLAS_NO_ERROR) {
		QMessageBox::warning(this, tr("Dallas error"), QString(dallasGetErrorText(error)));
		return;
	}
	*m_device = tempDevice;
	QDialog::accept();
}

void DS18B20SettingsDialog::on_resolutionSpinBox_valueChanged(int /*i*/)
{
	const int Tconv = 750;
	tempDevice.setResolution(resolutionSpinBox->value());
	stepSizeLabel->setText(DeviceDS18B20::temperatureText(
		1.0 / (1 << (tempDevice.resolution() - DS18B20_RES_MIN + 1))));
	conversionTimeLabel->setText(QString::number(
		double(Tconv) / (1 << (DS18B20_RES_MAX - tempDevice.resolution())), 'f', 2) + " ms");
}

