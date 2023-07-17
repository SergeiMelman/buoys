#include "modemtoolmainwindow.h"
#include "ui_modemtoolmainwindow.h"

#include "console.h"
#include "logger.h"
#include "settingsdialog.h"
#include <QDateTime>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTcpSocket>
#include <QTcpServer>
#include <QtNetwork>
#include <QTextStream>

QSettings settings(QLatin1String("modemtool.ini"), QSettings::IniFormat);
const QLatin1String setMod("modem/string");
const QLatin1String setCsDlg("modem/dialogString");
const QLatin1String setMainWnd("modemtool/mainwindow");

ModemToolMainWindow::ModemToolMainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, connectionSettingsDlg(new ConnectionSettingsDialog(this))
	, modemDevice(new Modem(this))
	, tcpServer(new QTcpServer(this))
	, serverPort(19348)
	, transmitBytesToModemTimer(new QTimer(this))
	, logger(new Logger(this))
	, bytesToModemBA()
	, bytesWrittenInt(0)
	, bytesRecievedInt(0)
	, modemCommandsModel(new QStandardItemModel(this))
	, waitingForRespondBool(false)
	, bytesRespondBA()
	, responseRssi(0)
	, responseIntegrity(0)
	, responseBL(0)
	, responseBR(0)
{
	pLogger = logger; // этот класс является предком логгера

	// без коментариев
	ui->setupUi(this);

	// соединить модель и представление Таблицы команд
	ui->tableView_modemCommand->setModel(modemCommandsModel);

	// все что набирается на консоли, сразу отправляется по соединению в модем
	connect(ui->console, SIGNAL(getData(QByteArray)),
			this, SLOT(sendDataToModem(QByteArray)));

	// настроить окна графиков CustomGraphs
	setupCustomGraph(ui->customPlot_bitrate, tr("In"), tr("Out"));
	setupCustomGraph(ui->customPlot_RSSI, tr("RSSI"), QLatin1String(""));
	setupCustomGraph(ui->customPlot_integrity, tr("Integrity"), QLatin1String(""));
	setupCustomGraph(ui->customPlot_BL_BR, tr("BL"), tr("BR"));
	// END  CustomGraphs

	// Работа с INI файлом настроек - желательно после настройки графиков
	loadINISettings();

	// таймер графиков
	QTimer* timerShowBitrate = new QTimer(this);
	connect(timerShowBitrate, SIGNAL(timeout()),
			this, SLOT(updateGraphs()));
	timerShowBitrate->start(1000); // раз в Х миллисекунд обновление

	// обработчики сигналов модема
	connectModemHandlers(modemDevice, this);

	// вкл и выкл соотв кнопок в интерфейсе - как если мы отключились от модема
	disconnectedModemHandler();

	// отправка данных идет порциями, по таймеру
	// настройка таймера отправки
	connect(transmitBytesToModemTimer, SIGNAL(timeout()),
			this, SLOT(transmitBytesToModemByTimer()));
	transmitBytesToModemTimer->setInterval(0);

	// настройка обработчика сообщений об ошибках, ведение лога и статусбара
	connect(modemDevice, SIGNAL(infoModem(QString)),
			logger, SLOT(log0(QString))); // в логгер из модема тип обишки 0
	connect(modemDevice, SIGNAL(errorModem(QString)),
			logger, SLOT(log1(QString))); // в логгер из модема тип обишки 1
	connect(this, SIGNAL(toLog(QString, int)),
			logger, SLOT(log(QString, int)));  // в логгер из гл окна
	connect(this, SIGNAL(toLog(QString, int)),
			this, SLOT(setStatusMessage(QString, int)));//в статусбар из гл окна

	// настроить сервер для входящих
	connect(tcpServer, SIGNAL(newConnection()), SLOT(serverClientConnected()));
	// финт! воспользуемся обработчиком ошибки от Модема
	connect(tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),
			modemDevice, SLOT(handleSocketError(QAbstractSocket::SocketError)));
	// после дисконекта сервер должен опять слушать входящие
	connect(modemDevice, SIGNAL(disconnectedModem()), SLOT(serverStartListen()));
	// При соединении Закрыть сервер только 1 буй может быть подключен
	connect(modemDevice, &Modem::connectedModem, tcpServer, &QTcpServer::close);
	serverStartListen();


	// заполнить список фалов *.cmi с командами для модема
	fillComboBoxCommandFiles();

	// обработчик загрузки файла *.cmi из списка
	connect(ui->comboBox_commandFiles, SIGNAL(currentIndexChanged(QString)),
			this, SLOT(loadCommands(QString)));

	// +++AT кнопки (настройка сигналов и слотов)
	setupPPPATButtonsConnections();

	// события СОХРАНИТЬ и ЗАГРУЗИТЬ сессию - текущую конф окон Modemtool
	connect(ui->actionLoad_state, SIGNAL(triggered(bool)),
			this, SLOT(loadINISettings()));
	connect(ui->actionSave_state, SIGNAL(triggered(bool)),
			this, SLOT(saveINISettings()));
}

ModemToolMainWindow::~ModemToolMainWindow()
{
	saveINISettings(); // перед окончанием работы скинуть сессию в файл.
	delete ui;
}

void
ModemToolMainWindow::fillComboBoxCommandFiles()
{
	// *.cmi - Command for Modem Input
	QStringList files;
	files << "" << QDir().entryList(QStringList() << "*.cmi",
									QDir::Files,
									QDir::Name);
	ui->comboBox_commandFiles->addItems(files);
}

void
ModemToolMainWindow::setupCustomGraph(QCustomPlot* cp,const QString& n1,
									   const QString& n2)
{
	cp->addGraph(); // 0 - incoming
	cp->graph(0)->setPen(QPen(Qt::blue));
	cp->graph(0)->setName(n1);
	cp->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 40)));
	if(!n2.isEmpty())
	{
		cp->addGraph(); // 1 - outgoing
		cp->graph(1)->setPen(QPen(Qt::green));
		cp->graph(1)->setName(n2);
		cp->graph(1)->setBrush(QBrush(QColor(0, 255, 0, 40)));
	}

	// cp->xAxis->setAutoTickCount(2); // old - version
	cp->xAxis->ticker()->setTickCount(2); // new ver
	cp->xAxis->setTickLabels(false);
	//cp->yAxis->setAutoTickCount(2);
	cp->yAxis->ticker()->setTickCount(2); // new ver

	connect(cp->xAxis, SIGNAL(rangeChanged(QCPRange)),
			cp->xAxis2, SLOT(setRange(QCPRange)));

	connect(cp->yAxis, SIGNAL(rangeChanged(QCPRange)),
			cp->yAxis2, SLOT(setRange(QCPRange)));

	connect(ui->checkBox_bitrateLegend, &QCheckBox::toggled, cp->legend, &QCPLegend::setVisible);

	cp->legend->setVisible(false);

	cp->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

QString
ModemToolMainWindow::getFormString() const
{
	QString set;
	set += QString::number(ui->tabWidget->currentIndex()) + "|";
	set += QString::number(ui->checkBox_longWait->isChecked()) + "|";
	set += QString::number(ui->spinBox_pushTimer->value()) + "|";
	set += QString::number(ui->checkBox_bitrateLegend->isChecked()) + "|";
	set += ui->lineEdit_modemSetupString->text() + "|";
	set += QString::number(ui->spinBox_graphsLong->value()) + "|";
	return set;
}

void
ModemToolMainWindow::setFormString(const QString& set)
{
	const QStringList ls(set.split(QLatin1Char('|')));

	if(ls.size() < 6)
	{
		return; // кто то покопался в INI
	}

	ui->tabWidget->setCurrentIndex(ls.at(0).toInt());
	ui->checkBox_longWait->setChecked(ls.at(1).toInt());
	ui->spinBox_pushTimer->setValue(ls.at(2).toInt());
	ui->checkBox_bitrateLegend->setChecked(ls.at(3).toInt());
	ui->lineEdit_modemSetupString->setText(ls.at(4));
	ui->spinBox_graphsLong->setValue(ls.at(5).toInt());
}

void
ModemToolMainWindow::setupPPPATButtonsConnections()
{
	// +++AT кнопки, все обрабатываются одним обработчиком.
	connect(ui->pushButton_0, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_1, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_2, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_3, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_4, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_5, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_6, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_7, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_8, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_9, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
	connect(ui->pushButton_10, SIGNAL(clicked(bool)),
			this, SLOT(on_pushButton_pppAT_clicked()));
}

void
ModemToolMainWindow::loadCommands(const QString& fileName)
{
	if(fileName.isEmpty())
	{
		return;
	}

	// setup Table Header
	modemCommandsModel->setHorizontalHeaderLabels({"Command","Respond","Help"});

	emit toLog("Trying to load commands file=" + fileName + ".", 0);
	QFile file(fileName);

	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		emit toLog("Can't open file " + fileName, 1);
		return;
	}

	modemCommandsModel->clear();
	modemCommandsModel->setColumnCount(2);
	QTextStream stream(&file);

	while(!stream.atEnd())
	{
		const QString s = stream.readLine();
		const QStringList sl = s.split(":");

		if(sl.size() > 1)
		{
			modemCommandsModel->appendRow({new QStandardItem(sl.at(0)),
										   new QStandardItem(),
										   new QStandardItem(sl.at(1))});
		}
	}
}

void
ModemToolMainWindow::setStatusMessage(QString str, int level)
{
	ui->statusBar->showMessage(str);

	if(level == 1)
	{
		QMessageBox::critical(this, "Modemtool", str);
	}
}

void
ModemToolMainWindow::saveINISettings()
{
	modemDevice->setSettings(connectionSettingsDlg->getSettings());
	settings.setValue(setMod, connectionSettingsDlg->getSettings().toString());
	settings.setValue(setCsDlg, connectionSettingsDlg->getFormString());
	settings.setValue(setMainWnd, getFormString());
}

void
ModemToolMainWindow::loadINISettings()
{
	connectionSettingsDlg->setFormString(settings.value(setCsDlg).toString());
	modemDevice->setSettings(ModemSettings(settings.value(setMod).toString()));
	setFormString(settings.value(setMainWnd).toString());
}

void
ModemToolMainWindow::errorModemHandler(const QString& s)
{
	setStatusMessage(s, 1);
}

void
ModemToolMainWindow::infoModemHandler(const QString& s)
{
	setStatusMessage(s, 0);
}

void
ModemToolMainWindow::interpretResponse(QByteArray data)
{
	if(data.left(5) != "+++AT")
	{
		return;
	}

	// ответ состоит из трех частей команда:длина:результат
	const QList<QByteArray> ls(data.split(':'));

	if(ls.size() != 3)
	{
		return;
	}

	const QByteArray& at0 = ls.at(0); //команда
	const double at2 = ls.at(2).trimmed().toDouble();

	if(at0 == "+++AT?E")
	{
		responseRssi = at2;
	}
	else if(at0 == "+++AT?I")
	{
		responseIntegrity = at2;
	}
	else if(at0 == "+++AT?BL")
	{
		responseBL = at2;
	}
	else if(at0 == "+++AT?BR")
	{
		responseBR = at2;
	}

	// если эхо требуется то выводить ответ на консоль
	if(ui->checkBox_needEcho->isChecked())
	{
		ui->console->putData(data);
	}
}

void
ModemToolMainWindow::incomingDataModemHandler(QByteArray data)
{
	bytesRecievedInt += data.size();// для счетчка bitrate
	setStatusMessage("Received " + QString::number(data.size()) + " bytes.");

	// попробовать распознать АТ команду в DATA режиме
	// так как работаем только с короткими командами, то ответ
	// ожидаем в однома пакете.
	if(ui->checkBox_getStatus->isChecked())
	{
		interpretResponse(data);
		return;
	}

	// если ожидается ответ (в режиме COMMAND) то записать его
	if(waitingForRespondBool)
	{
		bytesRespondBA += data;
	}

	//слишком длинные данные не выводим на консоль
	if(data.size() < 700)
	{
		ui->console->putData(data);
	}
	else
	{
		ui->console->putData("Big data...\n");
	}
}

void
ModemToolMainWindow::connectedModemHandler()
{
	ui->actionConnect->setEnabled(false);
	ui->actionDisconnect->setEnabled(true);
	ui->actionConfigure->setEnabled(false);
	ui->console->setEnabled(true);

	const ModemSettings& p = modemDevice->getSettings();

	if(p.type == ModemSettings::COMTYPE)
	{
		ui->label_ConnectedTo->setText(p.name);
	}
	else
	{
		ui->label_ConnectedTo->setText(p.server);
	}

	// Закрыть сервер только 1 буй может быть подключен
	tcpServer->close(); // stop listening
}

void
ModemToolMainWindow::disconnectedModemHandler()
{
	ui->actionConnect->setEnabled(true);
	ui->actionDisconnect->setEnabled(false);
	ui->actionConfigure->setEnabled(true);
	ui->console->setEnabled(false);
	ui->label_ConnectedTo->setText("Disonnected.");
}

void
ModemToolMainWindow::bytesWrittenModemHandler(qint64 bytes)
{
	bytesWrittenInt += bytes;
	ui->statusBar->messageChanged("Bytes written " +
								  QString::number(bytesWrittenInt));
}

void ModemToolMainWindow::serverClientConnected()
{
	QTcpSocket* tcpClient = tcpServer->nextPendingConnection();
	emit toLog(tr("Incoming connection from Buoy: ") + tcpClient->peerAddress().toString() + "\n", 0);
	// попытаться отдать связь на обработчики модема, иначе грохнуть входящее
	modemDevice->connectModemEmulateIp(tcpClient);
}

void ModemToolMainWindow::serverStartListen()
{
	if(!tcpServer->listen(QHostAddress::AnyIPv4, serverPort))
	{
		emit toLog(tr("Unable to start the server: %1.").arg(tcpServer->errorString()), 1);
	}
	else
	{
		for(const QHostAddress& addr : QNetworkInterface::allAddresses())
		{
			if(addr.toIPv4Address())
			{
				ui->console->insertPlainText(tr("The server is running on IP: %1 port: %2\n")
											 .arg(addr.toString())
											 .arg(tcpServer->serverPort()));
			}
		}

		emit toLog(tr("Listening on port: %1").arg(tcpServer->serverPort()));
	}
}

void
ModemToolMainWindow::sendPacketToModem(QByteArray data)
{
	int sz = data.size();
	sendDataToModem(QByteArray(reinterpret_cast<char*>(&sz), sizeof(sz))
					+ data);
}

void
ModemToolMainWindow::sendCommandToModem(QByteArray data)
{
	// если послали команду то нужно дождаться ответа
	// если посылаем данные на модем то тоже ничего не делать
	if(bytesToModemBA.isEmpty())
	{
		sendDataToModem(data + '\r');
		waitingForRespondBool = true;
	}
	else
	{
		emit toLog("Can't send command now. Waiting for data.", 1);
	}
}

void
ModemToolMainWindow::sendDataToModem(QByteArray data)
{
	if(!waitingForRespondBool)
	{
		bytesToModemBA += data;
		transmitBytesToModemTimer->start();
	}
	else
	{
		emit toLog("Can't send now. Waiting for Respond.", 0);
	}
}

void
ModemToolMainWindow::transmitBytesToModemByTimer()
{
	if(bytesToModemBA.isEmpty())
	{
		transmitBytesToModemTimer->stop();
		return;
	}

	const QByteArray block = bytesToModemBA.left(4000);
	setStatusMessage("sending block of bytes: " + QString::number(block.size()));
	bytesToModemBA = bytesToModemBA.right(bytesToModemBA.size() - block.size());
	modemDevice->writeDataModem(block);
}

void
setGraph(QCustomPlot* plot, double delayTime, double n1, double n2 = 0.0)
{
	const double currTime = QTime::currentTime().msecsSinceStartOfDay() / 1000.0;
	plot->graph(0)->addData(currTime, n1);
	plot->graph(0)->data()->removeBefore(currTime - delayTime); // new ver

	if(plot->graphCount() > 1)
	{
		plot->graph(1)->addData(currTime, n2);
		plot->graph(1)->data()->removeBefore(currTime - delayTime); // new ver
	}

	plot->rescaleAxes();
	plot->replot();
}

void
ModemToolMainWindow::sendPPPCommand(QByteArray com)
{
	if(ui->checkBox_needEcho->isChecked())
	{
		ui->console->insertPlainText(com);
	}
	modemDevice->writeDataModem(com);
}

void
ModemToolMainWindow::updateGraphs()
{
	const int graphsLongValue = ui->spinBox_graphsLong->value();
	// bitrate
	setGraph(ui->customPlot_bitrate, graphsLongValue,
			 bytesRecievedInt, bytesWrittenInt);

	ui->label_TXT_bitrateIn->setText(QString::number(bytesRecievedInt));
	ui->label_TXT_bitrateOut->setText(QString::number(bytesWrittenInt));
	bytesRecievedInt = 0; // до этого накапливали, а теперь обнулить
	bytesWrittenInt = 0;
	// bitrate END

	//RSSI
	setGraph(ui->customPlot_RSSI, graphsLongValue, responseRssi);
	ui->label_TXT_RSSI->setText(QString::number(responseRssi));

	//Integrity
	setGraph(ui->customPlot_integrity, graphsLongValue, responseIntegrity);
	ui->label_TXT_integrity->setText(QString::number(responseIntegrity));

	// BL BR
	setGraph(ui->customPlot_BL_BR, graphsLongValue,
			 responseBL, responseBR);
	ui->label_TXT_BL->setText(QString::number(responseBL));
	ui->label_TXT_BR->setText(QString::number(responseBR));

	// послать команды +++AT?E ?I ?BL ?BR
	static int cnt = 0;
	static const QByteArray cmd[4] = {"+++AT?E\r", "+++AT?I\r", "+++AT?BL\r", "+++AT?BR\r"};
	if(ui->checkBox_getStatus->isChecked()) // раз в секунду одна из команд
	{
		sendPPPCommand(cmd[cnt++]);
		cnt %= 4;
	}
}

void
ModemToolMainWindow::on_actionConfigure_triggered()
{
	connectionSettingsDlg->show();
}

void
ModemToolMainWindow::on_pushButton_SendFile_clicked()
{
	emit toLog("Trying to send a file.", 0);

	const QString fName = QFileDialog::getOpenFileName(this, "File to transfer");
	QFile file(fName);

	if(!file.open(QIODevice::ReadOnly))
	{
		emit toLog("Can't open file " + fName, 1);
		return;
	}

	const QByteArray fileB = file.readAll();
	sendDataToModem(fileB);

	emit toLog("File " + fName + ", " + QString::number(fileB.size()) +
			   "of bytes length opened. ",
			   0);
	emit toLog(
		"There is " + QString::number(bytesToModemBA.size()) + " bytes to send.", 0);
}

void
ModemToolMainWindow::on_actionConnect_triggered()
{
	if(modemDevice->getStatus() != DISCONNECTED)
	{
		emit toLog("Already connected!", 1);
		return;
	}

	ui->actionConnect->setEnabled(false);

	modemDevice->setSettings(connectionSettingsDlg->getSettings());
	modemDevice->connectModem();
}

void
ModemToolMainWindow::on_actionDisconnect_triggered()
{
	modemDevice->disconnectModem();
}

void
ModemToolMainWindow::on_pushButton_20mb_toRetrans_clicked()
{
	modemDevice->writeDataModem("data");
}

void
ModemToolMainWindow::on_pushButton_CLS_toRetrans_clicked()
{
	modemDevice->writeDataModem("cls");
}

void
ModemToolMainWindow::on_pushButton_connectModem_toRetrans_clicked()
{
	modemDevice->writeDataModem("connect");
}

void
ModemToolMainWindow::on_pushButton_setupModem_toRetrans_clicked()
{
	const QString s = "setup " + ui->lineEdit_modemSetupString->text();
	modemDevice->writeDataModem(s.toLatin1());
}

void
ModemToolMainWindow::on_pushButton_loadConf_clicked()
{
	// *.cmi - Command for Modem Input
	const QString fileName =
		QFileDialog::getOpenFileName(this, "File to open", "", "*.cmi");
	loadCommands(fileName);
}

QByteArray
ModemToolMainWindow::getRespondOnCommand(QByteArray command)
{
	sendCommandToModem(command);
	QTime dieTime = QTime::currentTime().addMSecs(ui->spinBox_pushTimer->value());

	while(QTime::currentTime() < dieTime)
	{
		QThread::msleep(10);
		QCoreApplication::processEvents();
	}

	waitingForRespondBool = false; //сделаем вид что дождались
	QByteArray rez;
	rez.swap(bytesRespondBA); // respon === .clear!
	return rez;
}

void
ModemToolMainWindow::on_pushButton_pushConf_clicked()
{
	if(modemDevice->getStatus() == DISCONNECTED)
	{
		emit toLog("Can't push conf: modem disconnected.", 0);
		return;
	}

	// запретить любые действия пока идет опрос модема.
	ui->tabWidget->setEnabled(false);

	int i = 0;
	QStandardItem* item;
	const int rows = modemCommandsModel->rowCount();

	while((item = modemCommandsModel->item(i, 0)) != 0)
	{
		ui->label_commandsQueue->setText(QString("%1(%2)").arg(i).arg(rows));
		const QByteArray command = item->text().toLatin1();
		const QByteArray respond = getRespondOnCommand(command);
		const QString res(respond);
		modemCommandsModel->setItem(i, 1, new QStandardItem(res));
		++i;
	}

	ui->tabWidget->setEnabled(true);
}

void
ModemToolMainWindow::on_pushButton_AT_CTRL_clicked()
{
	const QString res = getRespondOnCommand("AT@CTRL");
	emit toLog(res, 0);
}

void
ModemToolMainWindow::on_pushButton_ATO_clicked()
{
	const QString res = getRespondOnCommand("ATO");

	if(res.isEmpty())
	{
		emit toLog("Ok. Now modem is in burst mode.", 0);
	}
	else
	{
		emit toLog("Not Ok. respond is:" + res, 0);
	}
}

void
ModemToolMainWindow::on_pushButton_ppp_clicked()
{
	emit toLog("Waiting before +++", 0);
	const bool isChecked = ui->checkBox_longWait->isChecked();
	const unsigned long time = isChecked ? 6500 : 1500;
	QThread::msleep(time);

	sendDataToModem("+++");
	waitingForRespondBool = true;

	QCoreApplication::processEvents();
	emit toLog("Waiting after +++", 0);
	QCoreApplication::processEvents();

	QThread::msleep(time);

	QCoreApplication::processEvents(); // получить ответы из буфера

	waitingForRespondBool = false;

	QByteArray respond;
	respond.swap(bytesRespondBA); // важно swap он очищает bytesRespondBA

	emit toLog("Respond on +++ is " + respond, 0);
}

void
ModemToolMainWindow::on_pushButton_senCommand_clicked()
{
	const QString cmd = ui->comboBox_commandToSend->currentText();
	ui->comboBox_commandToSend->removeItem(
				ui->comboBox_commandToSend->findText(cmd));
	ui->comboBox_commandToSend->insertItem(0, cmd);
	ui->comboBox_commandToSend->setCurrentIndex(0);
	const QByteArray data = cmd.toLatin1() + '\r';
	ui->console->insertPlainText(data);
	modemDevice->writeDataModem(data);
}

void
ModemToolMainWindow::on_pushButton_pppAT_clicked()
{
	const QPushButton* button = qobject_cast<QPushButton*>(sender());

	if(button == 0)
	{
		return;
	}

	const QByteArray com = "+++AT" + button->text().toLatin1() + '\r';
	ui->console->insertPlainText(com);
	modemDevice->writeDataModem(com);
}

void
ModemToolMainWindow::on_pushButton_saveConf_clicked()
{
	// *.cmo - Command for Modem Output
	const QString fileName =
		QFileDialog::getSaveFileName(this, "File to save.", "", "*.cmo");

	emit toLog("Trying to save respondes file=" + fileName + ".", 0);
	QFile file(fileName);

	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		emit toLog("Can't open file " + fileName, 1);
		return;
	}

	QTextStream stream(&file);

	for(int i = 0, rows = modemCommandsModel->rowCount(); i < rows; ++i)
	{
		stream << modemCommandsModel->item(i, 0)->text() // команды
			   << "<<"
			   << modemCommandsModel->item(i, 1)->text() // ответы
			   << ">>\n";
	}
}

void
ModemToolMainWindow::on_tabWidget_currentChanged(int/* index*/)
{
	ui->checkBox_getStatus->setChecked(false);
}
