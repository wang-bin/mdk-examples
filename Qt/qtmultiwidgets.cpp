/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKWindowRenderer.h"
#include "QMDKWidgetRenderer.h"
#include "QMDKPlayer.h"
#include <QApplication>

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
    //a.setAttribute(Qt::AA_ShareOpenGLContexts);
    QMDKPlayer player;
    QMDKWidgetRenderer w[4];
    for (auto& i : w) {
        i.show();
        i.setSource(&player);
    }
    int i = a.arguments().indexOf("-c:v");
    if (i > 0)
        player.setDecoders(a.arguments().at(i+1).split(","));
    player.setMedia(a.arguments().last());
    player.play();

    return a.exec();
}
