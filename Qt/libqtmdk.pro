QT       += core gui widgets
TARGET = qtmdk
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = lib
INCLUDEPATH += $$PWD/../mdk-sdk/include
static|contains(CONFIG, staticlib) {
  DEFINES += Q_MDK_API
} else {
  DEFINES += Q_MDK_API=Q_DECL_EXPORT
}
macx {
  QMAKE_CXXFLAGS += -F$$PWD/../mdk-sdk/lib
  LIBS += -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/../mdk-sdk/lib -lmdk
}

SOURCES += QMDKRenderer.cpp \
        QMDKPlayer.cpp

HEADERS  += QMDKPlayer.h QMDKRenderer.h

mac {
  RPATHDIR *= @executable_path/Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}