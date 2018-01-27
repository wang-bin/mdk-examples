#-------------------------------------------------
#
# Project created by QtCreator 2017-03-21T21:08:15
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app
INCLUDEPATH += $$PWD/mdk-sdk/include
macx {
  LIBS += -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/mdk-sdk/lib -lmdk
}

SOURCES += main.cpp\
        QMDKWindow.cpp

HEADERS  += QMDKWindow.h
