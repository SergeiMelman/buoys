#include "client.h"
#include "server.h"
#include <QTcpSocket>
#include <QtNetwork>
#include <QStandardItem>
#include <QStringList>
#include <QStringListModel>
#include <QSqlError>
#include <QMutexLocker>

Client::Client(qintptr socketDescriptor_, int pos_, QStringListModel* specialListModel_,
			   QStandardItemModel* buoysModel_, QString servString_, QMutex* mutex_, QObject* parent)
	: QObject(parent)
	, socketDescriptor(socketDescriptor_)
	, pos(pos_)
	, specialListModel(specialListModel_)
	, buoysModel(buoysModel_)
	, servString(servString_)
	, mutex(mutex_)
{
}

Client::~Client()
{
}

QString Client::getCurrentDataTime() const
{
	return QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd HH:mm:ss"));
}

void Client::sendToDB()
{
	const QString queryStr = "INSERT INTO bouys"
							 "(name, address, request, time, answer) "
							 "VALUES ('%1', '%2', '%3', '%4', '%5');";
	const QString queryStrFinal =
			queryStr.arg(buoyName, //name
						 buoyAddress, // address
						 requestStr, // Request
						 respondTime, // Time
						 respondStr); // Answer
	emit toDB(queryStrFinal);
	//emit toBuoysModel(pos, POS_STATUS, rezMes);
}

void Client::start()
{
	clientSocket = new QTcpSocket(this);
	if (!clientSocket->setSocketDescriptor(socketDescriptor))
	{
		emit toLog("Socket error: " + clientSocket->error(), 1);
		emit finished();
		this->deleteLater();
		return;
	}

	buoyAddress = clientSocket->peerAddress().toString().split(':').last();
	emit toLog(tr("Connected: ") + buoyAddress + "\n", 0);

	connect(clientSocket, SIGNAL(readyRead()),
			this, SLOT(incomingDataClientHandler()));
	connect(clientSocket, SIGNAL(disconnected()), // при дисконнекте
			this, SLOT(clientDisconnected())); // запостить в лог и самоубиться

	// на все общение отводится 10 сек (чтоб отсекать случайных прохожих)
	// это приведет к дропу клиента и к уничтожению сокета и Client
	QTimer::singleShot(10 * 1000, clientSocket, &QTcpSocket::disconnectFromHost);

	// теперь сохранить адрес
	emit toBuoysModel(pos, POS_ADDR, buoyAddress);
	buoyName = "-";
	emit toBuoysModel(pos, POS_BUOY_NAME, buoyName);
	requestStr = "NAME";
	emit toBuoysModel(pos, POS_REQUEST_STR, requestStr);
	emit toBuoysModel(pos, POS_REQUEST_TIME, getCurrentDataTime());
	respondStr = "-";
	emit toBuoysModel(pos, POS_RESPOND_STR, respondStr);
	respondTime = "-";
	emit toBuoysModel(pos, POS_RESPOND_TIME, respondTime);
	emit toBuoysModel(pos, POS_STATUS, tr("request:") + requestStr);
	// Handshake
	clientSocket->write(requestStr); // попросить имя буя
}

void Client::incomingDataClientHandler()
{
	respondTime = getCurrentDataTime();
	emit toBuoysModel(pos, POS_RESPOND_TIME, respondTime);

	if(clientSocket->bytesAvailable() < 4) // minimum!
	{
		return;
	}

	const QByteArray head = clientSocket->peek(4);

	if(       head == "NAME" && clientSocket->bytesAvailable() == 4 + 10)
	{
		QMutexLocker lock(mutex);
		// пришло имя: проверить в списке спец и отправить запрос
		respondStr = clientSocket->readAll();
		emit toBuoysModel(pos, POS_RESPOND_STR, respondStr);
		buoyName = respondStr.right(10);
		emit toBuoysModel(pos, POS_BUOY_NAME, buoyName);

		requestStr = "DATA";
		const QStringList spec = specialListModel->stringList();
		const QRegularExpression re(buoyName + "|" + buoyAddress);
		const int index = spec.indexOf(re); // поищем по имени или адресу

		if(index != -1)
		{
			// Есть в списке особых. Отправить SERV и удалить из списка
			requestStr = servString.toLatin1();
			specialListModel->removeRow(index);
		}

		clientSocket->write(requestStr);
		emit toBuoysModel(pos, POS_REQUEST_STR, requestStr);
		emit toBuoysModel(pos, POS_REQUEST_TIME, getCurrentDataTime());
		emit toBuoysModel(pos, POS_STATUS, tr("Requested:") + requestStr);
	} else if(head == "DATA" && clientSocket->bytesAvailable() == 4 + 150) // 150 - размер буфера в который упакованы данные с буя
	{
		respondStr = clientSocket->readAll();
		emit toBuoysModel(pos, POS_RESPOND_STR, respondStr);
		emit toBuoysModel(pos, POS_STATUS, tr("DATA:Ok"));
		// получили ответ отключиться
		clientSocket->disconnectFromHost();
	} else if(head == "SERV" && clientSocket->bytesAvailable() == 4 + 21)
	{
		respondStr = clientSocket->readAll();
		emit toBuoysModel(pos, POS_RESPOND_STR, respondStr);
		emit toBuoysModel(pos, POS_STATUS, tr("SERV:Ok"));
		// получили ответ отключиться
		clientSocket->disconnectFromHost();
	} else
	{
		const int maxLen = 300;
		respondStr = "UNC:" + clientSocket->peek(maxLen); // посмотреть что наотвечал буй
		emit toBuoysModel(pos, POS_RESPOND_STR, respondStr);
		emit toBuoysModel(pos, POS_STATUS, tr("Receiving"));
		if(respondStr.size() >= maxLen)
		{
			// такой ответ слишком длинный, что то не так, пора отключаться
			emit toBuoysModel(pos, POS_STATUS, tr("Data size error"));
			// отключиться
			clientSocket->disconnectFromHost();
		}
	}
}

void Client::clientDisconnected()
{
	// занести в БД что насобирали перед смертью
	sendToDB();

	emit toLog(buoyAddress + tr("-disconnected"), 0);
	emit finished();
	this->deleteLater();
}
