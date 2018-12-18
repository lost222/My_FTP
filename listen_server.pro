#-------------------------------------------------
#
# Project created by QtCreator 2018-12-18T21:57:34
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = listen_server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fake_tcp.cpp

HEADERS  += mainwindow.h \
    fake_tcp.h

FORMS    += mainwindow.ui
