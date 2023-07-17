#include "buoyrequesteripmainwindow.h"
#include "ui_buoyrequesteripmainwindow.h"

#include <QStandardItem>
#include <QStringListModel>
#include <QTimer>

MainWindow::MainWindow(Server* server_, QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, server(server_)
{
	ui->setupUi(this);

	ui->tableView_history->setModel(server->getBuoysModel());
	ui->listView_specialCommands->setModel(server->getSpecialCommandsModel());
	ui->listView_specialCommands->setEditTriggers(
				QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);

	connect(this, SIGNAL(toLog(QString,int)), // из окна
			this, SLOT(toStatusBar(QString,int))); // в окно

	connect(server, SIGNAL(toLog(QString,int)), // из сервера
			this, SLOT(toStatusBar(QString,int))); // в окно

	connect(server, SIGNAL(serverStatus(bool)),
			this, SLOT(setStartStop(bool)));

	QTimer *historyUpdater = new QTimer(this);
	connect(historyUpdater, SIGNAL(timeout()), this, SLOT(buoysModelViewNeedUpdate()));
	historyUpdater->start(500);


	connect(server, SIGNAL(clientsNumChanged(int)),
			ui->label_clientsNum, SLOT(setNum(int)));

	// установить SERV строку в сервере
	on_lineEdit_specialCommands_editingFinished();

	// При запуске стартовать сервер
	server->start(true);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::setStartStop(bool s)
{
	const QString text = s ? tr("Stop") : tr("Start");
	ui->actionStart->setText(text);
	ui->actionStart->setChecked(s);
	ui->label_Serverport->setText(QString::number(server->getServerPort()));
	ui->label_serverIp->setText(server->getServerAddress());
}

void MainWindow::toStatusBar(const QString& mes, int) const
{
	ui->statusBar->showMessage(mes);
}

void MainWindow::buoysModelViewNeedUpdate() const
{
	if(ui->checkBox_folow->isChecked())
	{
		const int row = server->getBuoysModel()->rowCount() - 1;
		const QModelIndex index = server->getBuoysModel()->index(row, 0);
		ui->tableView_history->scrollTo(index);
	}

	if(ui->checkBox_historyClipping->isChecked() && ui->label_clientsNum->text() == "0")
	{
		const int rows = server->getBuoysModel()->rowCount()
						 - ui->spinBox_historySize->value();
		server->getBuoysModel()->removeRows(0, rows);
	}
}

void MainWindow::on_pushButton_add_clicked() const
{
	server->getSpecialCommandsModel()->insertRow(0);
	const QModelIndex& index = server->getSpecialCommandsModel()->index(0);
	ui->listView_specialCommands->edit(index);
	emit toLog(tr("New line in special list added."), 0);
}

void MainWindow::on_pushButton_remove_clicked() const
{
	const int row = ui->listView_specialCommands->currentIndex().row();
	const QModelIndex index = server->getSpecialCommandsModel()->index(row);
	const QVariant var = server->getSpecialCommandsModel()->data(index, Qt::EditRole);
	server->getSpecialCommandsModel()->removeRow(row);
	emit toLog(tr("Delete from special list: ") + var.toString(), 0);
}

void MainWindow::on_pushButton_clear_clicked() const
{
	server->getSpecialCommandsModel()->removeRows(0, server->getSpecialCommandsModel()->rowCount());
	emit toLog(tr("Cpecial list cleared."), 0);
}

void MainWindow::on_actionStart_triggered(bool checked) const
{
	server->start(checked);
}

void MainWindow::on_lineEdit_specialCommands_editingFinished()
{
	server->setServString(ui->lineEdit_specialCommands->text());
}

void MainWindow::on_pushButton_clear_history_clicked()
{
	server->getBuoysModel()->clear();
}
