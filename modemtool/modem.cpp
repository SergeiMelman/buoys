#include "modem.h"
#include <QByteArray>
#include <QSerialPort>
#include <QStringList>
#include <QTCPSocket>

Modem::Modem(QObject* parent)
	: QObject(parent)
	, device(nullptr)
{
}

Modem::~Modem()
{
	if(device != nullptr)
	{
		disconnectModem();
	}
}

STATUSTYPE
Modem::getStatus()
{
	if(device == nullptr)
		return DISCONNECTED;

	if(!device->isOpen())
		return DISCONNECTED;

	return qobject_cast<QTcpSocket*>(device) ? IPCONNECTED : COMCONNECTED;
}

qint64
Modem::bytesToWrite() const
{
	if(device == nullptr)
	{
		return 0;
	}
	else
	{
		return device->bytesToWrite();
	}
}

const ModemSettings&
Modem::getSettings() const
{
	return settings;
}

void
Modem::setSettings(const ModemSettings& value)
{
	settings = value;
}

void
Modem::flush()
{
	if(qobject_cast<QTcpSocket*>(device))
	{
		socket->flush();
		return;
	}

	if(qobject_cast<QSerialPort*>(device))
	{
		serial->flush();
		return;
	}
}

void
Modem::connectCom()
{
	serial = new QSerialPort(this);

	connect(serial, SIGNAL(error(QSerialPort::SerialPortError)),
			SLOT(handleSerialError(QSerialPort::SerialPortError)));
	connect(serial, SIGNAL(readyRead()), SLOT(readyReadModem()));
	connect(serial, SIGNAL(bytesWritten(qint64)), SIGNAL(bytesWritten(qint64)));

	serial->setPortName(settings.name);
	serial->setBaudRate(settings.baudRate);
	serial->setDataBits(settings.dataBits);
	serial->setParity(settings.parity);
	serial->setStopBits(settings.stopBits);
	serial->setFlowControl(settings.flowControl);
	const QString inf =
		settings.name + tr(" at speed: ") + QString::number(settings.baudRate);

	if(serial->open(QIODevice::ReadWrite))
	{
		emit connectedModem();
		emit infoModem(tr("Modem: Connected to com:") + inf);
	}
	else
	{
		serial->close();
		emit disconnectedModem();
		emit infoModem(tr("Modem: Can't connect to com:") + inf);
		serial->deleteLater();
		serial = nullptr;
	}
}

void Modem::connectModemEmulateIp(QTcpSocket* s)
{
	// эмуляция установления связи с модемом через IP
	// нужно для входящих соединений от буев
	if(getStatus() != DISCONNECTED)
	{
		emit errorModem(tr("Can't accept incoming to emulate modem: Already connected!"));
		// закрыть входящее соединение
		s->close();
		s->deleteLater();
	}
	else
	{
		connectIP(s);
	}
}

void
Modem::connectIP(QTcpSocket* s)
{
	socket = (s == nullptr) ? new QTcpSocket(this) : s;

	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
			SLOT(handleSocketError(QAbstractSocket::SocketError)));
	connect(socket, SIGNAL(connected()), SIGNAL(connectedModem()));
	connect(socket, SIGNAL(disconnected()), SLOT(disconnectModem()));
	connect(socket, SIGNAL(readyRead()), SLOT(readyReadModem()));
	connect(socket, SIGNAL(bytesWritten(qint64)), SIGNAL(bytesWritten(qint64)));

	emit infoModem(tr("Modem: Attempt to connect to ") + settings.server + ":" +
				   QString::number(settings.port));
	socket->connectToHost(settings.server, settings.port);

	if(socket->waitForConnected(3000))
	{
		emit connectedModem();
		emit infoModem(tr("Modem: Connected to server."));
	}
	else
	{
		socket->close();
		emit disconnectedModem();
		emit infoModem(tr("Modem: Can't connect."));
		socket->deleteLater();
		socket = nullptr;
	}
}

void
Modem::connectModem()
{
	if(getStatus() != DISCONNECTED)
	{
		emit errorModem(tr("Modem: Already connected!"));
		return;
	}

	if(settings.type == ModemSettings::COMTYPE)
	{
		connectCom();
	}
	else if(settings.type == ModemSettings::IPTYPE)
	{
		connectIP();
	}
	else
	{
		emit errorModem(tr("Modem: Unknown connection type!"));
		return;
	}
}

void
Modem::disconnectModem()
{
	if(device != nullptr)
	{
		device->close();
		device->deleteLater(); // close connection and disconnect signals
		device = nullptr;
	}

	emit infoModem(tr("Modem: Disconnected."));
	emit disconnectedModem();
}

void
Modem::writeDataModem(QByteArray data)
{
	if(device == nullptr)
	{
		emit infoModem(tr("Modem: can't write while not connected."));
		return;
	}

	const qint64 len = device->write(data);
	flush();

	if(len != data.length())
	{
		emit errorModem(tr("Modem: Not all data was written!"));
	}
}

void
Modem::readyReadModem()
{
	Q_ASSERT(device != nullptr);

	do
	{
		emit incomingDataModem(device->readAll());
	}
	while(device->bytesAvailable());
}

void
Modem::handleSerialError(QSerialPort::SerialPortError error)
{
	static const QStringList serialErrors =
		QStringList()
		<< "No error occurred."
		<< "An error occurred while attempting to open an non-existing device."
		<< "An error occurred while attempting to open an already opened device by "
		"another process or a user not having enough permission and credentials "
		"to open."
		<< "An error occurred while attempting to open an already opened device in "
		"this object."
		<< "Parity error detected by the hardware while reading data."
		<< "Framing error detected by the hardware while reading data."
		<< "Break condition detected by the hardware on the input line."
		<< "An I/O error occurred while writing the data."
		<< "An I/O error occurred while reading the data."
		<< "An I/O error occurred when a resource becomes unavailable, e.g. when "
		"the "
		"device is unexpectedly removed from the system."
		<< "The requested device operation is not supported or prohibited by the "
		"running operating system."
		<< "An unidentified error occurred."
		<< "A timeout error occurred. This value was introduced in QtSerialPort "
		"5.2."
		<< "This error occurs when an operation is executed that can only be "
		"successfully performed if the device is open. This value was "
		"introduced "
		"in QtSerialPort 5.2.";

	if(error == QSerialPort::NoError)
	{
		return;
	}

	const int sz = serialErrors.size();
	const int i = ((error >= 0) && (error < sz)) ? error : 11;
	emit errorModem(tr("Modem: Serial port error: ") + serialErrors.at(i));

	if(error == QSerialPort::ResourceError)
	{
		disconnectModem();
	}
}

void
Modem::handleSocketError(QAbstractSocket::SocketError error)
{
	static const QStringList socketErrors =
		QStringList()
		<< "The connection was refused by the peer (or timed out)."
		<< "The remote host closed the connection. Note that the client socket "
		"(i.e., this socket) will be closed after the remote close notification "
		"has been sent."
		<< "The host address was not found."
		<< "The socket operation failed because the application lacked the "
		"required "
		"privileges."
		<< "The local system ran out of resources (e.g., too many sockets)."
		<< "The socket operation timed out."
		<< "The datagram was larger than the operating system's limit (which can "
		"be "
		"as low as 8192 bytes)."
		<< "An error occurred with the network (e.g., the network cable was "
		"accidentally plugged out)."
		<< "The address specified to QAbstractSocket::bind() is already in use and "
		"was set to be exclusive."
		<< "The address specified to QAbstractSocket::bind() does not belong to "
		"the "
		"host."
		<< "The requested socket operation is not supported by the local operating "
		"system (e.g., lack of IPv6 support)."
		<< "The socket is using a proxy, and the proxy requires authentication."
		<< "The SSL/TLS handshake failed, so the connection was closed (only used "
		"in "
		"QSslSocket)"
		<< "Used by QAbstractSocketEngine only, The last operation attempted has "
		"not "
		"finished yet (still in progress in the background)."
		<< "Could not contact the proxy server because the connection to that "
		"server "
		"was denied"
		<< "The connection to the proxy server was closed unexpectedly (before the "
		"connection to the final peer was established)"
		<< "The connection to the proxy server timed out or the proxy server "
		"stopped "
		"responding in the authentication phase."
		<< "The proxy address set with setProxy() (or the application proxy) was "
		"not "
		"found."
		<< "The connection negotiation with the proxy server failed, because the "
		"response from the proxy server could not be understood."
		<< "An operation was attempted while the socket was in a state that did "
		"not "
		"permit it."
		<< "The SSL library being used reported an internal error. This is "
		"probably "
		"the result of a bad installation or misconfiguration of the library."
		<< "Invalid data (certificate, key, cypher, etc.) was provided and its use "
		"resulted in an error in the SSL library."
		<< "A temporary error occurred (e.g., operation would block and socket is "
		"non-blocking)."
		<< "An unidentified error occurred.";

	const int sz = socketErrors.size();
	const int i = ((error >= 0) && (error < sz)) ? error : sz - 1;

	emit errorModem(sender()->objectName() + tr("Ip socket error: ") + socketErrors.at(i));
}
