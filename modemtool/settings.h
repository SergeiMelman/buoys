#ifndef MODEMSETTINGS_H
#define MODEMSETTINGS_H

#include <QtSerialPort>

enum STATUSTYPE {IPCONNECTED, COMCONNECTED, DISCONNECTED};

struct ModemSettings {

	enum CONNECTTYPE {IPTYPE = 0, COMTYPE, UNDEFINED};

	ModemSettings()
	{
		name = "com2";
		baudRate = QSerialPort::Baud19200;
		dataBits = QSerialPort::Data8;
		parity = QSerialPort::NoParity;
		stopBits = QSerialPort::OneStop;
		flowControl = QSerialPort::SoftwareControl;

		server = "127.0.0.1";
		port = 8989;

		type = COMTYPE;
	}
	ModemSettings(QString str){fromString(str);}

	QString toString() const;
	bool fromString(QString str);

	// -= VARS =-
	QString name;

	qint32 baudRate;
	//QString stringBaudRate;

	QSerialPort::DataBits dataBits;
	//QString stringDataBits;

	QSerialPort::Parity parity;
	//QString stringParity;

	QSerialPort::StopBits stopBits;
	//QString stringStopBits;

	QSerialPort::FlowControl flowControl;
	//QString stringFlowControl;

	QString server;
	qint16  port;

	CONNECTTYPE type;

};

#define DELIM ';'

inline QString ModemSettings::toString() const
{
	const QLatin1Char delim(DELIM);
	QString str;
	str += name + delim;
	str += QString::number(baudRate) + delim;
	str += QString::number(dataBits) + delim;
	str += QString::number(parity) + delim;
	str += QString::number(stopBits) + delim;
	str += QString::number(flowControl) + delim;

	str += server + delim;
	str += QString::number(port) + delim;

	str += QString::number(type) + delim;

	return str;
}

inline bool ModemSettings::fromString(QString str)
{
	QStringList str_el = str.split(DELIM);

	if(str_el.size() < 9)
	{
		return false;
	}

	name = str_el.at(0);
	baudRate = str_el.at(1).toInt();
	dataBits = (QSerialPort::DataBits)str_el.at(2).toInt();
	parity = (QSerialPort::Parity)str_el.at(3).toInt();
	stopBits = (QSerialPort::StopBits)str_el.at(4).toInt();
	flowControl = (QSerialPort::FlowControl)str_el.at(5).toInt();

	server = str_el.at(6);
	port = str_el.at(7).toInt();

	type = (CONNECTTYPE)str_el.at(8).toInt();

	return true;
}

#undef DELIM

#endif // SERIALSETTINGS_H

