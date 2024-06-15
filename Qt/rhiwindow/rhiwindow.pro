QT       += gui-private

CONFIG += c++17
INCLUDEPATH += $$(VULKAN_SDK)/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    RhiVideoWindow.cpp

HEADERS += \
    RhiVideoWindow.h


macx|ios {
  #QMAKE_CXXFLAGS += -x objective-c++ -fobjc-arc
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

######## MDK SDK ##########
MDK_SDK = $$PWD/../../mdk-sdk
include($$MDK_SDK/mdk.pri)

