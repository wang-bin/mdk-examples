QT       += gui quick
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app

MDK_SDK = $$PWD/../mdk-sdk
include($$MDK_SDK/mdk.pri)

SOURCES += $$PWD/qml/qmdkqmlplay.cpp\
           $$PWD/qml/mdkplayer.cpp

HEADERS  += $$PWD/qml/mdkplayer.h

RESOURCES += $$PWD/qml/qmdkqmlplay.qrc
