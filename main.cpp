#include "OneWireTestMainWindow.h"
#include <QtGui>
#include <QSettings>
#include "DeviceDS2450.h"

int main(int argc, char *argv[])
{
	QTextCodec::setCodecForTr(QTextCodec::codecForName("Windows-1251"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("Windows-1251"));
	QApplication app(argc, argv);
	QSettings settings("ctrl2ini.txt", QSettings::IniFormat);
	DeviceDS2450::SamplingSeriesLength = settings.value("samplingSeriesLength", DeviceDS2450::SamplingSeriesLength).toInt();
	OneWireTestMainWindow w(settings);
	w.showMaximized();
	return app.exec();
}
