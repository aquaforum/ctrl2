#ifndef DS2450SETTINGSDIALOG_H
#define DS2450SETTINGSDIALOG_H

#include <QDialog>

#include "ui_DS2450SettingsDialog.h"

#include "dallas/dallas.h"
#include "DeviceDS2450.h"

class QStandardItemModel;

class DS2450SettingsDialog : public QDialog, public Ui::DS2450SettingsDialog
{
	Q_OBJECT

public:
	DS2450SettingsDialog(QWidget *parent = 0);

	DeviceDS2450 *device() const { return m_device; }
	void setDevice(DeviceDS2450 *device);

	void accept();

private slots:
	void on_resolutionSpinBox_valueChanged(int i);
	void on_inputRangeComboBox_currentIndexChanged(int i);

private:
	void createChannelSettingsTabs();
	void updateStepSize();

	class ChannelControlSet {
	public:
		ChannelControlSet() {}
		ChannelControlSet(DeviceDS2450 *device, int channel, QSpinBox *rsb, QComboBox *ircb, QLabel *ssl, QLabel *nwl)
			: m_device(device), m_channel(channel), resolutionSpinBox(rsb), inputRangeComboBox(ircb), stepSizeLabel(ssl), noiseWarningLabel(nwl) { }

		void on_resolutionSpinBox_valueChanged();
		void on_inputRangeComboBox_currentIndexChanged();

		QSpinBox *resolutionSpinBox;
		QComboBox *inputRangeComboBox;
		QLabel *stepSizeLabel;
		QLabel *noiseWarningLabel;
		void updateStepSize();
		DeviceDS2450 *m_device;
		int m_channel;
	};

	QVector<ChannelControlSet> channelControls;

	DeviceDS2450 *m_device;
	DeviceDS2450 tempDevice;
};

#endif // DS2450SETTINGSDIALOG_H
