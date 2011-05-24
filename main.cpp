#include "OneWireTestMainWindow.h"
#include <QtGui>

int main(int argc, char *argv[])
{
	QTextCodec::setCodecForTr(QTextCodec::codecForName("Windows-1251"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("Windows-1251"));
	QApplication app(argc, argv);
	OneWireTestMainWindow w;
	w.showMaximized();
	return app.exec();
}
