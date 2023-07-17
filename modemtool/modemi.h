#ifndef MODEMI_H
#define MODEMI_H

#include "modem.h"

class ModemI
{
public: // slots
	virtual void errorModemHandler(const QString &s) = 0;
	virtual void infoModemHandler(const QString &s) = 0;
	virtual void incomingDataModemHandler(QByteArray data) = 0;
	virtual void connectedModemHandler() = 0;
	virtual void disconnectedModemHandler() = 0;
	virtual void bytesWrittenModemHandler(qint64 ) = 0;
public:
	void connectModemHandlers(QObject* modem, QObject* handler)
	{
		QObject::connect(modem, SIGNAL(errorModem(const QString&)),
						 handler, SLOT(errorModemHandler(const QString&)));
		QObject::connect(modem, SIGNAL(infoModem(const QString&)),
						 handler, SLOT(infoModemHandler(const QString&)));
		QObject::connect(modem, SIGNAL(incomingDataModem(QByteArray)),
						 handler, SLOT(incomingDataModemHandler(QByteArray)));
		QObject::connect(modem, SIGNAL(connectedModem()),
						 handler, SLOT(connectedModemHandler()));
		QObject::connect(modem, SIGNAL(disconnectedModem()),
						 handler, SLOT(disconnectedModemHandler()));
		QObject::connect(modem, SIGNAL(bytesWritten(qint64)),
						 handler, SLOT(bytesWrittenModemHandler(qint64)));
	}
};

#endif // MODEMI_H
