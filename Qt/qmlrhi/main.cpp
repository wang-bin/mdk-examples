#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#ifndef QML_ELEMENT
#include "VideoTextureItem.h"
#endif
int main(int argc, char **argv)
{
    // TODO: QSurfaceFormat::setDefaultFormat() opengles for rpi, default wayland egl create desktop gl, not compatible with hevc
    QGuiApplication app(argc, argv);
    // env: QSG_RHI=1(qt5), QSG_RHI_BACKEND=opengl, QSG_INFO=1
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    //QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#else
    //QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGLRhi); // OpenGL
#endif
#ifndef QML_ELEMENT
    qmlRegisterType<VideoTextureItem>("MDKTextureItem", 1, 0, "VideoTextureItem");
#endif

    QQuickView view;
    view.connect(&view, &QQuickView::statusChanged, [&](QQuickView::Status s){
        if (s == QQuickView::Ready)
            view.rootObject()->setProperty("url", app.arguments().last());
    });

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/mdktextureitem/main.qml"));
    view.resize(400, 400);
    view.show();
    return app.exec();
}
