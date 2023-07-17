#include <QCoreApplication>
#include <iostream>
#include "dummibuoy.h"

void hello()
{
	qInfo() << "Part of modemtool";
	qInfo() << "Dummibuoy";
	qInfo() << "Immitation of real buoy.";
	qInfo() << "You can use AT commands on remote with prefix ---";
	qInfo() << "It will be executed on local modem and answer will back.";
	qInfo() << "If no command responded incoming data just returned back.";
	qInfo() << "Known commands:";
	qInfo() << "---GBG:n   = send back n bytes of garbage.";
	qInfo() << "---DATA    = send back collected data from sensors.";
	qInfo() << "Modem initialized (default) with: com7;19200;8;0;1;0;127.0.0.1;8989;1;";
	qInfo() << "You can pass COM name or all string as 1 parameter.";
	qInfo() << "Example: dummibuoy com6";
	qInfo() << "Enjoy.";
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	Dummibuoy buoy;
	// вывести привет+хелп
	hello();

	if(buoy.connect((argc > 1) ? argv[1] : "com7"))
	{
		// Конект удался. Запустить обработчик событий.
		return a.exec();
	}

	return 0;
}
