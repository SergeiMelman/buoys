#ifndef MODEM_H
#define MODEM_H

#include "settings.h"
#include <QObject>
#include <QTcpSocket>

class QStringList;
class QString;
class QIODevice;

class Modem : public QObject
{
	Q_OBJECT
public:  // TYPES
private: // VARIABLES
	ModemSettings settings;
	union
	{
		QIODevice* device;
		QTcpSocket* socket;
		QSerialPort* serial;
	};

public: // CONSTRUCTORS/DESTRUCTORS
	explicit Modem(QObject* parent = 0);
	~Modem();

public: // ROUTINES
	STATUSTYPE getStatus();
	bool getMode() const;
	qint64 bytesToWrite() const;
	const ModemSettings& getSettings() const;
	void setSettings(const ModemSettings& value);
	void flush();

private:
	void connectIP(QTcpSocket* s = nullptr);
	void connectCom();

signals:
	void errorModem(const QString& s);
	void infoModem(const QString& s);
	void incomingDataModem(QByteArray data);
	void connectedModem();
	void disconnectedModem();
	void bytesWritten(qint64);

public slots:
	void connectModemEmulateIp(QTcpSocket* s);
	void connectModem();
	void disconnectModem();
	void writeDataModem(QByteArray data);

private slots:
	void readyReadModem();

	void handleSerialError(QSerialPort::SerialPortError error);
	void handleSocketError(QAbstractSocket::SocketError error);

private:
};

#endif // MODEM_H
