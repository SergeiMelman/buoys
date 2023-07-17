#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "modemi.h"

namespace Ui
{
class MainWindow;
}

class Logger;
class QLabel;
class ConnectionSettingsDialog;
class Console;
class QIODevice;
class QSerialPort;
class QTcpServer;
class GraphDrawer;
class QStandardItemModel;
class QCustomPlot;

class ModemToolMainWindow : public QMainWindow, public ModemI
{
	Q_OBJECT

public:
	explicit ModemToolMainWindow(QWidget* parent = 0);
	~ModemToolMainWindow();
private:
	void fillComboBoxCommandFiles();
	void setupCustomGraph(QCustomPlot* cp, const QString& n1, const QString& n2);
	QString getFormString() const;
	void setFormString(const QString& set);
	void setupPPPATButtonsConnections();
	void interpretResponse(QByteArray data);

private slots:
	void loadCommands(const QString& fileName);
	void setStatusMessage(QString str, int level = 0);
	void saveINISettings();
	void loadINISettings();

	void errorModemHandler(const QString& s) Q_DECL_OVERRIDE;
	void infoModemHandler(const QString& s) Q_DECL_OVERRIDE;
	void incomingDataModemHandler(QByteArray data) Q_DECL_OVERRIDE;
	void connectedModemHandler() Q_DECL_OVERRIDE;
	void disconnectedModemHandler() Q_DECL_OVERRIDE;
	void bytesWrittenModemHandler(qint64 bytes) Q_DECL_OVERRIDE;

	void serverClientConnected();
	void serverStartListen();

	void sendPacketToModem(QByteArray data);
	void sendCommandToModem(QByteArray data);
	void sendDataToModem(QByteArray data);
	void transmitBytesToModemByTimer();

	void sendPPPCommand(QByteArray com);
	void updateGraphs();

	void on_actionConfigure_triggered();
	void on_pushButton_SendFile_clicked();
	void on_actionConnect_triggered();
	void on_actionDisconnect_triggered();

	void on_pushButton_20mb_toRetrans_clicked();

	void on_pushButton_CLS_toRetrans_clicked();

	void on_pushButton_connectModem_toRetrans_clicked();

	void on_pushButton_setupModem_toRetrans_clicked();

	void on_pushButton_loadConf_clicked();

	QByteArray getRespondOnCommand(QByteArray command);
	void on_pushButton_pushConf_clicked();

	void on_pushButton_AT_CTRL_clicked();

	void on_pushButton_ATO_clicked();

	void on_pushButton_ppp_clicked();

	void on_pushButton_senCommand_clicked();

	void on_pushButton_pppAT_clicked();

	void on_pushButton_saveConf_clicked();

	void on_tabWidget_currentChanged(int);

signals:
	void toLog(const QString& s, int level = 0);

private:
	Ui::MainWindow* ui;

	ConnectionSettingsDialog* connectionSettingsDlg;
	Modem* modemDevice;
	QTcpServer* tcpServer; // Для входящих
	quint16 serverPort;

	QTimer* transmitBytesToModemTimer;

	Logger* logger;

	QByteArray bytesToModemBA;
	qint64 bytesWrittenInt;
	qint64 bytesRecievedInt;

	QStandardItemModel *modemCommandsModel;

	bool waitingForRespondBool;
	QByteArray bytesRespondBA;

	double responseRssi;
	double responseIntegrity;
	double responseBL;
	double responseBR;
};

#endif // MAINWINDOW_H
