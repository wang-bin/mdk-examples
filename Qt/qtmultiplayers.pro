QT     += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += opengl openglwidgets

CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app
CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/debug
} else {
    LIBS += -L$$OUT_PWD/release
}
android: LIB_SUFFIX=_$$ANDROID_TARGET_ARCH
LIBS += -L$$OUT_PWD -lqtmdk$$LIB_SUFFIX

SOURCES += qtmultiplayers.cpp
