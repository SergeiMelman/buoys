#include "database.h"
#include <QSqlError>

DataBase::DataBase(QObject *parent)	: QObject(parent)
{
}

DataBase::~DataBase()
{
}

void DataBase::toDB(QString str)
{
	const bool rez = query.exec(str);
	const QString rezMes = rez ? tr("DB:Ok") : (tr("DB Error: ") + query.lastError().text());
	toLog(rezMes, rez?0:1);
}

void DataBase::start()
{
	dbase = QSqlDatabase::addDatabase("QSQLITE");  // default connection
	dbase.setDatabaseName("buoys.sqlite");
	dbase.open();
	query = QSqlQuery(dbase);

	if(dbase.isOpen() && !query.exec("CREATE TABLE IF NOT EXISTS bouys ("
									 "name    TEXT, "
									 "address TEXT, "
									 "request TEXT, "
									 "time    TEXT, "
									 "answer  TEXT);"))
	{
		emit toLog(tr("Can't create a TABLE buoys"), 1);
	} else
	{
		emit toLog(tr("DB opened."), 0);
	}
}
