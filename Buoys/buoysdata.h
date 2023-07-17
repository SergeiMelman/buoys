// BuoysData - хранит данные буев в виде таблицы связанной с именем буя
// stringModel - список имен буев для удобства отображения

#ifndef BUOYSDATA_H
#define BUOYSDATA_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStandardItemModel>
#include <QSqlQuery>
#include <QStringListModel>

class BuoysData : public QObject
{
	Q_OBJECT
public:
	explicit BuoysData(QObject *parent);

public:
	void update(QSqlQuery query);
	void clear();
	QStandardItemModel* getModel(QString name) const;
	QStringListModel* getStringModel() const;
	QStringList getNames() const;

signals:

public slots:

private:
	typedef QMap<QString, QStandardItemModel*> DataMap;
	DataMap data;
	QStringListModel *stringModel;
};

#endif // BUOYSDATA_H
