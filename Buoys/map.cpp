#include "map.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QTextStream>
#include <QMessageBox>

#include "rbfinterpolator.h"

Map::Map(QWidget *parent)
	: QWidget(parent)
	, geoToPixel(Eigen::Matrix3d::Identity())
	, mapVisibleScale(1)
	, mutex(0)
{

}

void Map::loadMap(QString name)
{
	// очистить карту и изолинии
	map = QImage();
	mapIsolines = QImage();
	map.load(name);
	if(map.isNull())
	{
		QMessageBox::information(this, tr("Information"), tr("Can't open map!"));
		return;
	}

	Eigen::Matrix3d pixelToGeo = Eigen::Matrix3d::Identity();

	QFile wldf(name + 'w');
	wldf.open(QIODevice::ReadOnly | QIODevice::Text);

	if(wldf.isOpen())
	{
		QTextStream wld(&wldf);
		wld >> pixelToGeo(0,0);
		wld >> pixelToGeo(0,1);
		wld >> pixelToGeo(0,2);
		wld >> pixelToGeo(1,0);
		wld >> pixelToGeo(1,1);
		wld >> pixelToGeo(1,2);
	}
	geoToPixel = pixelToGeo.inverse();

	mapVisibleTopLeft = QPointF(-map.width()/2.0, -map.height()/2.0);
	mapVisibleScale = 1;

	update();
}

void Map::saveMap(QString name)
{
	// если загружена карта и построены изолинии
	if(!map.isNull() && !mapIsolines.isNull())
	{
		// запомнить масштаб и сдвиг
		qreal mapVisibleScale_ = mapVisibleScale;
		QPointF mapVisibleTopLeft_ = mapVisibleTopLeft;
		// сбросить масштаб и сдвиг
		mapVisibleScale = 1;
		mapVisibleTopLeft = QPointF(0,0);

		QImage image(map.size(), QImage::Format_ARGB32);
		QPainter painter(&image);
		drawAll(painter);

		// впечатать строку по кот строили изолинии.
		painter.drawText(20, 20, isosStr);

		// вернуть масштаб и сдвиг
		mapVisibleScale = mapVisibleScale_;
		mapVisibleTopLeft = mapVisibleTopLeft_;

		const bool rez = image.save(name);
		if(!rez)
		{
			QMessageBox::critical(this, "Error",
								  "Can't save isolines map.");
		}
	}
}

void Map::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	drawAll(painter);

	QWidget::paintEvent(event);
}

void Map::drawAll(QPainter &painter)
{
	// нарисовать карту
	painter.drawImage(QRect(mapVisibleTopLeft.toPoint(),
							map.size() * mapVisibleScale), map);
	// нарисовать изолинии
	painter.drawImage(QRect(mapVisibleTopLeft.toPoint(),
							map.size() * mapVisibleScale), mapIsolines);

	// нарисовать буйки и их имена
	drawBuoys(painter);
}

QPointF Map::toPixel(QPointF p)
{
	Eigen::Vector3d geo(p.x(), p.y(), 1);
	Eigen::Vector3d pix = geoToPixel * geo;
	return QPointF(pix(0), pix(1));
}

void Map::drawBuoys(QPainter& painter)
{
	MapBuoyDataIterator it(*buoys);
	while(it.hasNext())
	{
		const QString& name = it.next().key();
		const BuoyData& data = it.value();
		const QPointF pixPos = mapVisibleTopLeft +
							   toPixel(data.geoPos) * mapVisibleScale;
		painter.setPen(QPen(Qt::red, 3));
		painter.setBrush(QBrush(Qt::green));
		painter.drawEllipse(pixPos, 5, 5);

		painter.setFont(buoysFont);
		//painter.setPen(QPen(Qt::white));
		painter.setPen(QPen(Qt::black));
		painter.drawText(pixPos + QPointF(1,1), name);
		painter.drawText(pixPos + QPointF(-1,-1), name);
		//painter.setPen(QPen(Qt::black));
		painter.setPen(QPen(Qt::white));
		painter.drawText(pixPos, name);
	}
}

void Map::calcMapIsolines(const ParametersIsolines& allIsolines)
{
	// если карта не загружена выйти
	if(mutex || map.isNull() || buoys->isEmpty())
	{
		return;
	}
	mutex = 1;
	// создать карту изолиний размером с карту
	mapIsolines = QImage(map.size(), QImage::Format_ARGB32/*_Premultiplied*/);
	mapIsolines.fill(QColor(0,0,0,0)); // заполнить нулями
	// подготовить интерполятор
	// на 2D поле интерполируется вектор 18 параметров
	// тип данных: на 2D поле задан вектор 18 double параметров
	typedef RBFInterpolator<double, 2, 18> RBF;
	RBF rbf; // интерполятор
	// перебрать данные всех буи
	for(const BuoyData& buoy: buoys->values())
	{
		// создавать RBF будем в пиксельной системе координат mapIsolines
		const QPointF pix = toPixel(buoy.geoPos);
		const RBF::Point p(pix.x(), pix.y()); //
		// ВНИМАНИЕ! ХАК! должно работать для всех парам-ов не только bat_v.
		// просто он первый а все остальные данные расположены за ней
		// получается 18 подряд double-ов
		// Eigen::Map умеет их превращать в вектор данных
		const RBF::Data d = Eigen::Map<const RBF::Data>(buoy.bat_v);
		// добавить точку (false - без перерасчета)
		rbf.add(p, d, false);
	}
	// после добавления всех точек рассчитать интерполятор
	rbf.calculate();
	// рассчитать изолинни используя интерполятор
	// Пройтись по всем пикселям
	for(int y = 0; y < mapIsolines.height(); ++y)
	{
		for(int x = 0; x < mapIsolines.width(); ++x)
		{
//            mapIsolines.setPixel(x, y, qRgba(255, 0, 0, 100)); // test
			// помним что RBF в сист коорд mapIsolines
			// интерполировать все 18 параметров в точке x,y
			const RBF::Data res = rbf(RBF::Point(x, y));
			int  par = 0; // номер обрабатываемого параметра
			// перебрать все параметры
			for(const ParameterIsolines& parIsos : allIsolines)
			{
				// перебрать все изолинни для параметра
				for(const ParameterIsoline& iso : parIsos)
				{
					// если значение изолинии "почти" равно интерполированному
					if(qAbs(iso.level - res[par]) < iso.width)
					{
						// закрасить этот пиксель цветом изоликии
						mapIsolines.setPixel(x, y, iso.color);
					}
				}
				++par;
			}
		}
		if(y%20 == 0) // иногда отрисовывать результат
		{
			update(); // перерисовать окошко
			qApp->processEvents();
		}
	}
	mutex = 0;
}

void Map::mousePressEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		grabMouse();
		mouseDown = event->pos();
		event->accept();
	}
}

void Map::mouseReleaseEvent(QMouseEvent* event)
{
	releaseMouse();
	event->accept();
}

void Map::mouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		mapVisibleTopLeft += event->pos() - mouseDown;
		mouseDown = event->pos();
		update();
		event->accept();
	}
}

void Map::wheelEvent(QWheelEvent* event)
{
	const qreal delta = event->delta() / 1000.0;
	mapVisibleScale += mapVisibleScale * delta;
	mapVisibleTopLeft -= (event->pos() - mapVisibleTopLeft) * delta;

	update();
	event->accept();
}

void Map::setBuoysFont(const QFont &value)
{
	// сохраним размер шрифта
	int i = buoysFont.pointSize();
	buoysFont = value;
	buoysFont.setPointSize(i);
	update();
}

void Map::setBuoysFontSize(int i)
{
	buoysFont.setPointSize(i);
	update();
}

void Map::setBuoys(MapBuoyData* value)
{
	buoys = value;
}

void Map::setAllIsolines(QString str)
{
	// разбор командной строки изолиний
	// пример строки: sensor1-min/0.2/255,0,0,255/93;
	// sensor1     имя параметра см. список NAMENUM ниже
	// -           дефис разделяем имя параметра и min max eve
	// min         min max eve
	// разделитель "/"
	// 0.2         толщина изолинии
	// разделитель "/"
	// 255,0,0,255 цвет RGBA через запятую от 0 до 255.
	// разделитель "/"
	// 93          уровень изолинии на поле значений
	// ;           разделитель изолиний

	const static QMap<QString, int> NAMENUM({{"bat_v", 0},{"sensor1", 1},{"sensor2", 2},
											 {"sensor3", 3},{"bitrate", 4},{"RSSI", 5}});
	const static QMap<QString, int> MINMAXEVE({{"min", 0},{"max", 1},{"eve", 2}});

	// оставить строку для построения изолиний на случай сохранения карты с изолиниями на диск
	isosStr = str;

	ParametersIsolines allIsolines(6 * 3); // 6 параметров по 3 значения (min max eve)
	const QStringList paramsSet = str.remove('\n').remove(' ').split(';');

	for(const QString& paramSet : paramsSet)
	{
		ParameterIsoline isoline;
		const QStringList paramParts = paramSet.split('/');
		isoline.width = paramParts.value(1, "1").toDouble(); // width
		// color
		const QStringList color = paramParts.value(2,"0,0,0,255").split(",");
		isoline.color = qRgba(color.value(0,"0").toInt(),  // R
							  color.value(1,"0").toInt(),  // G
							  color.value(2,"0").toInt(),  // B
							  color.value(3,"255").toInt());// A
		// level
		isoline.level = paramParts.value(3, "0").toDouble();
		// имяпараметра-минмакссред
		const QStringList namemm = paramParts.value(0).split('-');
		const int par = NAMENUM.value(namemm.value(0), 6); // number of parameter name, 6 - unrecognized
		const int add = MINMAXEVE.value(namemm.value(1), 2); // min max eve - по умолчанию eve
		if(par < 6)
		{
			// если имя параметра написано правильно, то добавить в изолинии
			allIsolines[par * 3 + add].append(isoline);
		}
	}
	calcMapIsolines(allIsolines);
}

