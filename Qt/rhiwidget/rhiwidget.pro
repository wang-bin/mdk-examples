QT += gui-private widgets
CONFIG -= app_bundle

INCLUDEPATH += $$(VULKAN_SDK)/include

HEADERS += RhiVideoWidget.h
SOURCES += RhiVideoWidget.cpp main.cpp

macos|ios: LIBS += -framework Metal
macos: LIBS += -framework AppKit

#macx|ios: QMAKE_CXXFLAGS += -x objective-c++ -fobjc-arc

######## MDK SDK ##########
MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)
