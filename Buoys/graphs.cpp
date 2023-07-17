#include "graphs.h"
#include "ui_graphs.h"

#include <QStandardItemModel>

#include "../dummybuoyip/data.h"

Graphs::Graphs(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::Graphs)
	, buoyModel(0)
{
	ui->setupUi(this);

	plots[0] = ui->frame_graph_bat_v;
	plots[1] = ui->frame_sensor1;
	plots[2] = ui->frame_sensor2;
	plots[3] = ui->frame_sensor3;
	plots[4] = ui->frame_bitrate;
	plots[5] = ui->frame_RSSI;

	connect(ui->spinBox_horizontalRange, SIGNAL(valueChanged(int)), this,
			SLOT(horizontalRangeChanged(int)));

	// для всех графиков
	for(int i = 0; i < 6; ++i)
	{
		// если поменялся диапазон перерисовать
		connect(plots[i]->xAxis, SIGNAL(rangeChanged(QCPRange)), plots[i],
				SLOT(replot()));
		// если мышь на графиком показать тултип
		connect(plots[i], SIGNAL(mouseMove(QMouseEvent*)), this,
				SLOT(showPointToolTip(QMouseEvent*)));

		// соединить каждую с каждой для синхронного изменения диапазона
		for(int j = 0; j < 6; ++j)
		{
			if(i != j)  // саму с собой соединять не надо
			{
				// смена диапазона у одной меняет его у всех а те в свою очередь
				// перерисовываются
				connect(plots[i]->xAxis, SIGNAL(rangeChanged(QCPRange)),
						plots[j]->xAxis, SLOT(setRange(QCPRange)));
			}
		}
	}

	clearGraphs();
}

Graphs::~Graphs()
{
	delete ui;
}

// model_ должна содержать 6 колонок данных (по кол-ву данных от буя)
void
Graphs::setModel(QStandardItemModel* model_)
{
	buoyModel = model_;
	clearGraphs();

	const int rows = buoyModel->rowCount();
	QVector<double> x(rows);
	// создать массив double y[6][rows];
	QVector<QVector<double> > y(6, QVector<double>(rows));

	for(int i = 0; i < rows; ++i)
	{
		// в 0 колонке юзер данные Data
		const QByteArray ba = buoyModel->item(i, 0)->data().toByteArray();
		const Data* pd = (const Data*)ba.data();

		// есть вариант абсциссу сделать не по количеству а по времени
		if(ui->checkBox_time->isChecked())
		{
			const QLatin1String format("yyyy-MM-dd HH:mm:ss");
			const QDateTime dt = QDateTime::fromString(pd->datetime, format);
			x[i] = dt.toTime_t();
		}
		else
		{
			// +1 потому что в таблице выводятся с 1 и не с 0
			x[i] = i + 1;
		}

		y[0][i] = pd->bat_v;
		y[1][i] = pd->sensor1;
		y[2][i] = pd->sensor2;
		y[3][i] = pd->sensor3;
		y[4][i] = pd->bitrate;
		y[5][i] = pd->RSSI;
	}

	for(int i = 0; i < 6; ++i)
	{
		plots[i]->graph(0)->setData(x, y[i]);
		plots[i]->yAxis->rescale();
		plots[i]->xAxis->rescale();
		plots[i]->replot();
	}

	ui->spinBox_horizontalRange->setValue(plots[0]->xAxis->range().size());
}

void
Graphs::clearGraphs()
{
	static const QStringList names({ tr("bat_v"), tr("sensor1"), tr("sensor2"),
			   tr("sensor3"), tr("bitrate"), tr("RSSI")});

	for(int i = 0; i < 6; ++i)
	{
		plots[i]->clearGraphs();
		plots[i]->addGraph();
		plots[i]->graph()->setPen(QPen(Qt::blue));
		plots[i]->graph()->setBrush(QBrush(QColor(0, 0, 255, 20)));
		// plots[i]->graph()->setLineStyle(QCPGraph::lsStepCenter);
		plots[i]->graph()->setScatterStyle(
			QCPScatterStyle(QCPScatterStyle::ssCrossCircle));
		plots[i]->axisRect()->setupFullAxesBox(true);
		plots[i]->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
								  QCP::iSelectAxes);
		plots[i]->yAxis->setLabel(names.at(i));
		plots[i]->yAxis->setTickLabelRotation(-90);
		plots[i]->axisRect()->setRangeZoom(Qt::Vertical);

		if(ui->checkBox_time->isChecked())
		{
			// configure bottom axis to show date instead of number:
			QSharedPointer<QCPAxisTickerDateTime> dateTicker(
				new QCPAxisTickerDateTime);
			dateTicker->setDateTimeFormat("HH:mm:ss\ndd.MM.yyyy");
			plots[i]->xAxis->setTicker(dateTicker);
		}
		else
		{
			QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
			plots[i]->xAxis->setTicker(ticker);
		}

		plots[i]->setToolTipDuration(0);
		plots[i]->replot();
	}
}

void
Graphs::changeEvent(QEvent* e)
{
	QWidget::changeEvent(e);

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
Graphs::horizontalRangeChanged(int v)
{
	const int lower = plots[0]->xAxis->range().lower;
	plots[0]->xAxis->setRange(lower, lower + v);
}

void
Graphs::showPointToolTip(QMouseEvent* event)
{
	QCustomPlot* cp = qobject_cast<QCustomPlot*>(sender());

	if(cp != 0 && cp->graphCount() > 0)
	{
		const double cx = cp->xAxis->pixelToCoord(event->pos().x());
		const auto localFindBegin = cp->graph()->data()->findBegin(cx);
		const double y = localFindBegin->value;
		const double x = localFindBegin->key;
		const QString str("Pos: %1\nValue: %2");

		if(ui->checkBox_time->isChecked())
		{
			const QLatin1String format("yyyy-MM-dd HH:mm:ss");
			const QString xs = QDateTime::fromTime_t(x).toString(format);
			cp->setToolTip(str.arg(xs).arg(y));
		}
		else
		{
			cp->setToolTip(str.arg(int(x)).arg(y));
		}
	}
}

void
Graphs::on_checkBox_time_clicked()
{
	if(buoyModel != 0)
	{
		setModel(buoyModel);
	}
}
