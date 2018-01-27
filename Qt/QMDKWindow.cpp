/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include "QMDKWindow.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QStringList>
#include <QtDebug>
#include "mdk/Player.h"

using namespace MDK_NS;
QMDKWindow::QMDKWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
    , player_(new Player())
{
    setLogHandler([](LogLevel level, const char* msg){
        if (level >= std::underlying_type<LogLevel>::type(LogLevel::Info)) {
            qDebug() << msg;
        } else if (level >= std::underlying_type<LogLevel>::type(LogLevel::Warning)) {
            qWarning() << msg;
        }
    });
    player_->setVideoDecoders({"VideoToolbox", "VAAPI", "D3D11", "DXVA2", "MMAL", "MediaCodec", "FFmpeg"});
    player_->setRenderCallback([this]{
        QCoreApplication::instance()->postEvent(this, new QEvent(QEvent::UpdateRequest));
    });
}

QMDKWindow::~QMDKWindow()
{
    delete player_;
}

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

void QMDKWindow::initializeGL()
{
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
    default:
        break;
    }
}
