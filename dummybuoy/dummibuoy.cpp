#include "dummibuoy.h"

#include <iostream>

#include "../modemtool/modem.h"

Dummibuoy::Dummibuoy(QObject* parent)
	: QObject(parent)
	, modemDevice(new Modem(this))
{
	// обработчики сигналов модема
	connectModemHandlers(modemDevice, this);
}

bool Dummibuoy::connect(const QString& s)
{
	const ModemSettings set(s + QLatin1String(";19200;8;0;1;0;127.0.0.1;8989;1;"));
	modemDevice->setSettings(set);
	modemDevice->connectModem();

	if(modemDevice->getStatus() == DISCONNECTED)
	{
		return false;
	}

	ATO(); // всегда работать в режиме данных
	return true;
}

void Dummibuoy::errorModemHandler(const QString& s)
{
	std::cout << "Error: " << s.toStdString() << std::endl;
}

void Dummibuoy::infoModemHandler(const QString& s)
{
	std::cout << "Info: " << s.toStdString() << std::endl;
}

void Dummibuoy::incomingDataModemHandler(QByteArray data)
{
	if     (data.left(3) == "---")
	{
		// пришла некая команда с удаленной машины
		//const QByteArray cmd = data.right(data.length() - 3);
		const QByteArray cmd = data.mid(3);
		const QList<QByteArray> ls = cmd.split(':');

		if(ls.at(0) == "GBG")
		{
			// если это команда "МУСОР"
			// обратно выслать пакет мусора заданного размера (по умолч. 1000б)
			const int szG = (ls.size() < 2) ? 1000 : ls.at(1).trimmed().toInt();
			QByteArray pac(szG, 0);

			for(int i = 0; i < szG; ++i)
			{
				//заполнить мусор. пусть ранд дает больше чем char - нужен мусор
				pac[i] = rand();
			}

			modemDevice->writeDataModem(pac);
		}
		else if(ls.at(0) == "DATA")
		{
			modemDevice->writeDataModem("Received command DATA, strating to "
										"send collected data from sensors...\r\n");

		}
		else
		{
			// иначе превратить ее в команду для лок. модема и послать
			data.replace('-', '+');
			modemDevice->writeDataModem(data);
		}
	}
	else if(data.left(5) == "+++AT")
	{
		// если пришел ответ от локального модема, то превратить ее в --- и
		// отослать удаленной машине
		data.replace('+', '-');
		modemDevice->writeDataModem(data);
	}
	else if(data.left(6) == "RECVIM")
	{
		// если в режиме +++ получили INSTANT MESSAGE то перейти в режим ATO
		ATO();
	}
	else
	{

		// не было никаких команд, тогда просто переслать пакет данных
		// Это может быть частью длинного ответа от модема буя
		// или пакет пришедший по аудиоканалу. Отправить его обратно (эхо).
		modemDevice->writeDataModem(data);
	}
}

void Dummibuoy::connectedModemHandler()
{
	std::cout << "Connected!" << std::endl;
}

void Dummibuoy::disconnectedModemHandler()
{
	std::cout << "Disconnected!" << std::endl;
	QCoreApplication::quit(); // нет коннекта, делать больше нечего
}

void Dummibuoy::bytesWrittenModemHandler(qint64 /*bytes*/)
{
	//
}

void Dummibuoy::ATO()
{
	// Команда ATO
	modemDevice->writeDataModem("ATO\r");
}
