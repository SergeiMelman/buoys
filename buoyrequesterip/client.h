#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QMutex>

class QStandardItemModel;
class QStringListModel;
class QTcpSocket;

class Client : public QObject
{
	Q_OBJECT
public:
	explicit Client(qintptr socketDescriptor_,
					int pos_,
					QStringListModel* specialListModel_,
					QStandardItemModel* buoysModel_,
					QString servString_,
					QMutex* mutex_,
					QObject *parent = 0);
	~Client();

private:
	QString getCurrentDataTime() const;
	void sendToDB();

signals:
	void toBuoysModel(int row, int col, QString);
	void toDB(QString);
	void toLog(const QString& mes, int level) const;
	void finished() const;

public slots:
	void start();
	void incomingDataClientHandler();
	void clientDisconnected();

private:
	qintptr socketDescriptor;
	QTcpSocket* clientSocket;
	int pos;
	QStringListModel* specialListModel;
	QStandardItemModel* buoysModel;
	QString servString; // сервисная строка для буев из списка

	QString buoyName;
	QString buoyAddress;
	QByteArray requestStr;
	QByteArray respondStr;
	QString respondTime;

	QMutex* mutex;
};

#endif // CLIENT_H
