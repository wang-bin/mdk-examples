#include "mdkplayer.h"
#include <QDebug>

class VideoRendererInternal : public QQuickFramebufferObject::Renderer
{
public:
    VideoRendererInternal(QmlMDKPlayer *r) {
        this->r = r;
    }

    void render() override {
        r->renderVideo();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        r->setVideoSurfaceSize(size.width(), size.height());
        return new QOpenGLFramebufferObject(size);
    }

    QmlMDKPlayer *r;
};


QmlMDKPlayer::QmlMDKPlayer(QQuickItem *parent):
    QQuickFramebufferObject(parent),
    internal_player(new Player)
{
    setMirrorVertically(true);
}

QmlMDKPlayer::~QmlMDKPlayer()
{
    delete internal_player;
}

QQuickFramebufferObject::Renderer *QmlMDKPlayer::createRenderer() const
{
    return new VideoRendererInternal(const_cast<QmlMDKPlayer*>(this));
}

void QmlMDKPlayer::play()
{
    internal_player->setState(PlaybackState::Playing);
    internal_player->setRenderCallback([=](void *){
        QMetaObject::invokeMethod(this, "update");
    });
}

void QmlMDKPlayer::setPlaybackRate(float rate)
{
    internal_player->setPlaybackRate(rate);
}

void QmlMDKPlayer::setVideoSurfaceSize(int width, int height)
{
    internal_player->setVideoSurfaceSize(width, height);
}

void QmlMDKPlayer::renderVideo()
{
    internal_player->renderVideo();
}
