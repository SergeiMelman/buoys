// 1. Считать INI файл, там названия буев, адреса и строки отправки.
// 2. Пользователь подключается к модему, сразу заработал опрос
// 3. Каждый раунд опроса:
//    - взять адрес удаленного модема очередного буя
//    - подключиться к удаленному модему
//    - послать строку
//    - дождаться ответа, записать строкув БД

#include "buoyrequestermainwindow.h"
#include "ui_buoyrequestermainwindow.h"

#include <QSettings>
#include <QStandardItem>
#include <QMessageBox>

#include "scheduler.h"

#include "../modemtool/logger.h"
#include "../modemtool/modem.h"
#include "../modemtool/settingsdialog.h"

static const QLatin1String INIname("buoyrequester.ini");
static const QLatin1String INIbuoysGroup("buoys");
static const QLatin1String INImodemDialog("modem/dialogString");
static const QLatin1String INIwinRInterval("window/requestinterval");
static const QLatin1String INIattempts("remote/attempts");
static const QLatin1String INIendPhrase("remote/endphrase");

BuoyrequesterMainWindow::BuoyrequesterMainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, buoysModel(new QStandardItemModel(this))
	, settings(new QSettings(INIname, QSettings::IniFormat, this))
	, modemDevice(new Modem(this))
	, logger(new Logger(this))
	, connectionSettingsDlg(new ConnectionSettingsDialog(this))
	, scheduler(new Scheduler(buoysModel, modemDevice, this))
{
	//
	ui->setupUi(this);
	// связать модель и вид таблицы
	ui->tableView->setModel(buoysModel);
	// установить гориз заголовки таблицы
	buoysModel->setHorizontalHeaderLabels({tr("Buoy Name"), tr("Address"),
										   tr("Request string"), tr("Request time"),
										   tr("Respond time"), tr("Status")});
	// обработчики сигналов модема лога интерфейса
	connect(modemDevice, SIGNAL(connectedModem()),
			this, SLOT(connectedModemHandler()));

	// настройка обработчика сообщений об ошибках, ведение лога и статусбара
	connect(modemDevice, SIGNAL(infoModem(QString)),
			logger, SLOT(log0(QString))); // в логгер из модема тип обишки 0
	connect(modemDevice, SIGNAL(errorModem(QString)),
			logger, SLOT(log1(QString))); // в логгер из модема тип обишки 1

	connect(scheduler, SIGNAL(toLog(QString,int)),
			logger, SLOT(log(QString,int))); //в логер из шедулера
	connect(scheduler, SIGNAL(toLog(QString,int)),
			this, SLOT(showMessage(QString))); // в тулбар из шедулера

	connect(this, SIGNAL(toLog(QString, int)),
			logger, SLOT(log(QString, int))); // в логгер из гл окна
	connect(this, SIGNAL(toLog(QString, int)),
			this, SLOT(showMessage(QString))); // в тулбар из гл окна
	connect(modemDevice, SIGNAL(infoModem(QString)),
			this, SLOT(showMessage(QString))); // в тулбар из модема
	connect(modemDevice, SIGNAL(errorModem(QString)),
			this, SLOT(showMessage(QString))); // в тулбар из модема
	connect(modemDevice, SIGNAL(infoModem(QString)),
			ui->label_connectionInfo, SLOT(setText(QString)));

	// де/активировать кнопки интерфейса
	connect(modemDevice, SIGNAL(connectedModem()),
			this, SLOT(setInterfaceControlsEnabled()));
	connect(modemDevice, SIGNAL(disconnectedModem()),
			this, SLOT(setInterfaceControlsEnabled()));
	connect(ui->checkBox, SIGNAL(toggled(bool)),
			this, SLOT(setInterfaceControlsEnabled()));

	// загрузить настройки из файла конфигурации
	loadINISettings();
}

BuoyrequesterMainWindow::~BuoyrequesterMainWindow()
{
	delete ui;
}

void
BuoyrequesterMainWindow::changeEvent(QEvent* e)
{
	QMainWindow::changeEvent(e);

	switch(e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;

	default:
		break;
	}
}

void
BuoyrequesterMainWindow::saveINISettings()
{
	// save Modem String
	settings->setValue(INImodemDialog, connectionSettingsDlg->getFormString());
	// save Interval
	settings->setValue(INIwinRInterval, ui->spinBox_requestInterval->value());

	//если еще нет таких то сохранить со значениями по умолчанию
	if(settings->value(INIattempts).isNull())
	{
		settings->setValue(INIattempts, 15);
	}

	if(settings->value(INIendPhrase).isNull())
	{
		settings->setValue(INIendPhrase, "ENDDATA");

	}

	settings->beginGroup(INIbuoysGroup);

	if(settings->allKeys().isEmpty())
	{
		settings->setValue("buoyname", QStringList({"0", "sendString"}));
	}

	settings->endGroup();
}

void
BuoyrequesterMainWindow::loadINISettings()
{
	loadBuoys();

	const QString settingsDlgStr = settings->value(INImodemDialog).toString();
	connectionSettingsDlg->setFormString(settingsDlgStr);

	const int interval = settings->value(INIwinRInterval, 60).toInt();
	ui->spinBox_requestInterval->setValue(interval);

	const int attempts = settings->value(INIattempts, 15).toInt();
	scheduler->setStage3Attempts(attempts);

	const QByteArray endPhrase =
			settings->value(INIendPhrase, "ENDDATA").toByteArray();
	scheduler->setEndPhrase(endPhrase);
}

void
BuoyrequesterMainWindow::loadBuoys()
{
	settings->beginGroup(INIbuoysGroup);
	const QStringList keys = settings->allKeys();
	emit toLog(tr("Buoys count %1").arg(keys.size()), 0);

	for(const QString& buoyName : keys)
	{
		const QVariant buoyData = settings->value(buoyName);
		const QStringList buoyDataList = buoyData.toStringList();

		if(buoyDataList.size() == 2)
		{
			const QString& buoyAddress = buoyDataList.at(0);
			const QString& buoySendString = buoyDataList.at(1);

			QList<QStandardItem*> row;
			row << new QStandardItem(buoyName) << new QStandardItem(buoyAddress)
				<< new QStandardItem(buoySendString);

			buoysModel->appendRow(row);
		}
	}

	settings->endGroup();
}

void
BuoyrequesterMainWindow::connectedModemHandler()
{
	// сигналы checkBox неявно вызывают setInterfaceControlsEnabled
	ui->checkBox->setChecked(true);
	scheduler->start(ui->spinBox_requestInterval->value());
}

void
BuoyrequesterMainWindow::showMessage(const QString& s)
{
	ui->statusBar->showMessage(s);
}

void
BuoyrequesterMainWindow::on_actionConnect_triggered()
{
	// save INI
	saveINISettings();

	if(!scheduler->isDatabaseOk())
	{
		QMessageBox::critical(this, "Database error",
							  tr("DB last error:") +
							  scheduler->getDatabaseLastError());
		return;
	}

	ui->actionConnect->setEnabled(false);

	modemDevice->setSettings(connectionSettingsDlg->getSettings());
	modemDevice->connectModem();
}

void
BuoyrequesterMainWindow::on_actionDisconnect_triggered()
{
	modemDevice->disconnectModem();
}

void
BuoyrequesterMainWindow::on_actionConfigure_triggered()
{
	connectionSettingsDlg->show();
}

void
BuoyrequesterMainWindow::setInterfaceControlsEnabled()
{
	if(ui->checkBox->isChecked())
	{
		// если включена блокировка все элементы управления заблокировать
		ui->actionConnect->setEnabled(false);
		ui->actionConfigure->setEnabled(false);
		ui->actionDisconnect->setEnabled(false);
		ui->spinBox_requestInterval->setEnabled(false);
	}
	else
	{
		// иначе разблокировать
		ui->spinBox_requestInterval->setEnabled(true);

		if(modemDevice->getStatus() == DISCONNECTED)
		{
			ui->actionConnect->setEnabled(true);
			ui->actionDisconnect->setEnabled(false);
			ui->actionConfigure->setEnabled(true);
		}
		else
		{
			ui->actionConnect->setEnabled(false);
			ui->actionDisconnect->setEnabled(true);
			ui->actionConfigure->setEnabled(false);
		}
	}
}
