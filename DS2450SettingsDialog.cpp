#include <QMessageBox>
#include "DS2450SettingsDialog.h"

static const double discreteness[] = {10, 5, 2, 1, 0.5, 0.2, 0.1, 0.05};

DS2450SettingsDialog::DS2450SettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	channelControls.append(ChannelControlSet(&tempDevice, 0, resolutionSpinBox, inputRangeComboBox, stepSizeLabel, 
		discretenessComboBox, filterComboBox));
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
		channelControls[i].discretenessComboBox->setCurrentIndex(0);
		for (int j = sizeof(discreteness) / sizeof(discreteness[0]) - 1; j >= 0; --j) {
			if (tempDevice.discreteness(i) <= discreteness[j]) {
				channelControls[i].discretenessComboBox->setCurrentIndex(j);
				break;
			}
		}
		tempDevice.setDiscreteness(i, discreteness[channelControls[i].discretenessComboBox->currentIndex()]);
		channelControls[i].filterComboBox->setCurrentIndex(tempDevice.filterType(i));
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

void DS2450SettingsDialog::on_filterComboBox_currentIndexChanged(int /*i*/)
{
	foreach(ChannelControlSet ccs, channelControls)
		if (sender() == ccs.filterComboBox)
			ccs.on_filterComboBox_currentIndexChanged();
}

void DS2450SettingsDialog::on_discretenessComboBox_currentIndexChanged(int /*i*/)
{
	foreach(ChannelControlSet ccs, channelControls)
		if (sender() == ccs.discretenessComboBox)
			ccs.on_discretenessComboBox_currentIndexChanged();
}

void DS2450SettingsDialog::createChannelSettingsTabs()
{
	while(channelControls.size() < 4) {
		QWidget *tab = new QWidget;
		QGridLayout *layout = new QGridLayout(tab);
		ChannelControlSet ccs(&tempDevice, channelControls.size(), new QSpinBox(this), new QComboBox(this), new QLabel(this), 
			new QComboBox(this), new QComboBox(this));

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

		ccs.discretenessComboBox->setGeometry(discretenessComboBox->geometry());
		ccs.discretenessComboBox->setSizePolicy(discretenessComboBox->sizePolicy());
		for (int i = 0; i < discretenessComboBox->count(); ++i)
			ccs.discretenessComboBox->addItem(discretenessComboBox->itemText(i));

		ccs.filterComboBox->setGeometry(filterComboBox->geometry());
		ccs.filterComboBox->setSizePolicy(filterComboBox->sizePolicy());
		for (int i = 0; i < filterComboBox->count(); ++i)
			ccs.filterComboBox->addItem(filterComboBox->itemText(i));

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

		label = new QLabel(this);
		label->setText(discretenessLabel->text());
		layout->addWidget(label, 3, 0);
		layout->addWidget(ccs.discretenessComboBox, 3, 1);

		label = new QLabel(this);
		label->setText(filterLabel->text());
		layout->addWidget(label, 4, 0);
		layout->addWidget(ccs.filterComboBox, 4, 1);

		channelControls.append(ccs);
		channelsTabWidget->addTab(tab, QString::number(channelControls.size()));

		connect(ccs.inputRangeComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_inputRangeComboBox_currentIndexChanged(int)));
		connect(ccs.resolutionSpinBox, SIGNAL(valueChanged(int)),
			this, SLOT(on_resolutionSpinBox_valueChanged(int)));
		connect(ccs.discretenessComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_discretenessComboBox_currentIndexChanged(int)));
		connect(ccs.filterComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_filterComboBox_currentIndexChanged(int)));
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
	updateStepSize();
}

void DS2450SettingsDialog::ChannelControlSet::on_inputRangeComboBox_currentIndexChanged()
{
	m_device->setRange(m_channel, DeviceDS2450::VoltageRange(inputRangeComboBox->currentIndex()));
	updateStepSize();
}

void DS2450SettingsDialog::ChannelControlSet::on_discretenessComboBox_currentIndexChanged()
{
	m_device->setDiscreteness(m_channel, discreteness[discretenessComboBox->currentIndex()]);
}

void DS2450SettingsDialog::ChannelControlSet::on_filterComboBox_currentIndexChanged()
{
	m_device->setFilterType(m_channel, DeviceDS2450::FilterType(filterComboBox->currentIndex()));
}

void DS2450SettingsDialog::ChannelControlSet::updateStepSize()
{
	stepSizeLabel->setText(m_device->milliVoltsText(m_device->milliVoltsStep(m_channel)));
}
