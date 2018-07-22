/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKWidgetRenderer.h"
#include "QMDKPlayer.h"
#include "mdk/Player.h"
#include <QCoreApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

using namespace mdk;
QMDKWidgetRenderer::QMDKWidgetRenderer(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

QMDKWidgetRenderer::~QMDKWidgetRenderer() = default;

void QMDKWidgetRenderer::setSource(QMDKPlayer* player)
{
    player->addRenderer(this);
    if (player_) {
        player_->removeRenderer(this);
    }
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

void QMDKWidgetRenderer::initializeGL()
{
    // instance is destroyed before aboutToBeDestroyed(), and no current context in aboutToBeDestroyed()
    auto ctx = context();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [ctx]{
        QOffscreenSurface s;
        s.create();
        ctx->makeCurrent(&s);
        Player::foreignGLContextDestroyed();
        ctx->doneCurrent();
    });
}

void QMDKWidgetRenderer::resizeGL(int w, int h)
{
    auto p = player_;
    if (!p)
        return;
    p->addRenderer(this, w, h);
}

void QMDKWidgetRenderer::paintGL()
{
    auto p = player_;
    if (!p)
        return;
    p->renderVideo(this);
}
