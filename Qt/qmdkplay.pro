QT       += gui widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += opengl openglwidgets
qtHaveModule(x11extras): QT += x11extras #libqt5x11extras5-dev
qtHaveModule(androidextras): QT += androidextras

CONFIG += c++17 utf8_source
CONFIG -= app_bundle
TEMPLATE = app

MDK_SDK = $$PWD/../mdk-sdk
include($$MDK_SDK/mdk.pri)

SOURCES += qmdkplay.cpp \
        QMDKWindow.cpp \
        QMDKWidget.cpp

HEADERS  += QMDKWindow.h QMDKWidget.h
