/*
 * Copyright (c) 2018-2025 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include "QMDKPlayer.h"
#include <QCoreApplication>
#include <QStringList>
#include <QtDebug>
#include <QRegion>
#include <QVariant>
#include "mdk/Player.h"

using namespace MDK_NS;
QMDKPlayer::QMDKPlayer(QObject *parent)
    : QObject(parent)
    , player_(new Player())
{
    player_->setRenderCallback([](void* vo_opaque){
        auto vo = reinterpret_cast<QObject*>(vo_opaque);
        if (vo->property("vsync").toBool())
            return;
        if (!vo->isWidgetType()) { // isWidgetType() is fastest, and no need to include <QWidget>
            if (vo->isWindowType())
                QCoreApplication::instance()->postEvent(vo, new QEvent(QEvent::UpdateRequest));
            return;
        }
        QMetaObject::invokeMethod(vo, "update", Qt::QueuedConnection);
#if 0
        class QUpdateLaterEvent final : public QEvent {
        public:
            explicit QUpdateLaterEvent(const QRegion& paintRegion)
                : QEvent(UpdateLater), m_region(paintRegion)
            {}
            ~QUpdateLaterEvent() {}
            inline const QRegion &region() const { return m_region; }
        protected:
            QRegion m_region;
        };
        QCoreApplication::instance()->postEvent(vo, new QUpdateLaterEvent(QRegion(0, 0, vo->property("width").toInt(), vo->property("height").toInt())));
#endif
    });

    player_->onMediaStatus([this](MediaStatus oldVal, MediaStatus newVal) {
        if (flags_added(oldVal, newVal, MediaStatus::Loaded)) {
            const auto& info = player_->mediaInfo();
            if (!info.video.empty()) {
                const auto& v = info.video[0].codec;
                emit videoSizeChanged({v.width, v.height});
            }
        }
        return true;
    });
}

QMDKPlayer::~QMDKPlayer() = default;

void QMDKPlayer::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    player_->setDecoders(MediaType::Video, v);
}

void QMDKPlayer::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void QMDKPlayer::play()
{
    player_->set(State::Playing);
}

void QMDKPlayer::pause(bool value)
{
    if (value)
        player_->set(State::Paused);
    else
        play();
}

void QMDKPlayer::stop()
{
    player_->set(State::Stopped);
}

bool QMDKPlayer::isPaused() const
{
    return player_->state() == State::Paused;
}

void QMDKPlayer::seek(qint64 ms)
{
    player_->seek(ms);
}

qint64 QMDKPlayer::position() const
{
    return player_->position();
}

void QMDKPlayer::addRenderer(QObject* vo, int w, int h)
{
    updateRenderer(vo, w, h);
    connect(vo, &QObject::destroyed, this, [this](QObject* obj){
        player_->setVideoSurfaceSize(-1, -1, obj); // remove vo
    }, Qt::DirectConnection);
}

void QMDKPlayer::updateRenderer(QObject* vo, int w, int h)
{
    if (w <= 0)
        w = vo->property("width").toInt() * qMax(vo->property("devicePixelRatio").toInt(), 1);
    if (h <= 0)
        h = vo->property("height").toInt() * qMax(vo->property("devicePixelRatio").toInt(), 1);
    player_->setVideoSurfaceSize(w, h, vo); // call update cb
}

void QMDKPlayer::setRenderAPI(RenderAPI* ra, QObject* vo)
{
    player_->setRenderAPI(ra, vo);
}

void QMDKPlayer::renderVideo(QObject* vo)
{
    player_->renderVideo(vo);
}

void QMDKPlayer::destroyGLContext(QObject* vo)
{
    player_->setVideoSurfaceSize(-1, -1, vo); // it's better to cleanup gl renderer resources
}

void QMDKPlayer::setROI(QObject* vo, const float* videoRoi, const float* viewportRoi)
{
    player_->setPointMap(videoRoi, viewportRoi, 2, vo);

}
