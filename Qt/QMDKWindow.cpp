/*
 * Copyright (c) 2016-2019 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include "QMDKWindow.h"
#include "mdk/Player.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QOffscreenSurface>
#include <QStringList>

using namespace MDK_NS;
QMDKWindow::QMDKWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
    , player_(std::make_shared<Player>())
{
    player_->setVideoDecoders({"VideoToolbox", "VAAPI", "D3D11", "DXVA2", "MMAL", "MediaCodec", "FFmpeg"});
    player_->setRenderCallback([this](void*){
        QCoreApplication::instance()->postEvent(this, new QEvent(QEvent::UpdateRequest), INT_MAX);
    });
}

QMDKWindow::~QMDKWindow() = default;

void QMDKWindow::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    player_->setVideoDecoders(v);
}

void QMDKWindow::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void QMDKWindow::play()
{
    player_->setState(State::Playing);
}

void QMDKWindow::pause()
{
    player_->setState(State::Paused);
}

void QMDKWindow::stop()
{
    player_->setState(State::Stopped);
}

bool QMDKWindow::isPaused() const
{
    return player_->state() == State::Paused;
}

void QMDKWindow::seek(qint64 ms)
{
    player_->seek(ms);
}

qint64 QMDKWindow::position() const
{
    return player_->position();
}

void QMDKWindow::snapshot() {
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

void QMDKWindow::initializeGL()
{
    auto player = player_;
    // instance is destroyed before aboutToBeDestroyed(), and no current context in aboutToBeDestroyed()
    auto ctx = context();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [player, ctx] {
        QOffscreenSurface s;
        s.create();
        ctx->makeCurrent(&s);
        Player::foreignGLContextDestroyed();
        ctx->doneCurrent();
    });
}

void QMDKWindow::resizeGL(int w, int h)
{
    player_->setVideoSurfaceSize(w, h);
}

void QMDKWindow::paintGL()
{
    player_->renderVideo();
}

void QMDKWindow::keyPressEvent(QKeyEvent *e)
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
