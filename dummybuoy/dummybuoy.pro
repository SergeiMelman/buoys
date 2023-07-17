QT += core serialport network
QT -= gui

CONFIG += c++11

TARGET = dummybuoy
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ../modemtool/modem.cpp \
    dummibuoy.cpp \
    dummibuoymain.cpp

HEADERS += \
    ../modemtool/modem.h \
    dummibuoy.h
