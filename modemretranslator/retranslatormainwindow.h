/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SERVER_H
#define SERVER_H

#include <QDialog>

#include "../modemtool/modemi.h"

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QNetworkSession;
class QSerialPort;
class QPlainTextEdit;
QT_END_NAMESPACE

//! [0]
class RetranslatorMainWindow : public QDialog, public ModemI
{
	Q_OBJECT

public:
	RetranslatorMainWindow(QWidget* parent = 0);

private:
	void addMessageToLogWindow(const QString& s);
	void sendToClient(const QByteArray& d);
	void sendToClient(const QString& s);
	void processPacket(QByteArray packet);

signals:

private slots:
	void networkSessionOpened();
	void serverAcceptError(QAbstractSocket::SocketError);
	void serverClientConnected();
	void serverStartListen();
	void incomingDataSocketHandler();

	virtual void errorModemHandler(const QString& errorString);
	virtual void infoModemHandler(const QString& infoString);
	virtual void incomingDataModemHandler(QByteArray packet);
	virtual void connectedModemHandler();
	virtual void disconnectedModemHandler();
	virtual void bytesWrittenModemHandler(qint64) {}

private:
	QPlainTextEdit* logWindow;

	QNetworkSession* networkSession;
	QTcpServer* tcpServer;
	quint16 serverPort;
	QTcpSocket* tcpClient;

	Modem* modem;
};
//! [0]

#endif
