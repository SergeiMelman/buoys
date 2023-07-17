#-------------------------------------------------
#
# Project created by QtCreator 2016-06-24T16:49:10
#
#-------------------------------------------------

QT       += core gui widgets network sql
CONFIG += c++11

TEMPLATE = app


SOURCES +=\
	buoyrequesteripmain.cpp \
	buoyrequesteripmainwindow.cpp \
	server.cpp \
    client.cpp \
    ../modemtool/logger.cpp \
    mytcpserver.cpp \
    database.cpp

HEADERS  += \
	buoyrequesteripmainwindow.h \
	server.h \
    client.h \
    ../modemtool/logger.h \
    mytcpserver.h \
    database.h

FORMS    += \
	buoyrequesteripmainwindow.ui
