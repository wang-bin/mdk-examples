QT += qml quick
CONFIG += c++17
CONFIG += qmltypes
CONFIG -= app_bundle
QML_IMPORT_NAME = MDKTextureItem
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += VideoTextureItem.h
SOURCES += VideoTextureItem.cpp main.cpp
RESOURCES += mdktextureitem.qrc

macos|ios: LIBS += -framework Metal
macos: LIBS += -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/mdktextureitem
INSTALLS += target

######## MDK SDK ##########
MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)

static|contains(CONFIG, staticlib) {
  DEFINES += Q_MDK_API
} else {
  DEFINES += Q_MDK_API=Q_DECL_EXPORT
}

macx|ios {
  QMAKE_CXXFLAGS += -x objective-c++ -fobjc-arc
}
