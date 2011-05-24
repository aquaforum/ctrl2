#ifndef DS18B20SETTINGSDIALOG_H
#define DS18B20SETTINGSDIALOG_H

#include <QDialog>

#include "ui_DS18B20SettingsDialog.h"

#include "dallas/dallas.h"
#include "DeviceDS18B20.h"

class DS18B20SettingsDialog : public QDialog, public Ui::DS18B20SettingsDialog
{
	Q_OBJECT

public:
	DS18B20SettingsDialog(QWidget *parent = 0);

	DeviceDS18B20 *device() const { return m_device; }
	void setDevice(DeviceDS18B20 *device);

	void accept();

private slots:
	void on_resolutionSpinBox_valueChanged(int i);

private:
	DeviceDS18B20 *m_device;
	DeviceDS18B20 tempDevice;
};

#endif // DS18B20SETTINGSDIALOG_H
