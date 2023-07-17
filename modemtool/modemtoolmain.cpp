/* Приложение для конфигурирования подводных акустических модемов.
 * sergeymelman0@gmail.com
*/

#include "modemtoolmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ModemToolMainWindow w;
	w.show();

	return a.exec();
}
