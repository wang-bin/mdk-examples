QT       += gui
CONFIG += c++17 utf8_source
CONFIG -= app_bundle
TEMPLATE = app
INCLUDEPATH += $$PWD/../mdk-sdk/include
contains(QT_ARCH, x.*64) {
  android: MDK_ARCH = x86_64
  else: MDK_ARCH = x64
} else:contains(QT_ARCH, .*86) {
  MDK_ARCH = x86
} else:contains(QT_ARCH, a.*64) {
  android: MDK_ARCH = arm64-v8a
  else: MDK_ARCH = arm64
} else:contains(QT_ARCH, arm.*) {
  android: MDK_ARCH = armeabi-v7a
  else: MDK_ARCH = arm
}

macx {
  LIBS += -F$$PWD/../mdk-sdk/lib -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/../mdk-sdk/lib/$$MDK_ARCH -lmdk
  win32: LIBS += -L$$PWD/../../mdk-sdk/bin/$$MDK_ARCH # qtcreator will prepend $$LIBS to PATH to run targets
}
linux: LIBS += -Wl,-rpath-link,$$PWD/../mdk-sdk/lib/$$MDK_ARCH # for libc++ symbols

SOURCES += qmdkplay.cpp\
        QMDKWindow.cpp

HEADERS  += QMDKWindow.h

mac {
  RPATHDIR *= @executable_path/Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
