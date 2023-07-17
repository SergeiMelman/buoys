#ifndef PARAMETERWIGET_H
#define PARAMETERWIGET_H

#include <QDateTime>
#include <QWidget>
#include <QMap>
#include <QDateTime>

#include "buoydata.h"

namespace Ui {
class ParameterWiget;
}

class BuoysData;

class ParameterWiget : public QWidget
{
	Q_OBJECT

public:
	explicit ParameterWiget(QWidget *parent = 0);
	~ParameterWiget();

public:
	void setBuoysData(BuoysData* value);

protected:
	void changeEvent(QEvent *e);

private slots:

	void on_pushButton_openMap_clicked();

	void on_listView_names_clicked(const QModelIndex &index);

	void on_pushButton_apply_clicked();

	void on_fontComboBox_nameFont_currentFontChanged(const QFont &f);

	void on_spinBox_fontSize_valueChanged(int arg1);

	void on_pushButton_saveMap_clicked();

private:
	void fillBuoyData();
	void setModelRow(QStandardItemModel* model, int row, QString str, double *par);
private:
	Ui::ParameterWiget *ui;

	BuoysData* buoysData;
	QDateTime startTime, endTime;
	MapBuoyData buoyData;
};

#endif // PARAMETERWIGET_H
