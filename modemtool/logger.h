#ifndef LOG_H
#define LOG_H

#include <QObject>

class Logger : public QObject
{
	Q_OBJECT
public:
	explicit Logger(QObject *parent = 0);
	~Logger();

public:
	QString getName() const;
	void setName(const QString& value);
private:
	void criticalMessage(const QString& str) const;
signals:
public slots:
	void log0(const QString& s) const;
	void log1(const QString& s) const;
	void log(const QString& s, int level) const;
private:
	QString name; // имя логфайла
	bool criticalAccepted; // не беспокоить критическими сообщениями
};

extern Logger* pLogger;

#endif // LOG_H
