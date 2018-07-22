QT       += core gui widgets
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app
LIBS += -L$$OUT_PWD -lqtmdk

SOURCES += qtmultiwidgets.cpp

mac {
  RPATHDIR *= @executable_path/Frameworks @loader_path
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}