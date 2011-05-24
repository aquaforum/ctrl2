#include <QMessageBox>
#include "DS2450SettingsDialog.h"

DS2450SettingsDialog::DS2450SettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	channelControls.append(ChannelControlSet(&tempDevice, 0, resolutionSpinBox, inputRangeComboBox, stepSizeLabel, noiseWarningLabel));
	createChannelSettingsTabs();
}

void DS2450SettingsDialog::setDevice(DeviceDS2450 *device)
{
	m_device = device;
	idLabel->setText(OneWireDevice::dallasRomIdString(device->romId()));
	tempDevice = *device;
	for (int i = 0; i < 4; ++i) {
		channelControls[i].resolutionSpinBox->setValue(tempDevice.resolution(i));
		channelControls[i].inputRangeComboBox->setCurrentIndex(tempDevice.range(i));
	}
}

void DS2450SettingsDialog::on_resolutionSpinBox_valueChanged(int /*i*/)
{
	foreach(ChannelControlSet ccs, channelControls)
		if (sender() == ccs.resolutionSpinBox)
			ccs.on_resolutionSpinBox_valueChanged();
}

void DS2450SettingsDialog::on_inputRangeComboBox_currentIndexChanged(int /*i*/)
{
	foreach(ChannelControlSet ccs, channelControls)
		if (sender() == ccs.inputRangeComboBox)
			ccs.on_inputRangeComboBox_currentIndexChanged();
}

void DS2450SettingsDialog::createChannelSettingsTabs()
{
	while(channelControls.size() < 4) {
		QWidget *tab = new QWidget;
		QGridLayout *layout = new QGridLayout(tab);
		ChannelControlSet ccs(&tempDevice, channelControls.size(), new QSpinBox(this), new QComboBox(this), new QLabel(this), new QLabel(this));

		ccs.resolutionSpinBox->setGeometry(resolutionSpinBox->geometry());
		ccs.resolutionSpinBox->setSizePolicy(resolutionSpinBox->sizePolicy());
		ccs.resolutionSpinBox->setMinimum(resolutionSpinBox->minimum());
		ccs.resolutionSpinBox->setMaximum(resolutionSpinBox->maximum());
		ccs.resolutionSpinBox->setValue(resolutionSpinBox->value());

		ccs.inputRangeComboBox->setGeometry(inputRangeComboBox->geometry());
		ccs.inputRangeComboBox->setSizePolicy(inputRangeComboBox->sizePolicy());
		for (int i = 0; i < inputRangeComboBox->count(); ++i)
			ccs.inputRangeComboBox->addItem(inputRangeComboBox->itemText(i));

		ccs.stepSizeLabel->setGeometry(stepSizeLabel->geometry());
		ccs.stepSizeLabel->setSizePolicy(stepSizeLabel->sizePolicy());
		ccs.stepSizeLabel->setFrameShape(stepSizeLabel->frameShape());
		ccs.stepSizeLabel->setText(stepSizeLabel->text());

		ccs.noiseWarningLabel->setGeometry(noiseWarningLabel->geometry());
		ccs.noiseWarningLabel->setText(noiseWarningLabel->text());

		QLabel *label = new QLabel(this);
		label->setText(resolutionLabel->text());
		layout->addWidget(label, 0, 0);
		layout->addWidget(ccs.resolutionSpinBox, 0, 1);

		label = new QLabel(this);
		label->setText(inputRangeLabel->text());
		layout->addWidget(label, 1, 0);
		layout->addWidget(ccs.inputRangeComboBox, 1, 1);

		label = new QLabel(this);
		label->setText(stepSizeCaptionLabel->text());
		layout->addWidget(label, 2, 0);
		layout->addWidget(ccs.stepSizeLabel, 2, 1);

		layout->addWidget(ccs.noiseWarningLabel, 3, 0, 1, 2);

		channelControls.append(ccs);
		channelsTabWidget->addTab(tab, QString::number(channelControls.size()));

		connect(ccs.inputRangeComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_inputRangeComboBox_currentIndexChanged(int)));
		connect(ccs.resolutionSpinBox, SIGNAL(valueChanged(int)),
			this, SLOT(on_resolutionSpinBox_valueChanged(int)));
	}
}

void DS2450SettingsDialog::accept()
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

void DS2450SettingsDialog::ChannelControlSet::on_resolutionSpinBox_valueChanged()
{
	m_device->setResolution(m_channel, resolutionSpinBox->value());
	if (m_device->resolution(m_channel) < DeviceDS2450::MinimalNoiseResolution) {
		noiseWarningLabel->hide();
	} else {
		noiseWarningLabel->show();
	}
	updateStepSize();
}

void DS2450SettingsDialog::ChannelControlSet::on_inputRangeComboBox_currentIndexChanged()
{
        m_device->setRange(m_channel, VoltageRange(inputRangeComboBox->currentIndex()));
	updateStepSize();
}

void DS2450SettingsDialog::ChannelControlSet::updateStepSize()
{
	stepSizeLabel->setText(m_device->milliVoltsText(m_device->milliVoltsStep(m_channel)));
}
