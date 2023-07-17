#ifndef GRAPHS_H
#define GRAPHS_H

#include <QWidget>
#include <QStringList>
#include "../qcustomplot/qcustomplot.h"


namespace Ui {
class Graphs;
}

class QCustomPlot;
class QStandardItemModel;

class Graphs : public QWidget
{
	Q_OBJECT

public:
	explicit Graphs(QWidget *parent = 0);
	~Graphs();
public:
	void setModel(QStandardItemModel* model_);
	void clearGraphs();

protected:
	void changeEvent(QEvent *e);

private slots:
	void horizontalRangeChanged(int v);
	void showPointToolTip(QMouseEvent*event);

	void on_checkBox_time_clicked();

private:
	Ui::Graphs *ui;

	QCustomPlot *plots[6]; // для удобства обращения к графикам по номеру.

	QStandardItemModel* buoyModel;
};

#endif // GRAPHS_H
