/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKPlayer.h"
#include "QMDKWidgetRenderer.h"
#include <QApplication>
#include <QFont>
#include <QOpenGLPaintDevice>
#include <QPainter>

class PaintVideoWidget : public QMDKWidgetRenderer {
protected:
    void beforeGL() override {
        dev_.reset(new QOpenGLPaintDevice(width(), height()));
        painter_.begin(dev_.get());
        painter_.beginNativePainting();
    }

    void afterGL() override {
        painter_.endNativePainting();
        painter_.setPen(Qt::green);
        QFont ft;
        ft.setBold(true);
        ft.setPointSize(60);
        painter_.setFont(ft);
        painter_.drawText(contentsRect(), "Qt Paint on MDK OpenGL Video");
        painter_.end();
        dev_.reset();
    }
private:
    std::unique_ptr<QOpenGLPaintDevice> dev_;
    QPainter painter_;
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qDebug("Usage: %s [-c:v dec1,dec2,...] url\n"
                "-c:v: set video decoder list, seperated by comma, decoder can be FFmpeg, VideoToolbox, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi)"
        , argv[0]);
        return 0;
    }
    QApplication a(argc, argv);
    //a.setAttribute(Qt::AA_UseDesktopOpenGL);
    QMDKPlayer player;
    PaintVideoWidget w;
    w.show();
    w.setSource(&player);
    int i = a.arguments().indexOf("-c:v");
    if (i > 0)
        player.setDecoders(a.arguments().at(i+1).split(","));
    player.setMedia(a.arguments().last());
    player.play();

    return a.exec();
}
