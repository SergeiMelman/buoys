#include "scheduler.h"

#include <QStandardItem>
#include <QTimer>
#include <QSqlError>

#include "../modemtool/modem.h"

Scheduler::Scheduler(QStandardItemModel* buoysModel_,
					 Modem* modemDevice_,
					 QObject *parent)
	: QObject(parent)
	, buoysModel(buoysModel_)
	, modemDevice(modemDevice_)
	, globalRequestTimer(new QTimer(this))
	, currentRow(0)
	, incomingData()
	, stage3Count(0)
	, stage3Attempts(15)
	, endPhrase("ENDDATA")
	, dbase(QSqlDatabase::addDatabase("QSQLITE", "bouys"))
{
	dbase.setDatabaseName("buoys.sqlite");
	dbase.open();
	query = QSqlQuery(dbase);

	if(dbase.isOpen() && !query.exec("CREATE TABLE IF NOT EXISTS bouys ("
									 "name TEXT, "
									 "address TEXT, "
									 "request TEXT, "
									 "time, TEXT, "
									 "answer TEXT);"))
	{
		emit toLog(tr("Can't create a TABLE buoys"), 2);
	}

	connect(modemDevice, SIGNAL(disconnectedModem()),
			this, SLOT(stop()));
	connect(modemDevice, SIGNAL(incomingDataModem(QByteArray)),
			this, SLOT(incomingDataModemHandler(QByteArray)));
	connect(globalRequestTimer, SIGNAL(timeout()),
			this, SLOT(doRequestRound()));
}

bool
Scheduler::isDatabaseOk()
{
	return dbase.isOpen();
}

QString
Scheduler::getDatabaseLastError()
{
	return dbase.lastError().text();
}

void
Scheduler::setEndPhrase(const QString& value)
{
	emit toLog(tr("End phrase: ") + value, 2);
	endPhrase = value;
}

void
Scheduler::setStage3Attempts(int value)
{
	emit toLog(tr("Attempts count=%1").arg(value), 2);
	stage3Attempts = value;
}

void
Scheduler::start(int intervalMinutes)
{
	emit toLog(tr("Start requesting."), 2);

	if(modemDevice->getStatus() == DISCONNECTED)
	{
		return;
	}

	prepareModemDevice();
	globalRequestTimer->start(intervalMinutes * 1000 * 60);
}

void
Scheduler::stop()
{
	emit toLog(tr("Stop requesting."), 2);
	globalRequestTimer->stop();
}

void
Scheduler::prepareModemDevice()
{
	// работаем в режиме ATO
	// даже если мы уже и были в этом режиме то ничего страшного
	// принимающая сторона должна проигнорировать
	emit toLog("Prepare modem.", 2);
	modemDevice->writeDataModem("ATO\r");
}

void
Scheduler::incomingDataModemHandler(QByteArray data)
{
	incomingData += data;
}

void
Scheduler::setBuoysModel(int column, QString str)
{
	buoysModel->setItem(currentRow, column, new QStandardItem(str));
}

void
Scheduler::doRequestStage1()
{
	if(currentRow < buoysModel->rowCount())
	{
		// зафиксировать время запроса
		setBuoysModel(3, QTime::currentTime().toString());
		// сменить статус
		setBuoysModel(5, tr("Changing adress"));
		const QString address = buoysModel->item(currentRow, 1)->text();

		// cменить адрес
		incomingData.clear();
		modemDevice->writeDataModem("+++AT!AR" + address.toLatin1());
		// подождать и вызвать 2 этап
		QTimer::singleShot(1000, this, &Scheduler::doRequestStage2);
	}
	else
	{
		// прошли по всем буям. Запустить ожидание след раунда и выйти
		globalRequestTimer->start();
	}
}

void
Scheduler::doRequestStage2()
{
	//проверить результат смены адреса
	if(incomingData.right(2) == "OK")
	{
		// сменить статус
		setBuoysModel(5, tr("Requesting"));
		incomingData.clear();
		const QString sendString = buoysModel->item(currentRow, 2)->text();
		// послать реквест
		modemDevice->writeDataModem(sendString.toLatin1());
		// подождать и вызвать 3 этап
		stage3Count = 0;
		QTimer::singleShot(1000, this, &Scheduler::doRequestStage3);
	}
	else
	{
		finalizeRequest(tr("Can't setup remote buoy"));
	}
}

void
Scheduler::doRequestStage3()
{
	if(incomingData.right(endPhrase.length()) == endPhrase) // incomingData.size() == 150
	{
		const QString queryStr = "INSERT INTO bouys"
							 "(name, address, request, time, answer) "
							 "VALUES ('%1', %2, '%3', '%4', '%5');";
		const QString queryStrFinal =
				queryStr.arg(buoysModel->item(currentRow, 0)->text(), //name
							 buoysModel->item(currentRow, 1)->text(), // address
							 buoysModel->item(currentRow, 2)->text(), // Request string
							 QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd HH:mm:sss")),
							 incomingData);
		const bool rez = query.exec(queryStrFinal);
		if(rez)
		{
			finalizeRequest(tr("Ok"));
		}
		else
		{
			finalizeRequest(tr("Can't insert respond into SQLite database"));
		}

	} else
	{
		++stage3Count;
		setBuoysModel(5, tr("Requesting ") + QString::number(stage3Count));
		if(stage3Count < stage3Attempts)
		{
			// подождать и вызвать 3 этап еще раз
			QTimer::singleShot(1000, this, &Scheduler::doRequestStage3);
		}
		else
		{
			finalizeRequest(tr("Can't get respond"));
		}
	}
}

void
Scheduler::finalizeRequest(const QString& mess)
{
	const QString buoyName = buoysModel->item(currentRow, 0)->text();
	emit toLog(mess + ' ' + buoyName + ':' + QString::number(currentRow)
			   + tr(" with answer: ") + incomingData, 2);
	incomingData.clear();
	// зафиксировать время
	setBuoysModel(4, QTime::currentTime().toString());
	// зафиксировать результат
	setBuoysModel(5, mess);
	// перейти к опросу следующего буя
	++currentRow;
	QTimer::singleShot(1000, this, &Scheduler::doRequestStage1);
}

void
Scheduler::doRequestRound()
{
	emit toLog("Start request round", 2);
	globalRequestTimer->stop();

	currentRow = 0;
	QTimer::singleShot(1000, this, &Scheduler::doRequestStage1);
}
