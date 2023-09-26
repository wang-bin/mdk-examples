QT       += core gui widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += opengl openglwidgets
qtHaveModule(x11extras): QT += x11extras #libqt5x11extras5-dev
qtHaveModule(androidextras): QT += androidextras
TARGET = qtmdk
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = lib

MDK_SDK = $$PWD/../mdk-sdk
include($$MDK_SDK/mdk.pri)

static|contains(CONFIG, staticlib) {
  DEFINES += Q_MDK_API
} else {
  DEFINES += Q_MDK_API=Q_DECL_EXPORT
}
SOURCES += QMDKRenderer.cpp \
        QMDKPlayer.cpp

HEADERS  += QMDKPlayer.h QMDKRenderer.h
