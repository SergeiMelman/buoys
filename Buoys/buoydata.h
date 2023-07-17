#ifndef BUOYDATA_H
#define BUOYDATA_H

#include <QMapIterator>
#include <QPointF>
#include <QSharedPointer>
#include <limits>

class QStandardItemModel;

struct BuoyData
{
	// 0 - min, 1 - max, 2 - everage
	double bat_v[3];
	double sensor1[3];
	double sensor2[3];
	double sensor3[3];
	double bitrate[3];
	double RSSI[3];
	QPointF geoPos;
	QSharedPointer<QStandardItemModel> model;

	BuoyData()
	{
		bat_v[0] = sensor1[0] = sensor2[0] = sensor3[0] = bitrate[0] = RSSI[0] =
				std::numeric_limits<double>::max();
		bat_v[1] = sensor1[1] = sensor2[1] = sensor3[1] = bitrate[1] = RSSI[1] =
				-std::numeric_limits<double>::max();
		bat_v[2] = sensor1[2] = sensor2[2] = sensor3[2] = bitrate[2] = RSSI[2] =
				0;
	}
};

typedef QMap<QString, BuoyData> MapBuoyData;
typedef QMapIterator<QString, BuoyData> MapBuoyDataIterator;

#endif // BUOYDATA_H
