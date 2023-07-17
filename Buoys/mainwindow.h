#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>



namespace Ui {
class MainWindow;
}

class QSqlDatabase;
class QSqlTableModel;
class FilterTableHeader;
class BuoysData;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	void changeEvent(QEvent *e);
	void openDB();
	void updateFilterAndSelect();

private slots:
	void on_filterChanged(int col, QString text);

	void on_pushButton_select_clicked();

	void on_pushButton_selectAll_clicked();

	void on_tabWidget_currentChanged(int index);

	void on_actionOpen_DB_triggered();

	void on_actionHelpInfo_triggered();

	void on_listView_names2_clicked(const QModelIndex &index);

private:
	Ui::MainWindow *ui;

	QSqlDatabase *db;
	QSqlTableModel *sql_tableModel;
	FilterTableHeader *tableHeader;
	QMap<int, QString> filters;
	BuoysData *buoysData;
};

#endif // MAINWINDOW_H
