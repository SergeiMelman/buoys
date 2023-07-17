#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtSql/QSqlTableModel>

#include "FilterTableHeader.h"
#include "buoysdata.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, buoysData(new BuoysData(this))
{
	ui->setupUi(this);

	// вообще-то QSqlDatabase синглтон и сохранять его не обязательно, но...
	db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));

	// установить имя файла базы данных
	db->setDatabaseName("buoys.sqlite");

	// открыть базу
	openDB();

	// установить гориз заголовок таблицы
	tableHeader = new FilterTableHeader(ui->tableView_SQL);
	ui->tableView_SQL->setHorizontalHeader(tableHeader);

	// для красоты
	tableHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
	// сгенерировать 5 фильтров в горизонтальном заголовке включая 0
	tableHeader->generateFilters(5, true);
	// соединить сигналы фильтров с обработчиком
	connect(tableHeader, SIGNAL(filterChanged(int, QString)), this,
			SLOT(on_filterChanged(int, QString)));

	// хардкод
	const QLatin1String format("yyyy-MM-dd HH:mm:ss");
	// установить формат датывремени в select start and end date-time
	ui->dateTimeEdit_start->setDisplayFormat(format);
	ui->dateTimeEdit_end->setDisplayFormat(format);

	// отобразить дата-время первой и последней записи с учетом фильтров
	QSqlQuery query = sql_tableModel->query();
	query.first();
	ui->dateTimeEdit_start->setDateTime(query.value("time").toDateTime());
	query.last();
	ui->dateTimeEdit_end->setDateTime(query.value("time").toDateTime());

	// для красоты
	ui->tableView_parameters->horizontalHeader()->setSectionResizeMode(
		QHeaderView::ResizeToContents);
}

MainWindow::~MainWindow()
{
	delete ui;
}

// сгенеренное атоматически
void
MainWindow::changeEvent(QEvent* e)
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
MainWindow::openDB()
{
	if(!db->open())
	{
		QMessageBox::critical(0, QObject::tr("Error"),
							  QObject::tr("Can't open database."),
							  QMessageBox::Cancel);
	}

	// модель данных по SQL таблице
	sql_tableModel = new QSqlTableModel(this, *db);
	// выбрать таблицу
	sql_tableModel->setTable("bouys");
	// выключить сохранение в БД при редактировании
	sql_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
	// заполнить модель
	sql_tableModel->select();

	// связать модель и вид
	ui->tableView_SQL->setModel(sql_tableModel);
}

void
MainWindow::updateFilterAndSelect()
{
	// подготовить SQL запрос с фильтрами
	const QString filterHead = "name    LIKE '%%1%' AND "
							   "address LIKE '%%2%' AND "
							   "request LIKE '%%3%' AND "
							   "time    LIKE '%%4%' AND "
							   "answer  LIKE '%%5%' ";
	const QString filterHeadReady =
		filterHead.arg(filters[0], filters[1], filters[2], filters[3], filters[4]);
	const QLatin1String format("yyyy-MM-dd HH:mm:ss");
	const QString startD = ui->dateTimeEdit_start->dateTime().toString(format);
	const QString endD = ui->dateTimeEdit_end->dateTime().toString(format);
	const QString filterDateTime = "time >= '%1' AND time <= '%2' AND ";
	const QString ready = filterDateTime.arg(startD, endD) + filterHeadReady;

	sql_tableModel->setFilter(ready);
	sql_tableModel->select();
}

// обработка сигнала о конце редактирования одного из фильтров
void
MainWindow::on_filterChanged(int col, QString text)
{
	// сохранить текст фильтра
	filters[col] = text;
	// и применить все фильтры занова
	updateFilterAndSelect();
}

void
MainWindow::on_pushButton_select_clicked()
{
	updateFilterAndSelect();
}

void
MainWindow::on_pushButton_selectAll_clicked()
{
	tableHeader->clearFilters();
	sql_tableModel->setFilter(QString());
	sql_tableModel->select();
}

void
MainWindow::on_tabWidget_currentChanged(int index)
{
	if(index == 0)
	{
		buoysData->clear();
	}
	else
	{
		buoysData->update(sql_tableModel->query());
		QStringListModel* stringListModel = buoysData->getStringModel();
		ui->listView_names2->setModel(stringListModel);
		ui->parameterWidget->setBuoysData(buoysData);
	}
}

// меню открыть базу
void
MainWindow::on_actionOpen_DB_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(
						   this, tr("Open DB"), "", tr("Sqlite Files (*.sqlite)"));

	if(fileName.isNull())
	{
		return;
	}

	ui->graphs->clearGraphs();
	ui->listView_names2->setModel(0);
	ui->tableView_parameters->setModel(0);

	ui->tabWidget->setCurrentIndex(0);

	sql_tableModel->deleteLater();

	db->close();

	db->setDatabaseName(fileName);

	openDB();
}

void
MainWindow::on_actionHelpInfo_triggered()
{
	QMessageBox::information(this, "Информация",
							 "Приложение для отображения информации накопленной");
}

// кликнули по имени буя
void
MainWindow::on_listView_names2_clicked(const QModelIndex& index)
{

	const QString buoyName = index.data().toString();
	// модель с данными буя
	QStandardItemModel* buoyModel = buoysData->getModel(buoyName);
	// образить данные буя
	ui->tableView_parameters->setModel(buoyModel);
	// отобразить графики
	ui->graphs->setModel(buoyModel);
}
