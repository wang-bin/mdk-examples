/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWidget example
 */
#include "QMDKWidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMDKWidget w;
    w.resize(400, 300);
    w.show();
    int i = a.arguments().indexOf("-c:v");
    if (i > 0)
        w.setDecoders(a.arguments().at(i+1).split(","));
    w.setMedia(a.arguments().last());
    w.play();

    return a.exec();
}
