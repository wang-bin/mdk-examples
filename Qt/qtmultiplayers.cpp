/*
 * Copyright (c) 2018-2021 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKRenderer.h"
#include "QMDKPlayer.h"
#include <QApplication>
#include <QHBoxLayout>

class MyRenderWidget : public QMDKWidgetRenderer {
public:
    MyRenderWidget(QWidget *parent = nullptr) : QMDKWidgetRenderer(parent) {}
private:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent*) override {
#else
    void enterEvent(QEvent*) override {
#endif
        source()->play();
    }

    void leaveEvent(QEvent*) override {
        source()->stop();
    }
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
    //a.setAttribute(Qt::AA_ShareOpenGLContexts);
    QWidget win;
    win.setWindowTitle("Move mouse on widgets to start");
    win.resize(1920, 300);
    win.show();

    const int count = 9;
    QMDKPlayer player[count];
    MyRenderWidget* w[count];
    auto hb = new QHBoxLayout();
    win.setLayout(hb);
    for (int i = 0; i < count; ++i) {
        w[i] = new MyRenderWidget();
        hb->addWidget(w[i]);
        w[i]->setSource(&player[i]);
    }
    int i = a.arguments().indexOf("-c:v");
    if (i > 0) {
        for (int n = 0; n < count; ++n)
            player[n].setDecoders(a.arguments().at(i+1).split(","));
    }
    for (int n = 0; n < count; ++n)
        player[n].setMedia(a.arguments().last());

    return a.exec();
}
