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
LIBS += -L$$OUT_PWD -lqtmdk

SOURCES += qtmultiwidgets.cpp

mac {
  RPATHDIR *= @executable_path/Frameworks @loader_path $$MDK_SDK/lib
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
