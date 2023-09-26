QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += opengl openglwidgets
qtHaveModule(x11extras): QT += x11extras #libqt5x11extras5-dev

CONFIG += c++17
CONFIG -= app_bundle

HEADERS += GraphicsVideoItem.h
SOURCES += GraphicsVideoItem.cpp main.cpp


######## MDK SDK ##########
MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)

static|contains(CONFIG, staticlib) {
  DEFINES += Q_MDK_API
} else {
  DEFINES += Q_MDK_API=Q_DECL_EXPORT
}
