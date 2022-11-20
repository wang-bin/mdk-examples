#include "GraphicsVideoItem.h"
#include "mdk/Player.h"
#include <QPainter>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QOpenGLContext>
#include <QOffscreenSurface>

using namespace std;
using namespace MDK_NS;

GraphicsVideoItem::GraphicsVideoItem(QGraphicsItem * parent)
    : QGraphicsWidget(parent)
    , player_(make_shared<Player>())
{
    setFlag(ItemIsFocusable); //receive key events

    player_->setDecoders(MediaType::Video, {
#if (__APPLE__+0)
        "VT",
#elif (__ANDROID__+0)
        "AMediaCodec:java=0:copy=0:surface=1:async=0",
#elif (_WIN32+0)
        "MFT:d3d=11",
        "CUDA",
        "D3D11",
        "DXVA",
#elif (__linux__+0)
        "VAAPI",
        "VDPAU",
        "CUDA",
#endif
        "FFmpeg"});
    player_->setRenderCallback([this](void*){
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    });
}

GraphicsVideoItem::~GraphicsVideoItem()
{
    delete fbo_; // FIXME: current context
}

void GraphicsVideoItem::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    for (const auto& d : dec) {
        v.push_back(d.toStdString());
    }
    player_->setDecoders(MediaType::Video, v);
}

void GraphicsVideoItem::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void GraphicsVideoItem::play()
{
    player_->set(State::Playing);
}

void GraphicsVideoItem::pause()
{
    player_->set(State::Paused);
}

void GraphicsVideoItem::stop()
{
    player_->set(State::Stopped);
}

bool GraphicsVideoItem::isPaused() const
{
    return player_->state() == State::Paused;
}

void GraphicsVideoItem::seek(qint64 ms, bool accurate)
{
    auto flags = SeekFlag::FromStart;
    if (!accurate)
        flags |= SeekFlag::KeyFrame;
    player_->seek(ms, flags);
}

qint64 GraphicsVideoItem::position() const
{
    return player_->position();
}

qint64 GraphicsVideoItem::duration() const
{
    return player_->mediaInfo().duration;
}

void GraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    // save painter state, switch to native opengl painting
    painter->save();
    painter->beginNativePainting();

    const auto newSize = boundingRect().size().toSize();
    if (!fbo_ || fboSize_ != newSize) {
        fboSize_ = newSize;
        delete fbo_;
        fbo_ = new QOpenGLFramebufferObject(fboSize_);
        GLRenderAPI ra;
        ra.fbo = fbo_->handle();
        player_->setRenderAPI(&ra, this);
        player_->setVideoSurfaceSize(fboSize_.width(), fboSize_.height(), this);
    }
    if (!tb_) {
        tb_ = new QOpenGLTextureBlitter();
        tb_->create();

        std::weak_ptr<Player> wp = player_;
        const auto ctx = QOpenGLContext::currentContext();
        auto tb = tb_;
        auto fbo = fbo_;
        connect(ctx, &QOpenGLContext::aboutToBeDestroyed, [=]{
            QOffscreenSurface s;
            s.create();
            ctx->makeCurrent(&s);
            auto sp = wp.lock();
            if (sp) // release and remove old gl resources with the same vo_opaque(nullptr), then new resource will be created in resizeGL/paintGL
                sp->setVideoSurfaceSize(-1, -1, this); // it's better to cleanup gl renderer resources as early as possible
            else
                Player::foreignGLContextDestroyed();

            tb->destroy();
            delete tb;
            ctx->doneCurrent();
        });
    }

    player_->renderVideo(this);

    tb_->bind();
    tb_->blit(fbo_->texture(), sceneTransform(), QOpenGLTextureBlitter::OriginBottomLeft);
    tb_->release();

    // end native painting, restore state
    painter->endNativePainting();
    painter->restore();
}


bool GraphicsVideoItem::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        scene()->update(sceneBoundingRect());
    } else {
        return QGraphicsWidget::event(e);
    }
    return true;
}
