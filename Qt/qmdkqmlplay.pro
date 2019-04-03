QT       += gui quick
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app
INCLUDEPATH += $$PWD/../mdk-sdk/include

macx {
  LIBS += -F$$PWD/../mdk-sdk/lib -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/../mdk-sdk/lib -lmdk
}

SOURCES += $$PWD/qml/qmdkqmlplay.cpp\
           $$PWD/qml/mdkplayer.cpp

HEADERS  += $$PWD/qml/mdkplayer.h

RESOURCES += $$PWD/qml/qmdkqmlplay.qrc

mac {
  RPATHDIR *= @executable_path/Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
