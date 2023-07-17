/* Сервер для связи с буями
 * 1) нужен список вышедших на связь
 * - имя
 * - IP
 * - время выхода
 * - время ответа
 * - полученная строка
 * 2) список буев которым дать задание на связь с конфигуратором
*/

#include "buoyrequesteripmainwindow.h"
#include "server.h"
#include <QApplication>

#include "../modemtool/logger.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	Logger logger;

	Server server;
	QObject::connect(&server, SIGNAL(toLog(QString,int)), // из сервера
			&logger, SLOT(log(QString,int))); // в логгер

	MainWindow window(&server);
	QObject::connect(&window, SIGNAL(toLog(QString,int)), // из интерфейса
			&logger, SLOT(log(QString,int))); // в логгер

	window.show();

	return a.exec();
}
