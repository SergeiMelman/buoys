#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>

class MyTcpServer : public QTcpServer
{
	Q_OBJECT

public:
	MyTcpServer(QObject *parent = 0);

signals:
	void incomingConnectionSignal(qintptr socketDescriptor);

public:
	void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE;

};

#endif // MYTCPSERVER_H
