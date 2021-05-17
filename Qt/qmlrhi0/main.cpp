#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#ifndef QML_ELEMENT
#include "VideoTextureItem.h"
#endif
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    // env: QSG_RHI=1, QSG_RHI_BACKEND=metal, QSG_INFO=1
    //QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
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
