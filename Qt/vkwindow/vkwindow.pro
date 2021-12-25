QT       += gui
CONFIG += utf8_source
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += $$(VULKAN_SDK)/include

MDK_SDK = $$PWD/../../mdk-sdk
INCLUDEPATH += $$MDK_SDK/include
contains(QT_ARCH, x.*64) {
  android: MDK_ARCH = x86_64
  else:linux: MDK_ARCH = amd64
  else: MDK_ARCH = x64
} else:contains(QT_ARCH, .*86) {
  MDK_ARCH = x86
} else:contains(QT_ARCH, a.*64.*) { # arm64-v8a
  android: MDK_ARCH = arm64-v8a
  else: MDK_ARCH = arm64
} else:contains(QT_ARCH, arm.*) {
  android: MDK_ARCH = armeabi-v7a
  else:linux: MDK_ARCH = armhf
  else: MDK_ARCH = arm
}

macx {
  LIBS += -F$$MDK_SDK/lib -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$MDK_SDK/lib/$$MDK_ARCH -lmdk
  win32: LIBS += -L$$MDK_SDK/bin/$$MDK_ARCH # qtcreator will prepend $$LIBS to PATH to run targets
}
linux: LIBS += -Wl,-rpath-link,$$MDK_SDK/lib/$$MDK_ARCH # for libc++ symbols
linux: LIBS += -Wl,-rpath,$$MDK_SDK/lib/$$MDK_ARCH

SOURCES += main.cpp \
    VkMDKWindow.cpp

HEADERS  += \
    VkMDKWindow.h

mac {
  RPATHDIR *= @executable_path/Frameworks $$MDK_SDK/lib
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
