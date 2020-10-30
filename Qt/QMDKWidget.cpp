/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWidget example
 */
#include "QMDKWidget.h"
#include "mdk/Player.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStringList>
#include <QScreen>

using namespace MDK_NS;
QMDKWidget::QMDKWidget(QWidget *parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
    , player_(std::make_shared<Player>())
{
    player_->setVideoDecoders({"VT", "VAAPI", "MFT:d3d=11", "DXVA", "MMAL", "AMediaCodec:java=1:copy=0:surface=1:async=0", "FFmpeg"});
    player_->setRenderCallback([this](void*){
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    });
}

QMDKWidget::~QMDKWidget() = default; // TODO: release gfx resources in an offscreen context

void QMDKWidget::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    player_->setVideoDecoders(v);
}

void QMDKWidget::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void QMDKWidget::play()
{
    player_->setState(State::Playing);
}

void QMDKWidget::pause()
{
    player_->setState(State::Paused);
}

void QMDKWidget::stop()
{
    player_->setState(State::Stopped);
}

bool QMDKWidget::isPaused() const
{
    return player_->state() == State::Paused;
}

void QMDKWidget::seek(qint64 ms)
{
    player_->seek(ms);
}

qint64 QMDKWidget::position() const
{
    return player_->position();
}

void QMDKWidget::snapshot() {
    Player::SnapshotRequest sr{};
    player_->snapshot(&sr, [](Player::SnapshotRequest *_sr, double frameTime) {
        const QString path = QDir::toNativeSeparators(
            QString("%1/%2.png")
                .arg(QCoreApplication::applicationDirPath())
                .arg(frameTime));
        return path.toStdString();
        // Here's how to convert SnapshotRequest to QImage and save it to disk.
        /*if (_sr) {
            const QImage img = QImage(_sr->data, _sr->width, _sr->height,
                                      QImage::Format_RGBA8888);
            if (img.save(path)) {
                qDebug() << "Snapshot saved:" << path;
            } else {
                qDebug() << "Failed to save:" << path;
            }
        } else {
            qDebug() << "Snapshot failed.";
        }
        return "";*/
    });
}

void QMDKWidget::initializeGL()
{
    auto player = player_;
    // instance is destroyed before aboutToBeDestroyed(), and no current context in aboutToBeDestroyed()
    auto ctx = context();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [player, ctx] {
        QOffscreenSurface s;
        s.create();
        ctx->makeCurrent(&s);
        player->setVideoSurfaceSize(-1, -1); // it's better to cleanup gl renderer resources
        ctx->doneCurrent();
    });
}

void QMDKWidget::resizeGL(int w, int h)
{
    auto s = screen();
    qDebug("resizeGL>>>>>dpr: %f, logical dpi: (%f,%f), phy dpi: (%f,%f)", s->devicePixelRatio(), s->logicalDotsPerInchX(), s->logicalDotsPerInchY(), s->physicalDotsPerInchX(), s->physicalDotsPerInchY());
    player_->setVideoSurfaceSize(w*devicePixelRatio(), h*devicePixelRatio());
}

void QMDKWidget::paintGL()
{
    player_->renderVideo();
}

void QMDKWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Space: {
        if (isPaused())
            play();
        else
            pause();
    }
        break;
    case Qt::Key_Right:
        seek(position() + 10000);
        break;
    case Qt::Key_Left:
        seek(position() - 10000);
        break;
    case Qt::Key_Q:
        qApp->quit();
        break;
    case Qt::Key_C:
        if (QKeySequence(e->modifiers() | e->key()) == QKeySequence::Copy) {
            snapshot();
        }
        break;
    default:
        break;
    }
}
