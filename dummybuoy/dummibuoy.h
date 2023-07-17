#ifndef DUMMIBUOY_H
#define DUMMIBUOY_H

#include <QObject>
//#include "../modemtool/modem.h"
#include "../modemtool/modemi.h"

class Modem;

class Dummibuoy : public QObject, public ModemI
{
	Q_OBJECT
public:
	explicit Dummibuoy(QObject *parent = 0);

	bool connect(const QString& s);

signals:

private slots:
	void errorModemHandler(const QString& s) Q_DECL_OVERRIDE;
	void infoModemHandler(const QString& s) Q_DECL_OVERRIDE;
	void incomingDataModemHandler(QByteArray data) Q_DECL_OVERRIDE;
	void connectedModemHandler() Q_DECL_OVERRIDE;
	void disconnectedModemHandler() Q_DECL_OVERRIDE;
	void bytesWrittenModemHandler(qint64 bytes) Q_DECL_OVERRIDE;

	void ATO();

public slots:

private:
	Modem* modemDevice;
};

#endif // DUMMIBUOY_H
