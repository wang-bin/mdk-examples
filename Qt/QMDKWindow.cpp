/*
 * Copyright (c) 2016-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include "QMDKWindow.h"
#include "mdk/Player.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QStringList>
#include <QScreen>
#include <QGuiApplication>
#if __has_include(<QX11Info>)
#include <QX11Info>
#endif

using namespace MDK_NS;
QMDKWindow::QMDKWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
    , player_(std::make_shared<Player>())
{
#ifdef QX11INFO_X11_H
    SetGlobalOption("X11Display", QX11Info::display());
    qDebug("X11 display: %p", QX11Info::display());
#elif (QT_FEATURE_xcb + 0 == 1) && (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    const auto xdisp = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->display();
    SetGlobalOption("X11Display", xdisp);
    qDebug("X11 display: %p", xdisp);
#endif
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
        "MMAL",
#endif
        "FFmpeg"});
    player_->setRenderCallback([this](void*){
        QCoreApplication::instance()->postEvent(this, new QEvent(QEvent::UpdateRequest), INT_MAX);
    });
}

QMDKWindow::~QMDKWindow()
{
    makeCurrent();
    player_->setVideoSurfaceSize(-1, -1); // cleanup gl renderer resources
}

void QMDKWindow::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    player_->setDecoders(MediaType::Video, v);
}

void QMDKWindow::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void QMDKWindow::play()
{
    player_->set(State::Playing);
}

void QMDKWindow::pause()
{
    player_->set(State::Paused);
}

void QMDKWindow::stop()
{
    player_->set(State::Stopped);
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
    player_->snapshot(&sr, [](Player::SnapshotRequest */*_sr*/, double frameTime) {
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

void QMDKWindow::resizeGL(int w, int h)
{
    auto s = screen();
    qDebug("resizeGL>>>>>dpr: %f, logical dpi: (%f,%f), phy dpi: (%f,%f)", s->devicePixelRatio(), s->logicalDotsPerInchX(), s->logicalDotsPerInchY(), s->physicalDotsPerInchX(), s->physicalDotsPerInchY());
    player_->setVideoSurfaceSize(w*devicePixelRatio(), h*devicePixelRatio());
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
