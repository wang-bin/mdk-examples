/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWidget example
 */
#include "QMDKWidget.h"
#include <QApplication>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMDKWidget w;
    w.resize(400, 300);
    w.show();
    auto preview = new QMDKWidget(&w);
    preview->resize(192, 108);
    preview->show();
    a.connect(&w, &QMDKWidget::mouseMoved, [&](int x, int y){
        qDebug() << "mouse move " << x << ", " << y;

        const auto pos = w.duration() * x / w.width();
        preview->seek(pos, true);
        preview->move(x - preview->width()/2, y - preview->height());
    });
    a.connect(preview, &QMDKWidget::doubleClicked, [&]{
        w.seek(preview->position(), true);
    });
    int i = a.arguments().indexOf("-c:v");
    if (i > 0)
        w.setDecoders(a.arguments().at(i+1).split(","));
    w.setMedia(a.arguments().last());
    preview->setMedia(a.arguments().last());
    w.play();
    preview->prepreForPreview();

    QTimer t;
    t.setInterval(100);
    t.callOnTimeout([&]{
        const auto pos = w.position()/1000;
        w.setWindowTitle(QString("%1:%2:%3").arg(pos/3600, 2, 10, QLatin1Char('0')).arg((pos%3600) / 60, 2, 10, QLatin1Char('0')).arg(pos % 60, 2, 10, QLatin1Char('0')));
    });
    t.start();
    return a.exec();
}
