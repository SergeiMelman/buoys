#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include "server.h"

class Logger;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Server* server_, QWidget *parent = 0);
	~MainWindow();

public:

protected:
	void changeEvent(QEvent *e);

signals:
	void toLog(const QString& mes, int) const;

private slots:
	void setStartStop(bool s);
	void toStatusBar(const QString& mes, int) const;
	void buoysModelViewNeedUpdate() const;

	void on_pushButton_add_clicked() const;

	void on_pushButton_remove_clicked() const;

	void on_pushButton_clear_clicked() const;

	void on_actionStart_triggered(bool checked) const;

	void on_lineEdit_specialCommands_editingFinished();

	void on_pushButton_clear_history_clicked();

private:
	Ui::MainWindow *ui;
	Server* server;
};

#endif // MAINWINDOW_H
