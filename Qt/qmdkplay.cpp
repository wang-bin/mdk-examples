/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include "QMDKWindow.h"
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QMDKWindow w;
    w.show();
    int i = a.arguments().indexOf("-c:v");
    if (i > 0)
        w.setDecoders(a.arguments().at(i+1).split(","));
    w.setMedia(a.arguments().last());
    w.play();

    return a.exec();
}
