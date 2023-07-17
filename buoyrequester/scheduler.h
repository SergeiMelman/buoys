#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>

class QStandardItemModel;
class Modem;
class QTimer;

class Scheduler : public QObject
{
	Q_OBJECT
public:
	explicit Scheduler(QStandardItemModel* buoysModel_, Modem* modemDevice_,
					   QObject *parent = 0);

public:
	bool isDatabaseOk();
	QString getDatabaseLastError();

	void setStage3Attempts(int value);
	void setEndPhrase(const QString& value);

signals:
	void toLog(const QString& s, int level);

public slots:
	void start(int intervalMinutes);
	void stop();

protected slots:
	void prepareModemDevice();
	void incomingDataModemHandler(QByteArray data);
	void setBuoysModel(int column, QString str);

	void doRequestStage1();
	void doRequestStage2();
	void doRequestStage3();
	void finalizeRequest(const QString& mess);
	void doRequestRound();


protected:
	QStandardItemModel* buoysModel;
	Modem* modemDevice;
	QTimer* globalRequestTimer;
	QTimer* singleRequestTimer;

	int currentRow;
	QByteArray incomingData;
	int stage3Count;
	int stage3Attempts;
	QString endPhrase;

	QSqlDatabase dbase;
	QSqlQuery query;
};

#endif // SCHEDULER_H
