#-------------------------------------------------
#
# Project created by QtCreator 2016-04-21T17:37:05
#
#-------------------------------------------------

QT       += core gui serialport network sql
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = buoyrequester
TEMPLATE = app


SOURCES += \
	../modemtool/modem.cpp \
	../modemtool/logger.cpp \
	../modemtool/settingsdialog.cpp \
	scheduler.cpp \
    buoyrequestermain.cpp \
    buoyrequestermainwindoww.cpp

HEADERS  += \
	../modemtool/modem.h \
	../modemtool/logger.h \
	../modemtool/modemi.h \
	../modemtool/settings.h \
	../modemtool/settingsdialog.h \
	scheduler.h \
    buoyrequestermainwindow.h

FORMS    += \
	../modemtool/settingsdialog.ui \
    buoyrequestermainwindow.ui

RESOURCES += \
	../modemtool/modemtool.qrc

DISTFILES +=
