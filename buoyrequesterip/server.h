#ifndef SERVER_H
#define SERVER_H

#include <QMultiMap>
#include <QObject>
#include <QAbstractSocket>
#include <QMutex>
#include <QModelIndex>
#include "database.h"

enum
{
	POS_BUOY_NAME = 0,
	POS_ADDR,
	POS_REQUEST_STR,
	POS_REQUEST_TIME,
	POS_RESPOND_STR,
	POS_RESPOND_TIME,
	POS_STATUS
};

class QStandardItemModel;
class QStringListModel;
class MyTcpServer;
class QTcpSocket;
class QNetworkSession;

class Server : public QObject
{
	Q_OBJECT
public:
	explicit Server(QObject *parent = 0);

public:
	QStandardItemModel* getBuoysModel() const;
	QStringListModel* getSpecialCommandsModel() const;
	void start(bool start_);
	quint16 getServerPort() const;
	QString getServerAddress() const;

signals:
	void toLog(const QString& message, int level) const;
	void serverStatus(bool status) const;
	void clientsNumChanged(int num) const;
	void toDB(QString str);

public slots:
	void serverAcceptError(QAbstractSocket::SocketError) const;
	void incomingConnection(qintptr socketDescriptor);
	//void serverClientConnected();
	void setServString(const QString& str);
	void specialListDataChanged(QModelIndex i1, QModelIndex, QVector<int>) const;
	void toBuoysModel(int row, int col, QString mes);

private:
	QStandardItemModel* buoysModel;
	QStringListModel* specialListModel;
	QMutex mutex;

	QNetworkSession* networkSession;
	MyTcpServer* tcpServer;
	quint16 serverPort;
	int clientsNum;

	QString servString; // сервисная строка для буев из списка

	DataBase *dbase; // живет в отдельном потоке.


};

#endif // SERVER_H
