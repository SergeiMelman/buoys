#include "buoyrequestermainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	BuoyrequesterMainWindow w;
	w.show();

	return a.exec();
}
