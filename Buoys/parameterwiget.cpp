#include "parameterwiget.h"
#include "ui_parameterwiget.h"
#include "buoysdata.h"
#include "../dummybuoyip/data.h"

#include <QFileDialog>

ParameterWiget::ParameterWiget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ParameterWiget)
{
	ui->setupUi(this);
//	ui->comboBox_parameters->addItems({tr("bat_v"), tr("sensor1"),
//									   tr("sensor2"), tr("sensor3"),
//									   tr("bitrate"), tr("RSSI")});
	ui->tableView_buoyParams->horizontalHeader()->
			setSectionResizeMode(QHeaderView::ResizeToContents);
}

ParameterWiget::~ParameterWiget()
{
	delete ui;
}

void ParameterWiget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void ParameterWiget::fillBuoyData()
{
	buoyData.clear();
	const QLatin1String format("yyyy-MM-dd HH:mm:ss");
	QStringList names = buoysData->getNames();
	for(auto name : names)
	{
		const QStandardItemModel* model = buoysData->getModel(name);
		const int rows = model->rowCount();
		startTime = QDateTime::currentDateTime();
		const QDateTime sTime =
				QDateTime::fromString(model->item(0, 0)->text(), format);
		startTime = qMin(sTime, startTime);
		const QDateTime eTime =
				QDateTime::fromString(model->item(rows - 1, 0)->text(), format);
		endTime = qMax(eTime, endTime);
		BuoyData &buoy = buoyData[name];
		buoy.geoPos.setX(model->item(0, 3)->text().toDouble());
		buoy.geoPos.setY(model->item(0, 2)->text().toDouble());
		for(int i = 0; i < rows; ++i)
		{
			const QByteArray ba = model->item(i,0)->data().toByteArray();
			const Data *pd = (const Data *)ba.data();
			// рассчитать мин макс и сумму.
#define PARAM(par) buoy.par[0] = qMin((double)pd->par, buoy.par[0]); buoy.par[1] = qMax((double)pd->par, buoy.par[1]); buoy.par[2] += pd->par;
			PARAM(bat_v);
			PARAM(sensor1);
			PARAM(sensor2);
			PARAM(sensor3);
			PARAM(bitrate);
			PARAM(RSSI);
#undef PARAM
		}
		// рассчитать среднее
		buoy.bat_v[2] /= rows;
		buoy.sensor1[2] /= rows;
		buoy.sensor2[2] /= rows;
		buoy.sensor3[2] /= rows;
		buoy.bitrate[2] /= rows;
		buoy.RSSI[2] /= rows;
		//buoy.model = QSharedPointer<QStandardItemModel>(new QStandardItemModel);
		//buoy.model = QSharedPointer<QStandardItemModel>::create();
		buoy.model.reset(new QStandardItemModel);
		buoy.model->setHorizontalHeaderLabels({tr("Parameter"),tr("min"),
											   tr("max"), tr("average")});
		setModelRow(buoy.model.data(), 0, "bat_v", buoy.bat_v);
		setModelRow(buoy.model.data(), 1, "sensor1", buoy.sensor1);
		setModelRow(buoy.model.data(), 2, "sensor2", buoy.sensor2);
		setModelRow(buoy.model.data(), 3, "sensor3", buoy.sensor3);
		setModelRow(buoy.model.data(), 4, "bitrate", buoy.bitrate);
		setModelRow(buoy.model.data(), 5, "RSSI", buoy.RSSI);
	}
	ui->widget_map->setBuoys(&buoyData);
}

void ParameterWiget::setModelRow(QStandardItemModel* model, int row, QString str, double* par)
{
	model->setItem(row, 0, new QStandardItem(str));
	for(int i = 1; i < 4; ++i)
	{
		model->setItem(row, i, new QStandardItem(QString::number(par[i - 1])));
	}
}

void ParameterWiget::setBuoysData(BuoysData* value)
{
	const QLatin1String format("yyyy-MM-dd HH:mm:ss");

	buoysData = value;

	fillBuoyData();

	ui->listView_names->setModel(buoysData->getStringModel());

	const QString str = tr("Start time: ") + "%1\n" + tr("End time:") + "%2";
	ui->label_startEndDate->setText(str.arg(startTime.toString(format),
											endTime.toString(format)));
}

void ParameterWiget::on_listView_names_clicked(const QModelIndex &index)
{
	ui->tableView_buoyParams->setModel(buoyData[index.data().toString()].model.data());
}

void ParameterWiget::on_pushButton_openMap_clicked()
{
	// открыть диалог выбора карты
	QString fileName
			= QFileDialog::getOpenFileName(this,
										   tr("Open Map"), "",
										   tr("Images (*.tif *.tiff *.bmp "
											  "*.png *.jpg *.xpm)"));
	// если нажали cansel попытаться использовать карту по умолчанию
	if(fileName.isNull())
	{
		fileName = "map.jpg";
	}

	ui->widget_map->loadMap(fileName);
}

void ParameterWiget::on_pushButton_apply_clicked()
{
	ui->widget_map->setAllIsolines(ui->plainTextEdit_commandLine->toPlainText());
}

void ParameterWiget::on_fontComboBox_nameFont_currentFontChanged(const QFont &f)
{
	ui->widget_map->setBuoysFont(f);
}

void ParameterWiget::on_spinBox_fontSize_valueChanged(int arg1)
{
	ui->widget_map->setBuoysFontSize(arg1);
}

void ParameterWiget::on_pushButton_saveMap_clicked()
{
	// открыть диалог
	QString fileName
			= QFileDialog::getSaveFileName(this,
										   tr("Save Map"), "",
										   tr("Images (*.tif *.tiff *.bmp "
											  "*.png *.jpg *.xpm)"));
	if(!fileName.isNull())
	{
		ui->widget_map->saveMap(fileName);
	}
}
