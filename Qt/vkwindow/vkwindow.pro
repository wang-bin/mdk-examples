QT       += gui
CONFIG += utf8_source
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += $$(VULKAN_SDK)/include

MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)

SOURCES += main.cpp \
    VkMDKWindow.cpp

HEADERS  += \
    VkMDKWindow.h
