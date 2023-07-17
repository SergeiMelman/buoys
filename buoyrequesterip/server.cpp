#include "server.h"

#include <QStandardItem>
#include <QStringList>
#include <QStringListModel>
#include <QTcpSocket>
#include <QtNetwork>
#include <QMessageBox>

#include "client.h"
#include "mytcpserver.h"

Server::Server(QObject *parent)
	: QObject(parent)
	, buoysModel(new QStandardItemModel(this))
	, specialListModel(new QStringListModel(this))
	, tcpServer(new MyTcpServer(this))
	, serverPort(8990)
	, clientsNum(0)
{
	// установить гориз заголовки таблицы
	buoysModel->setHorizontalHeaderLabels({tr("Buoy Name"), tr("Address"),
										   tr("Request string"), tr("Request time"),
										   tr("Respond string"), tr("Respond time"),
										   tr("Status")});
	// задать пару примеров в список буев со спецзаданием
	specialListModel->setStringList({"Any Buoy Name",
										 "10.12.44.154 - or buoy IP",
										 "Other one",
										 "Other two"});

	connect(tcpServer, SIGNAL(incomingConnectionSignal(qintptr)),
			SLOT(incomingConnection(qintptr)));

	connect(tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),
			SLOT(serverAcceptError(QAbstractSocket::SocketError)));

	connect(specialListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
			this, SLOT(specialListDataChanged(QModelIndex,QModelIndex,QVector<int>)));

	dbase = new DataBase;
	QThread* thread = new QThread;
	dbase->moveToThread(thread);
	connect(thread, SIGNAL(started()), dbase, SLOT(start()));
	connect(dbase, SIGNAL(toLog(QString,int)), SIGNAL(toLog(QString,int)));
	// в конце работы наступает смерть всех объектов MyTcpServer в том числе
	// а она вызывает смерть dbase
	connect(tcpServer, SIGNAL(destroyed(QObject*)), dbase, SLOT(deleteLater()));
	//смерть dbase останваливает работу thread
	connect(dbase, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
	//остановка thread приводит к самоуничтожению
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	// пуск Базы Данных!
	thread->start();
}

QStandardItemModel* Server::getBuoysModel() const
{
	return buoysModel;
}

QStringListModel* Server::getSpecialCommandsModel() const
{
	return specialListModel;
}

void Server::start(bool start_)
{
	if(!start_)
	{
		tcpServer->close();
		emit toLog(tr("Server stop"), 0);
	} else if(tcpServer->listen(QHostAddress::Any, serverPort))
	{
		emit toLog(tr("Listening on port %1").arg(serverPort), 0);
	} else
	{
		const QString message = tr("Unable to start the server on port %1."
								   "Error message: ").arg(serverPort) +
								tcpServer->errorString();
		emit toLog(message, 1);
		QMessageBox::critical(0, tr("Server"), message);
	}
	emit serverStatus(tcpServer->isListening());
}

quint16 Server::getServerPort() const
{
	return tcpServer->serverPort();
}

QString Server::getServerAddress() const
{
	QString rez;
	for(const QHostAddress& addr : QNetworkInterface::allAddresses())
	{
		if(addr.toIPv4Address())
		{
			rez += addr.toString() + " ";
		}
	}
	return rez;
}

void Server::serverAcceptError(QAbstractSocket::SocketError) const
{
	emit toLog(tr("Accepting error: ") + tcpServer->errorString(), 1);
}

void Server::incomingConnection(qintptr socketDescriptor)
{
	//	// кто то приконектился.
	//	// Создать объект Клиента.
	//	// Клиент сам позаботится об delete

	emit clientsNumChanged(++clientsNum);
	int rows = buoysModel->rowCount();
	buoysModel->setItem(rows, 0, new QStandardItem("---"));
	Client* client = new Client(socketDescriptor,
								rows,specialListModel,
								buoysModel, servString, &mutex);

	QThread* thread = new QThread(this);
	client->moveToThread(thread);
	connect(client, SIGNAL(toBuoysModel(int,int,QString)), SLOT(toBuoysModel(int,int,QString)));
	connect(client, SIGNAL(toDB(QString)), dbase, SLOT(toDB(QString)));
	connect(client, SIGNAL(toLog(QString,int)), SIGNAL(toLog(QString,int)));
	connect(client, &Client::finished, [this](){emit clientsNumChanged(--clientsNum);});// для уменьшения счетчика подключенных клиентов - лямбдареализация.
	connect(client, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(thread, SIGNAL(started()), client, SLOT(start()));
	thread->start();


}

void Server::setServString(const QString& str)
{
	if(servString != str)
	{
		servString = str;
		emit toLog("servString:" + str, 0);
	}
}

void Server::specialListDataChanged(QModelIndex i1, QModelIndex, QVector<int>) const
{
	const QVariant var = specialListModel->data(i1, Qt::EditRole);
	toLog(tr("Special list edited: ") + var.toString(), 0);
}

void Server::toBuoysModel(int row, int col, QString mes)
{
	buoysModel->setItem(row, col, new QStandardItem(mes));
}
