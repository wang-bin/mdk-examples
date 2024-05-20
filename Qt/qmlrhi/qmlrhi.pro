QT += qml quick
qtHaveModule(x11extras): QT += x11extras #libqt5x11extras5-dev
qtHaveModule(androidextras): QT += androidextras

CONFIG += c++17
CONFIG += qmltypes
CONFIG -= app_bundle
QML_IMPORT_NAME = MDKTextureItem
QML_IMPORT_MAJOR_VERSION = 1

INCLUDEPATH += $$(VULKAN_SDK)/include

HEADERS += VideoTextureItem.h \
    VideoTextureNode.h
SOURCES += VideoTextureItem.cpp main.cpp \
    VideoTextureNode.cpp \
    VideoTextureNodePub.cpp
RESOURCES += mdktextureitem.qrc


greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 14) {
    qtHaveModule(quick-private) {
        QT += quick-private gui-private
        SOURCES += VideoTextureNodePriv.cpp
    }
}


macos|ios: LIBS += -framework Metal
macos: LIBS += -framework AppKit

macx|ios {
  QMAKE_CXXFLAGS += -x objective-c++ -fobjc-arc
}

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/mdktextureitem
INSTALLS += target

######## MDK SDK ##########
MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)
