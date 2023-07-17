#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStandardItemModel;
class QSettings;
class Modem;
class Logger;
class ConnectionSettingsDialog;
class Scheduler;

namespace Ui
{
class MainWindow;
}

class BuoyrequesterMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit BuoyrequesterMainWindow(QWidget* parent = 0);
	~BuoyrequesterMainWindow();

protected:
	void changeEvent(QEvent* e);
	void saveINISettings();
	void loadINISettings();
	void loadBuoys();


private slots:
	void connectedModemHandler();

	void showMessage(const QString& s);

	void on_actionConnect_triggered();

	void on_actionDisconnect_triggered();

	void on_actionConfigure_triggered();

	void setInterfaceControlsEnabled();

signals:
	void toLog(const QString& s, int level);

private:
	Ui::MainWindow* ui;

	QStandardItemModel* buoysModel;
	QSettings* settings;
	Modem* modemDevice;
	Logger* logger;
	ConnectionSettingsDialog* connectionSettingsDlg;
	Scheduler* scheduler;
};

#endif // MAINWINDOW_H
