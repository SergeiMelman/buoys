QT += network widgets serialport

CONFIG += c++11

HEADERS       = \
    ../modemtool/modem.h \
    ../modemtool/modemi.h \
    retranslatormainwindow.h

SOURCES       = \
    ../modemtool/modem.cpp \
    retranslatormainwindow.cpp \
    retranslatormain.cpp
