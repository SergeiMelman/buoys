#include "buoysdata.h"
#include "../dummybuoyip/data.h"

BuoysData::BuoysData(QObject *parent)
	: QObject(parent)
	, stringModel(new QStringListModel(this))
{

}

// update - очищает все сохраненные данные и заполняет их
//          согласно query
// query - настроенный на БД со всеми фильтрами
void BuoysData::update(QSqlQuery query)
{
	// идея  такова, что stringModel является предком для всех
	// QStandardItemModel которые хранятся в data.
	// чтобы не перебирать весь data при data.clear() и не делать
	// delete data[name] нужно просто сделать delete stringModel и он убъет всех
	// потомков. При разрушении объекта BuoysData и сам stringModel будет уничтожен
	// как потомок (this). Ура!

	clear();

	if(!query.first())
	{
		// если данных нет то выйти
		return;
	}

	do
	{
		QString name = query.value("name").toString();
		if(!data.contains(name))
		{
			QStandardItemModel *model = new QStandardItemModel(stringModel);
			data[name] = model;
			model->setHorizontalHeaderLabels({tr("datetime"), tr("buoy_id"), tr("lat"),
											  tr("lon"), tr("state"),
											  tr("bat_v"), tr("sensor1"),
											  tr("sensor2"), tr("sensor3"),
											  tr("bitrate"), tr("RSSI")});
		}

		QStandardItemModel *model = data[name];
		int row = model->rowCount();
		QByteArray answer = query.value("answer").toByteArray();

		if(answer.size() == 154)
		{

			Data d;
			d.fromString(answer.data()+4); // пропустить первые 4 байта DATA
			QStandardItem *item = new QStandardItem(QString(d.datetime));
			// сохраним структуру d в нулевой колонке в Qt::UserRole
			item->setData(QByteArray((const char *)&d, sizeof(d)));
			model->setItem(row, 0, item);
			model->setItem(row, 1, new QStandardItem(QString::number(d.buoy_id)));
			model->setItem(row, 2, new QStandardItem(QString::number(d.lat)));
			model->setItem(row, 3, new QStandardItem(QString::number(d.lon)));
			model->setItem(row, 4, new QStandardItem(QString::number(d.state)));
			model->setItem(row, 5, new QStandardItem(QString::number(d.bat_v)));
			model->setItem(row, 6, new QStandardItem(QString::number(d.sensor1)));
			model->setItem(row, 7, new QStandardItem(QString::number(d.sensor2)));
			model->setItem(row, 8, new QStandardItem(QString::number(d.sensor3)));
			model->setItem(row, 9, new QStandardItem(QString::number(d.bitrate)));
			model->setItem(row, 10, new QStandardItem(QString::number(d.RSSI)));
		} else
		{
			data[name]->setItem(row, 0, new QStandardItem(tr("Error in DATA")));
		}
	}while(query.next());
	// сохранить список имен буев
	stringModel->setStringList(data.keys());
}

void BuoysData::clear()
{
	stringModel->deleteLater();
	stringModel = new QStringListModel(this);
	data.clear();
}

QStandardItemModel* BuoysData::getModel(QString name) const
{
	return data[name];
}

QStringListModel*BuoysData::getStringModel() const
{
	return stringModel;
}

QStringList BuoysData::getNames() const
{
	return data.keys();
}
