/*
 * Copyright (c) 2018-2022 WangBin <wbsecg1 at gmail.com>
 */
#include "videogroup.h"
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
    VideoGroup wall;
    wall.show();
    wall.play(a.arguments().last());

    return a.exec();
}
