#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T10:25:13
#
#-------------------------------------------------

QT       += core gui widgets sql printsupport
CONFIG += c++11

TEMPLATE = app

SOURCES += main.cpp\
		mainwindow.cpp \
	FilterLineEdit.cpp \
	FilterTableHeader.cpp \
	buoysdata.cpp \
	../qcustomplot/qcustomplot.cpp \
	graphs.cpp \
	parameterwiget.cpp \
	map.cpp

HEADERS  += mainwindow.h \
	FilterLineEdit.h \
	FilterTableHeader.h \
	buoysdata.h \
	../qcustomplot/qcustomplot.h \
	graphs.h \
	parameterwiget.h \
	map.h \
	buoydata.h \
	rbfinterpolator.h

FORMS    += mainwindow.ui \
	graphs.ui \
	parameterwiget.ui

#Eigen
INCLUDEPATH += "../Eigen3"
