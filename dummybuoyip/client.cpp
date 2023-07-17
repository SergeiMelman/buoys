#include "client.h"

#include "data.h"

#include <QDateTime>

Client::Client(QString name_, QString host_, quint16 port_, QObject *parent)
	: QObject(parent)
	, socket(new QTcpSocket(this))
	, name(name_)
	, host(host_)
	, port(port_)
{
    // без коментариев.
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(error()));
	connect(socket, SIGNAL(connected()),
			this, SLOT(connectedToHost()));
	connect(socket, SIGNAL(readyRead()),
			this, SLOT(read()));
	connect(socket, SIGNAL(disconnected()),
			this, SLOT(disconnectedFromHost()));
}

// запустить процесс связи с сервером
void Client::start()
{
	qInfo() << name << " is trying to connect.\n";
	socket->connectToHost(host, port);
}

// слоты обработки сигналов от сокета
void Client::error()
{
	qInfo() << name << ": error: " << socket->errorString() << "\n";
	this->deleteLater();
}

void Client::connectedToHost()
{
	qInfo() << name << " is connected to host.\n";
}

void Client::disconnectedFromHost()
{
    qInfo() << name << ": disconnected\n";
    this->deleteLater();
}

// getBuoy создает, хранит и возвращает виртуальные данные от виртуального буя
// если name еще нет в списке создает буй с таким именем и отдает
// если есть то "немного подкручивает" показания и отдает
Data getBuoy(QString name)
{
    static QMap<QString, Data> buoys; // так делать нельзя! но хочется для скорости
	bool contains = buoys.contains(name);
	Data& buoy = buoys[name];
	QByteArray time = QDateTime::currentDateTime().
					  toString(QLatin1String("yyyy-MM-dd HH:mm:ss")).
					  toLatin1();
    memcpy(buoy.datetime, time.data(), 20); // time.data() всегда оканчивается \0

	if(contains)
	{
        // небольшой дрейф показаний буя
        buoy.bat_v -= 0.1;
		buoy.state = (qrand() < (RAND_MAX/20)) ? 1 : 0; // с нек вероят 1 к 20 ошибка
		buoy.sensor1 += 1.0 * (2.0 * qrand()/RAND_MAX - 1.0); // небольшой дрейф
		buoy.sensor2 += 1.5 * (2.0 * qrand()/RAND_MAX - 1.0); // небольшой дрейф
		buoy.sensor3 += 2.5 * (2.0 * qrand()/RAND_MAX - 1.0); // небольшой дрейф

	} else
	{
        buoy.lon =  180.0 * (2.0 * qrand()/RAND_MAX - 1.0); // -180 < x < 180
        buoy.lat =  90.0 * (2.0 * qrand()/RAND_MAX - 1.0); // -90 < y < 90
		buoy.buoy_id = 0;
		buoy.state = 0;
		buoy.bat_v = 12 + (2.0 * qrand()/RAND_MAX - 1.0);
		buoy.sensor1 = 100 + 10 * (2.0 * qrand()/RAND_MAX - 1.0);
		buoy.sensor2 = -100 - 105 * (2.0 * qrand()/RAND_MAX - 1.0);
		buoy.sensor3 = 25 + 25 * (2.0 * qrand()/RAND_MAX - 1.0);
		buoy.bitrate = 100;
		buoy.RSSI = 200;
	}
	return buoy;
}

void Client::read()
{
    if(socket->bytesAvailable() < 4) // заголовок команды минимум 4 байта
	{
        return; // ждем
	}

	QByteArray socketData = socket->peek(4);
    if(socketData == "NAME") // отреагировать на команду сервера NAME
	{
		qInfo() << name << ": " << socket->read(4);
		socket->write("NAME" + name.toLatin1());
    } else if(socketData == "DATA")  // отреагировать на команду сервера DATA
	{
		qInfo() << name << ": " << socket->read(4);
		char buf[1000]; // с запасом
        /*int sz = */getBuoy(name).toString(buf); // sz = 150
		socket->write("DATA" + QByteArray(buf, 150));
    } else if(socketData == "SERV")  // отреагировать на команду сервера SERV
	{
        if(socket->bytesAvailable() < 25) // длина команды SERV от сервера 25 байт
		{
            return; // ждем.
		}
		socketData = socket->read(25);
		qInfo() << name << ": " << socketData;
        socket->write(socketData); // отправить все что получили.
    } else
    {
        // сервер прислал ерунду...
        qInfo() << name << ": got mood from server. disconnecting\n";
        socket->disconnectFromHost();
    }
}

