#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
	Q_OBJECT
public:
	explicit Client(QString name_, QString host_, quint16 port_, QObject *parent = 0);

signals:

public slots:
	void start();
	void error();
	void connectedToHost();
    void disconnectedFromHost();
    void read();



private:
	QTcpSocket* socket;
	QString name;
	QString host;
	quint16 port;
};

#endif // CLIENT_H
