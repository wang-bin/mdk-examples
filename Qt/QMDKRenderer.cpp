/*
 * Copyright (c) 2018-2023 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKRenderer.h"
#include "QMDKPlayer.h"
#include "mdk/Player.h"
#include <QCoreApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QGuiApplication>
#if __has_include(<QX11Info>)
#include <QX11Info>
#endif
#if defined(Q_OS_ANDROID)
# if __has_include(<QAndroidJniEnvironment>)
#   include <QAndroidJniEnvironment>
# endif
# if __has_include(<QtCore/QJniEnvironment>)
#   include <QtCore/QJniEnvironment>
# endif
#endif
#include <mutex>

using namespace mdk;

static void InitEnv()
{
#ifdef QX11INFO_X11_H
    SetGlobalOption("X11Display", QX11Info::display());
    qDebug("X11 display: %p", QX11Info::display());
#elif (QT_FEATURE_xcb + 0 == 1) && (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    const auto x = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (x) {
        const auto xdisp = x->display();
        SetGlobalOption("X11Display", xdisp);
        qDebug("X11 display: %p", xdisp);
    }
#endif
#ifdef QJNI_ENVIRONMENT_H
    SetGlobalOption("JavaVM", QJniEnvironment::javaVM());
#endif
#ifdef QANDROIDJNIENVIRONMENT_H
    SetGlobalOption("JavaVM", QAndroidJniEnvironment::javaVM());
#endif
}

static std::once_flag initFlag;

QMDKWindowRenderer::QMDKWindowRenderer(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
{
    std::call_once(initFlag, InitEnv);
}

QMDKWindowRenderer::~QMDKWindowRenderer() = default;

void QMDKWindowRenderer::setSource(QMDKPlayer* player)
{
    setProperty("devicePixelRatio", devicePixelRatio());
    player->addRenderer(this);
    struct NoDeleter {
        void operator()(QMDKPlayer*) {}
    };
    player_.reset(player, NoDeleter());
    if (player) {
        // should skip rendering if player object is destroyed
        connect(player, &QObject::destroyed, [this](QObject* obj){
            auto p = static_cast<QMDKPlayer*>(obj); // why qobject_cast is null?
            if (player_.get() == p)
                player_.reset();
        });
    }
}

void QMDKWindowRenderer::setROI(const float* videoRoi, const float* viewportRoi)
{
    player_->setROI(this, videoRoi, viewportRoi);
}

void QMDKWindowRenderer::initializeGL()
{
    // instance is destroyed before aboutToBeDestroyed(), and no current context in aboutToBeDestroyed()
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [=]{
        makeCurrent();
        if (player_)
            player_->destroyGLContext(this); // it's better to cleanup gl renderer resources
        doneCurrent();
    });
}

void QMDKWindowRenderer::resizeGL(int w, int h)
{
    auto p = player_;
    if (!p)
        return;
    p->addRenderer(this, w*devicePixelRatio(), h*devicePixelRatio());
}

void QMDKWindowRenderer::paintGL()
{
    auto p = player_;
    if (!p)
        return;
    beforeGL();
    player_->renderVideo(this);
    afterGL();
}

#ifdef QT_WIDGETS_LIB
QMDKWidgetRenderer::QMDKWidgetRenderer(QWidget *parent)
    : QOpenGLWidget(parent)
{
    std::call_once(initFlag, InitEnv);
}

QMDKWidgetRenderer::~QMDKWidgetRenderer() = default;

void QMDKWidgetRenderer::setSource(QMDKPlayer* player)
{
    setProperty("devicePixelRatio", devicePixelRatio());
    player->addRenderer(this); // use QWidget* instead of QOpenGLContext* as vo_opaque must ensure context() will not change, e.g. no reparenting the widget via setParent()
    player_ = player;
    if (player) {
        // should skip rendering if player object is destroyed
        connect(player, &QObject::destroyed, [this](QObject* obj){
            auto p = reinterpret_cast<QMDKPlayer*>(obj); // why qobject_cast is null? destroying and sub class dtor is finished?
            if (player_ == p)
                player_ = nullptr;
        });
    }
}

void QMDKWidgetRenderer::setROI(const float* videoRoi, const float* viewportRoi)
{
    player_->setROI(this, videoRoi, viewportRoi);
}

void QMDKWidgetRenderer::initializeGL()
{
    // instance is destroyed before aboutToBeDestroyed(), and no current context in aboutToBeDestroyed()
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [=]{
        makeCurrent();
        if (player_) // release and remove old gl resources with the same vo_opaque(this), then new resource will be created in resizeGL/paintGL
            player_->destroyGLContext(this); // it's better to cleanup gl renderer resources
        else // player was destroyed before context, gl resources are still alive
            Player::foreignGLContextDestroyed();
        doneCurrent();
    });
}

void QMDKWidgetRenderer::resizeGL(int w, int h)
{
    if (!player_) // TODO: not safe. lock? but if player qobject is destroying, player dtor is finished. use true shared_ptr?
        return;
    player_->addRenderer(this, w*devicePixelRatio(), h*devicePixelRatio());
}

void QMDKWidgetRenderer::paintGL()
{
    if (!player_) // safe, if player is destroyed, no update callback
        return;
    beforeGL();
    player_->renderVideo(this);
    afterGL();
}
#endif // QT_WIDGETS_LIB
