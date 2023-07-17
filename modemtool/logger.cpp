#include "logger.h"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QTextStream>
#include <QFileInfo>

// глобальный указательно на логгер
// я не стал делать класс синглтон (лень). Нужно его самому инициализировать,
// то есть завести и потом приравнять pLogger на него, после чего pLogger
// можно использовать
// объяснение лени: логгер должен иметь предка, чтобы если нет sender то
// вставить в строчку имя предка.

Logger* pLogger = 0;

Logger::Logger(QObject* parent)
	: QObject(parent)
	, name()
	, criticalAccepted(false)
{
	const QFileInfo fInfo(qApp->applicationFilePath());
	name = fInfo.canonicalPath() + "/" + fInfo.baseName() + "_"
			+ QDateTime::currentDateTime().toString(QLatin1String("yyyyMMdd_HHmmss"))
			+ ".log";
	log0(tr("Started"));
}

Logger::~Logger()
{
	log0(tr("Finished"));
}

QString
Logger::getName() const
{
	return name;
}

void
Logger::setName(const QString& value)
{
	name = value;
}

void
Logger::criticalMessage(const QString& str) const
{
	// если пользователь уже "принял" критическое сообщение,
	// то больше не выводить. Не выводить вообще критические сообщения.
	if(!criticalAccepted)
	{
		// проверить qApp гуйное или нет?
		QApplication* guiApp = qobject_cast<QApplication*>(qApp);
		// если гуйное то поверх активного окна вывести мессадж
		QWidget* topWin = guiApp ? guiApp->activeWindow() : 0;
		const QString head = QFileInfo(qApp->applicationFilePath()).baseName();
		QMessageBox::StandardButton rez =
			QMessageBox::critical(topWin, head, str,
								  QMessageBox::Ok | QMessageBox::Ignore,
								  QMessageBox::Ok);
		// юзер нажал игнор?
		*(const_cast<bool*>(&criticalAccepted)) = (rez == QMessageBox::Ignore);
	}
}

// короткая запись для применения в сигнально-слотной системе
// для информ записей
void
Logger::log0(const QString& s) const
{
	log(s, 0);
}

// короткая запись для применения в сигнально-слотной системе
// для ошибок
void
Logger::log1(const QString& s) const
{
	log(s, 1);
}

void
Logger::log(const QString& s, int level) const
{
	QFile logf(name);

	if(!logf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{
		criticalMessage("Can't write log!");
		return;
	}

	// кто пишет в лог?
	const QObject* sender_= sender() ? sender() : (parent() ? parent() : this);
	const char* className = sender_->metaObject()->className();

	QTextStream logs(&logf);
	logs << level
		 << "<" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << ">"
		 << "<" << className << ">" << s << "\n";
}
