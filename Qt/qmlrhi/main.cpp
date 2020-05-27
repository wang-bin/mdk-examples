#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    // env: QSG_RHI=1, QSG_RHI_BACKEND=metal, QSG_INFO=1
    //QQuickWindow::setSceneGraphBackend(QSGRendererInterface::MetalRhi);

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