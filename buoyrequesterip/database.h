#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>


class DataBase : public QObject
{
	Q_OBJECT
public:
	explicit DataBase(QObject *parent = 0);
	~DataBase();

signals:
	void toLog(QString message, int level) const;

public slots:
	void toDB(QString str);
	void start();

private:
	QSqlDatabase dbase;
	QSqlQuery query;
};

#endif // DATABASE_H
