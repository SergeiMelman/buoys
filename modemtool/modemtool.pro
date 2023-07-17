#-------------------------------------------------
#
# Project created by QtCreator 2016-03-21T14:24:18
#
#-------------------------------------------------

QT       += core gui widgets serialport network printsupport

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = modemtool
TEMPLATE = app


SOURCES +=\
    settingsdialog.cpp \
    console.cpp \
    modem.cpp \
    logger.cpp \
    modemtoolmainwindow.cpp \
    ../qcustomplot/qcustomplot.cpp \
    modemtoolmain.cpp

HEADERS  += \
    settingsdialog.h \
    console.h \
    modem.h \
    settings.h \
    modemi.h \
    logger.h \
    modemtoolmainwindow.h \
    ../qcustomplot/qcustomplot.h

FORMS    += \
    settingsdialog.ui \
    modemtoolmainwindow.ui

RESOURCES += \
    modemtool.qrc

DISTFILES +=
