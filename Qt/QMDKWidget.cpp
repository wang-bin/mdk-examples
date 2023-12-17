/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWidget example
 */
#include "QMDKWidget.h"
#include "mdk/Player.h"
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QStringList>
#include <QScreen>
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

using namespace MDK_NS;

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

QMDKWidget::QMDKWidget(QWidget *parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
    , player_(std::make_shared<Player>())
{
    static std::once_flag initFlag;
    std::call_once(initFlag, InitEnv);

    player_->setDecoders(MediaType::Video, {
#if (__APPLE__+0)
        "VT",
        "hap",
#elif (__ANDROID__+0)
        "AMediaCodec:java=0:copy=0:surface=1:async=0",
#elif (_WIN32+0)
        "MFT:d3d=11",
        "CUDA",
        "hap", // before any ffmpeg based decoders
        "D3D11",
        "DXVA",
#elif (__linux__+0)
        "hap",
        "VAAPI",
        "VDPAU",
        "CUDA",
#endif
        "FFmpeg"});
    player_->setRenderCallback([this](void*){
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    });
}

QMDKWidget::~QMDKWidget()
{
    makeCurrent();
    player_->setVideoSurfaceSize(-1, -1); // cleanup gl renderer resources
}

void QMDKWidget::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    for (const auto& d : dec) {
        v.push_back(d.toStdString());
    }
    player_->setDecoders(MediaType::Video, v);
}

void QMDKWidget::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void QMDKWidget::play()
{
    player_->set(State::Playing);
}

void QMDKWidget::pause()
{
    player_->set(State::Paused);
}

void QMDKWidget::stop()
{
    player_->set(State::Stopped);
}

bool QMDKWidget::isPaused() const
{
    return player_->state() == State::Paused;
}

void QMDKWidget::seek(qint64 ms, bool accurate)
{
    auto flags = SeekFlag::FromStart;
    if (!accurate)
        flags |= SeekFlag::KeyFrame;
    player_->seek(ms, flags);
}

qint64 QMDKWidget::position() const
{
    return player_->position();
}

void QMDKWidget::snapshot() {
    Player::SnapshotRequest sr{};
    player_->snapshot(&sr, [](Player::SnapshotRequest * _sr, double frameTime) {
        const QString path = QDir::toNativeSeparators(
            QString("%1/%2.jpg")
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
        return std::string();*/
    });
}

qint64 QMDKWidget::duration() const
{
    return player_->mediaInfo().duration;
}

void QMDKWidget::prepreForPreview()
{
    player_->setActiveTracks(MediaType::Audio, {});
    player_->setActiveTracks(MediaType::Subtitle, {});
    player_->setProperty("continue_at_end", "1"); // not required by the latest sdk
    player_->setBufferRange(0);
    player_->prepare();
}

void QMDKWidget::initializeGL()
{
    GLRenderAPI ra;
    ra.getProcAddress = +[](const char* name, void* opaque) {
        Q_UNUSED(opaque); // ((QOpenGLContext*)opaque)->getProcAddress(name));
        return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
    };
    ra.opaque = context();
    player_->setRenderAPI(&ra/*, this*/);
    // context() may change(destroy old and create new) via setParent()
    std::weak_ptr<mdk::Player> wp = player_;
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, [=]{
        makeCurrent();
        auto sp = wp.lock();
        if (sp) // release and remove old gl resources with the same vo_opaque(nullptr), then new resource will be created in resizeGL/paintGL
            sp->setVideoSurfaceSize(-1, -1/*, context()*/); // it's better to cleanup gl renderer resources as early as possible
        else
            Player::foreignGLContextDestroyed();
        doneCurrent();
    });
}

void QMDKWidget::resizeGL(int w, int h)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto s = screen();
    qDebug("resizeGL>>>>>dpr: %f, logical dpi: (%f,%f), phy dpi: (%f,%f)", s->devicePixelRatio(), s->logicalDotsPerInchX(), s->logicalDotsPerInchY(), s->physicalDotsPerInchX(), s->physicalDotsPerInchY());
    player_->setVideoSurfaceSize(w*devicePixelRatio(), h*devicePixelRatio()/*, this*/);
#else
    player_->setVideoSurfaceSize(w, h/*, this*/);
#endif
}

void QMDKWidget::paintGL()
{
    player_->renderVideo(/*this*/);
}

void QMDKWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Space: {
        if (player_->state() != State::Playing)
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
    case Qt::Key_F: {
        if (isFullScreen())
            showNormal();
        else
            showFullScreen();
    }
        break;
    default:
        break;
    }
}

void QMDKWidget::mouseMoveEvent(QMouseEvent *ev)
{
    Q_EMIT mouseMoved(ev->pos().x(), ev->pos().y());
    ev->accept();
}

void QMDKWidget::mouseDoubleClickEvent(QMouseEvent *ev)
{
    Q_EMIT doubleClicked();
    ev->accept();
}
