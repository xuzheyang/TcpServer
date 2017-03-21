QT += core network
QT -= gui

CONFIG += c++11

TARGET = TcpServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    testclient.cpp

HEADERS += \
    tcpserver.hpp \
    tcpclient.hpp
