#include <QPlainTextEdit>
#include <QTcpSocket>
#include <QtNetwork>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtWidgets>

#include "retranslatormainwindow.h"

static QSettings settings(QLatin1String("modemretranslator.ini"),
						  QSettings::IniFormat);

RetranslatorMainWindow::RetranslatorMainWindow(QWidget* parent)
	: QDialog(parent)
	, logWindow(new QPlainTextEdit)
	, networkSession(nullptr)
	, tcpServer(nullptr)
	, serverPort(8989)
	, tcpClient(nullptr)
	, modem(new Modem(this))
{
	resize(400, 300);
	setWindowTitle(tr("Modem retranslator tool"));

	connectModemHandlers(modem, this);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	logWindow->setMaximumBlockCount(100);
	mainLayout->addWidget(logWindow);

	QPushButton* quitButton;
	quitButton = new QPushButton(tr("Quit"));
	connect(quitButton, SIGNAL(clicked()), SLOT(close()));
	mainLayout->addWidget(quitButton);

	const QLatin1String ms("modem/settings");
	const QString modemInitString = settings.value(ms).toString();

	ModemSettings p;
	// если настроек вообще нет, то создать с деф. настройками.
	if(modemInitString.isEmpty())
	{
		settings.setValue(ms, p.toString());
	}

	if(p.fromString(modemInitString))
	{
		// если v.isNull() то происходит неочивидное: настройка p по умолчанию
		modem->setSettings(p);
	}
	else
	{
		addMessageToLogWindow(tr("Wrong modem setup string in INI file."));
	}


	serverPort = static_cast<quint16>(settings.value("server/port", 0).toInt());

	if(serverPort == 0)
	{
		settings.setValue("server/port", "8989");
	}

	QNetworkConfigurationManager manager;

	if(manager.capabilities() &
			QNetworkConfigurationManager::NetworkSessionRequired)
	{
		// Get saved network configuration
		const QString id =
			settings.value(QLatin1String("QtNetwork/Configuration")).toString();
		// If the saved network configuration is not currently discovered use the
		// system default
		QNetworkConfiguration config = manager.configurationFromIdentifier(id);

		if((config.state() & QNetworkConfiguration::Discovered) !=
				QNetworkConfiguration::Discovered)
		{
			config = manager.defaultConfiguration();
		}

		networkSession = new QNetworkSession(config, this);
		connect(networkSession, SIGNAL(opened()), this,
				SLOT(networkSessionOpened()));
		addMessageToLogWindow(tr("Opening network session."));
		networkSession->open();
	}
	else
	{
		networkSessionOpened();
	}
}

void
RetranslatorMainWindow::addMessageToLogWindow(const QString& s)
{
	logWindow->appendPlainText(s);
	QScrollBar* bar = logWindow->verticalScrollBar();
	bar->setValue(bar->maximum());
}

void
RetranslatorMainWindow::sendToClient(const QByteArray& d)
{
	if(tcpClient != nullptr)
	{
		qint64 sz = tcpClient->write(d);
		addMessageToLogWindow(QString("Send %1b\n").arg(sz));
	}
}

void
RetranslatorMainWindow::sendToClient(const QString& s)
{
	sendToClient(s.toLocal8Bit());
}

void
RetranslatorMainWindow::networkSessionOpened()
{
	// Save the used configuration
	if(networkSession)
	{
		QNetworkConfiguration config = networkSession->configuration();
		QString id;

		if(config.type() == QNetworkConfiguration::UserChoice)
		{
			id = networkSession
				 ->sessionProperty(QLatin1String("UserChoiceConfiguration"))
				 .toString();
		}
		else
		{
			id = config.identifier();
		}

		settings.setValue(QLatin1String("QtNetwork/Configuration"), id);
	}

	tcpServer = new QTcpServer(this);
	connect(tcpServer, SIGNAL(newConnection()), SLOT(serverClientConnected()));
	connect(tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),
			SLOT(serverAcceptError(QAbstractSocket::SocketError)));
	serverStartListen();

	for(const QHostAddress& addr : QNetworkInterface::allAddresses())
	{
		if(addr.toIPv4Address())
		{
			addMessageToLogWindow(tr("The server is running on IP: %1 port: %2\n")
								  .arg(addr.toString())
								  .arg(tcpServer->serverPort()));
		}
	}
}

void
RetranslatorMainWindow::serverAcceptError(QAbstractSocket::SocketError)
{
	addMessageToLogWindow(tcpServer->errorString());
}

void
RetranslatorMainWindow::serverClientConnected()
{
	QByteArray block;
	QTextStream out(&block);
	out << tr("Connected to server. Ports list=");

	for(const QSerialPortInfo& info : QSerialPortInfo::availablePorts())
	{
		out << "\n"
			<< tr(" portName=") << info.portName() << tr(" description=")
			<< info.description().toLatin1() << tr(" manufacturer=")
			<< info.manufacturer().toLocal8Bit() << tr(" serialNumber=")
			<< info.serialNumber() << tr(" systemLocation=")
			<< info.systemLocation() << tr(" vendorIdentifier=")
			<< (info.vendorIdentifier()
				? QString::number(info.vendorIdentifier(), 16)
				: tr("N/A"))
			<< tr(" productIdentifier=")
			<< (info.productIdentifier()
				? QString::number(info.productIdentifier(), 16)
				: tr("N/A")) << "\n"
			<< ((modem->getStatus() == DISCONNECTED) ? tr("Disconnected")
				: tr("Connected")) << "\n";
	}

	out.flush();

	tcpClient = tcpServer->nextPendingConnection();
	tcpServer->close(); // stop listening
	addMessageToLogWindow(tr("Connected: ") +
						  tcpClient->peerAddress().toString() + "\n");
	connect(tcpClient, SIGNAL(readyRead()), SLOT(incomingDataSocketHandler()));
	connect(tcpClient, SIGNAL(disconnected()), modem, SLOT(disconnectModem()));
	connect(tcpClient, SIGNAL(disconnected()), tcpClient, SLOT(deleteLater()));
	connect(tcpClient, SIGNAL(disconnected()), SLOT(serverStartListen()));
	sendToClient(block); // say hello
	addMessageToLogWindow(block);
}

void
RetranslatorMainWindow::serverStartListen()
{
	tcpClient = nullptr;

	if(!tcpServer->listen(QHostAddress::Any, serverPort))
	{
		QMessageBox::critical(
			this, tr("Retranslator"),
			tr("Unable to start the server: %1.").arg(tcpServer->errorString()));
		close();
		return;
	}

	addMessageToLogWindow(tr("Listening on port ") + QString::number(serverPort) +
						  "\n");
}

void
RetranslatorMainWindow::incomingDataSocketHandler()
{
	while(tcpClient->bytesAvailable())
	{
		const QByteArray packet = tcpClient->readAll();
		if(modem->getStatus() != DISCONNECTED)
		{
			addMessageToLogWindow(QLatin1String("+"));
			// если подключен модем, то тупо передавать
			modem->writeDataModem(packet);
		}
		else
		{
			addMessageToLogWindow(packet);
			// если не подключен, то интерпретировать комманды
			processPacket(packet);
		}
	}
}

void
RetranslatorMainWindow::processPacket(QByteArray packet)
{
	// Дисконнект выполнить невозможно, ибо переходим в режим ретранслятора.
	// Можно отключиться от ретранслятора, он разорвет связь с модемом.
	if(packet.left(5) == "setup")
	{
		addMessageToLogWindow(tr("Trying to setup modem.\n"));
		QString stp = packet.right(packet.size() - 5 - 1);
		ModemSettings p;

		if(p.fromString(stp))
		{
			addMessageToLogWindow(tr("Setup complete.\n"));
			modem->setSettings(p);
		}
		else
		{
			errorModemHandler(tr("Wrong setup sequence.\n"));
		}
	}
	else if(packet.left(3) == "cls")
	{
		logWindow->clear();
	}
	else if(packet.left(7) == "connect")
	{
		addMessageToLogWindow(tr("Trying to connect modem\n"));
		modem->connectModem();
	}
	else if(packet.left(4) == "data")
	{
		QByteArray rnd(1024 * 1024 * 10, 0); // 20Mb мусора :)

		for(int i = rnd.size() - 1; i >= 0; i--)
		{
			rnd[i] = rand();
		}

		addMessageToLogWindow(tr("Sending a garbage.\n") +
							  QString::number(rnd.size()));
		sendToClient(rnd);
	}
	else
	{
		sendToClient(
			QByteArray("I can't understand you! "
					   "Need one of: setup xxx, cls, connect, data"));
	}
}

void
RetranslatorMainWindow::errorModemHandler(const QString& errorString)
{
	addMessageToLogWindow(errorString);
	sendToClient(errorString);
}

void
RetranslatorMainWindow::infoModemHandler(const QString& infoString)
{
	addMessageToLogWindow(infoString);
	sendToClient(infoString);
}

void
RetranslatorMainWindow::incomingDataModemHandler(QByteArray packet)
{
	addMessageToLogWindow("-");
	sendToClient(packet);
}

void
RetranslatorMainWindow::disconnectedModemHandler()
{
	static const QString s = tr("Modem disconnected.\n");
	addMessageToLogWindow(s);
	sendToClient(s);
}

void
RetranslatorMainWindow::connectedModemHandler()
{
	static const QString s = tr("Modem connected.\n");
	addMessageToLogWindow(s);
	sendToClient(s);
}
