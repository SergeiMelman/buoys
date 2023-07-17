#ifndef MAP_H
#define MAP_H

#include <QWidget>
#include "buoydata.h"

#include "Eigen/Dense"

struct ParameterIsoline
{
public:
	ParameterIsoline()
		: width(1)
		, color(qRgba(0, 0, 0, 255))
		, level(0) {}

	qreal width;
	QRgb color;
	qreal level;
};

typedef QVector<ParameterIsoline> ParameterIsolines;
typedef QVector<ParameterIsolines> ParametersIsolines;

class Map : public QWidget
{
	Q_OBJECT
public:
	explicit Map(QWidget *parent = 0);

public:
	void loadMap(QString name);
    void saveMap(QString name);
	void setBuoys(MapBuoyData* value);
	void setAllIsolines(QString str);
    void setBuoysFont(const QFont &value);
    void setBuoysFontSize(int i);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void drawAll(QPainter& painter);
	QPointF toPixel(QPointF p);
	void drawBuoys(QPainter& painter);
	void calcMapIsolines(const ParametersIsolines& allIsolines);

	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	//void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

signals:

public slots:

private:
	QImage map;
	QImage mapIsolines;

	// http://gis-lab.info/qa/tfw.html
	// перевод гео координаты в пикселей map
	Eigen::Matrix3d geoToPixel;

	qreal mapVisibleScale;
	QPointF mapVisibleTopLeft;

	QPoint mouseDown;

	MapBuoyData* buoys;
    QFont buoysFont;

    QString isosStr;

    int mutex;
};

#endif // MAP_H
