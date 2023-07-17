// Эмулятор буя. Остановк эмуляции по ctrl+c
// Client пытается установить соединение с сервером и обрабатывает 3 команды:
// NAME - отправляет свое имя
// DATA - отправляет 154 байт данных
// SERV - отправляет полученную строку.


#include <QCoreApplication>
#include <QTimer>
#include <QRegularExpression>
#include "client.h"
#include "data.h"

void startOne()
{
	static int i = 0;
	// сгенерировать случайное имя
	i%=10; // в пределах от Name000001 до Name000010
	const QString buoyName = QString("Name%1").arg(++i, 6, 10, QChar('0'));
	// создать клиента (имитатор буя)
	Client *client = new Client(buoyName,        // имя буя - генерится случайно
								"192.168.2.75",  // адрес сервера для отправки данных
								8990);           // порт сервера
	client->start(); // запустить имитацию сеанса связи буя с сервером

	// запустить имитацию еще раз через некоторое случайное время
	const int secondsLimit = 3; // новый сеанс запустится через 0..secondsLimit секунд
	// расчет случайной величины в интервале [0,secondsLimit]
	const int seconds = 1000 * secondsLimit * qrand()/RAND_MAX;
	// запуск таймера по истечении которого запустится еще одна имитация
	QTimer::singleShot(seconds, &startOne);
}

int main(int argc, char *argv[])
{
	/*
	// для теста упаковки/распаковки структуры данных в/из буфера
	Data d; //"F6FF9A9999999999F1BFC3F54840" = little endian
	//      //"FFF6BFF199999999999A4048F5C3" = big endian
	d.x = -1.1;
	d.y = 3.14;
	d.z = -10;
	char buf[1000]; memset(buf, 0, sizeof(buf));
	int sz = d.toString(buf);
	Data s;
	s.fromString(buf);
	*/

	QCoreApplication a(argc, argv);

	qInfo() << "To stop press ctrl+c";

	// сколько в среднем одновременных имитаций будет работать
	const int imitationsCount = 5;
	for(int i = 0; i < imitationsCount; ++i)
	{
		startOne();
	}

	return a.exec();
}
