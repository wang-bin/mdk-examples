/*
 * Copyright (c) 2020-2024 WangBin <wbsecg1 at gmail.com>
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
    delete program_;
    delete fbo_;
}

void QMDKWidget::setEffect(Effect value)
{
    if (effect_ == value)
        return;
    effect_ = value;
    new_effect_ = true;
}

void QMDKWidget::setEffectIntensity(float value)
{
    intensity_ = value;
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
    player_->snapshot(&sr, [](const Player::SnapshotRequest * _sr, double frameTime) {
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
    initializeOpenGLFunctions();
    GLRenderAPI ra;
#if 0 // disable for linux. if requested api is not supported, getProcAddress() may return an invalid address I don't know how to check(use mask 0xffffffff00000000?) and result in crash
    ra.getProcAddress = +[](const char* name, void* opaque) {
        Q_UNUSED(opaque); // ((QOpenGLContext*)opaque)->getProcAddress(name));
        return (void*)QOpenGLContext::currentContext()->getProcAddress(name); // FIXME: invalid ptr if run xcb in wayland
    };
#endif
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
    fb_size_ = QSize(w, h);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto s = screen();
    fb_size_ *= devicePixelRatio();
    qDebug("resizeGL>>>>>dpr: %f, logical dpi: (%f,%f), phy dpi: (%f,%f)", s->devicePixelRatio(), s->logicalDotsPerInchX(), s->logicalDotsPerInchY(), s->physicalDotsPerInchX(), s->physicalDotsPerInchY());
    player_->setVideoSurfaceSize(w*devicePixelRatio(), h*devicePixelRatio()/*, this*/);
#else
    player_->setVideoSurfaceSize(w, h/*, this*/);
#endif
    if (fbo_) {
        delete fbo_;
        fbo_ = new QOpenGLFramebufferObject(fb_size_);
        program_->setUniformValue("res", fb_size_.toSizeF());
    }
}

void QMDKWidget::paintGL()
{
    ensureEffect();
    if (fbo_) {
        fbo_->bind();
    }
    player_->renderVideo(/*this*/);
    if (fbo_) {
        fbo_->release();
        program_->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo_->texture());

        program_->setUniformValue("res", fb_size_.toSizeF());
        program_->setUniformValue("intensity", intensity_);
        const GLfloat kVertices[] = {
            -1, 1,
            -1, -1,
            1, 1,
            1, -1,
        };
        const GLfloat kTexCoords[] = {
            0, 0,
            0, 1,
            1, 0,
            1, 1,
        };
        program_->setAttributeArray(0, GL_FLOAT, kVertices, 2);
        program_->setAttributeArray(1, GL_FLOAT, kTexCoords, 2);


        static const char a0[] = {0x61, 0x5f, 0x50, 0x6f, 0x73, 0x0};
        static const char a1[] = {0x61, 0x5f, 0x54, 0x65, 0x78, 0x0};
        static const char a2[] = {0x00, 0x51, 0x74, 0x41, 0x56, 0x0};
        static const char* attr[] = { a0, a1, a2};

        for (int i = 0; attr[i][0]; ++i) {
            program_->enableAttributeArray(i);
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        for (int i = 0; attr[i][0]; ++i) {
            program_->disableAttributeArray(i); //TODO: in setActiveShader
        }
    }
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
    case Qt::Key_G:
        setEffect(Effect::GaussianBlur);
        break;
    case Qt::Key_B:
        setEffect(Effect::BoxBlur);
        break;
    case Qt::Key_M:
        setEffect(Effect::Mosaic);
        break;
    case Qt::Key_N:
        setEffect(Effect::None);
        break;
    case Qt::Key_1:
        setEffectIntensity(1.0f);
        break;
    case Qt::Key_2:
        setEffectIntensity(2.0f);
        break;
    case Qt::Key_3:
        setEffectIntensity(3.0f);
        break;
    case Qt::Key_4:
        setEffectIntensity(4.0f);
        break;
    case Qt::Key_5:
        setEffectIntensity(5.0f);
        break;
    case Qt::Key_6:
        setEffectIntensity(6.0f);
        break;
    case Qt::Key_7:
        setEffectIntensity(7.0f);
        break;
    case Qt::Key_8:
        setEffectIntensity(8.0f);
        break;
    case Qt::Key_9:
        setEffectIntensity(9.0f);
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

bool QMDKWidget::ensureEffect()
{
    if (!new_effect_.exchange(false))
        return false;

    delete program_;
    program_ = nullptr;

    if (effect_ == Effect::None) {
        delete fbo_;
        fbo_ = nullptr;
        return true;
    }
    if (!fbo_) {
        fbo_ = new QOpenGLFramebufferObject(fb_size_);
    }

    program_ = new QOpenGLShaderProgram();
    if (!program_->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
attribute vec2 a_Position;
attribute vec2 a_TexCoords0;
varying vec2 v_TexCoords0;
void main() {
    gl_Position = vec4(a_Position.x, -a_Position.y, 1, 1);
    v_TexCoords0 = a_TexCoords0;
}
)"
                                           )) {
        return false;
    }
    if (effect_ == Effect::BoxBlur) {
        if (!program_->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, R"(
#ifdef GL_ES
precision mediump int;
precision highp float;
#endif
uniform sampler2D source;
uniform vec2 res;
uniform float intensity; // 0~10

varying vec2 v_TexCoords0;

void main()
{
    vec2 uv = v_TexCoords0.xy;
    vec4 c = vec4(0.0);
    int kernel = 2*(int(floor(intensity/2.0 - 0.5)) + 1)-1;
    int min_val = - (kernel - 1) / 2;
    int max_val = min_val + kernel;
    vec2 step = 1.0/res.xy;

    for (int x = min_val; x < max_val; x++)
    {
        for (int y = min_val; y < max_val; y++)
        {
            vec2 offset = vec2(x, y) * step;
            c += texture2D(source, uv + offset);
        }
    }

    c /= pow(float(kernel), 2.0);
    gl_FragColor = c;
}
)"
                                                        )) {
            return false;
        }
    } else if (effect_ == Effect::GaussianBlur) {
        if (!program_->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, R"(
#ifdef GL_ES
precision mediump int;
precision highp float;
#endif
uniform sampler2D source;
uniform vec2 res;
uniform float intensity; // 0~10
varying vec2 v_TexCoords0;

void main()
{
    vec2 uv = v_TexCoords0.xy;
    vec4 c = vec4(0.0);
    int r = 3;
    int min_val = - (r - 1) / 2;
    int max_val = min_val + r;
    float kernel[9];
    kernel[6] = 1.0; kernel[7] = 2.0; kernel[8] = 1.0;
    kernel[3] = 2.0; kernel[4] = 4.0; kernel[5] = 2.0;
    kernel[0] = 1.0; kernel[1] = 2.0; kernel[2] = 1.0;
    vec2 step = intensity/res.xy;
    for (int x = min_val; x < max_val; ++x) {
        for (int y = min_val; y < max_val; ++y) {
            vec2 offset = vec2(x, y) * step;
            c += texture2D(source, uv + offset) * kernel[(x - min_val) + r * (y - min_val)];
        }
    }

    c /= 16.0;
    gl_FragColor = c;
}
)"
                                                        )) {
            return false;
        }
    } else if (effect_ == Effect::Mosaic) {
        if (!program_->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, R"(
#ifdef GL_ES
precision mediump int;
precision highp float;
#endif
uniform sampler2D source;
uniform vec2 res;
uniform float intensity; // 0~10
varying vec2 v_TexCoords0;

void main()
{
    vec2 uv = v_TexCoords0.xy;
    vec2 xy = uv * res;
    vec2 mXY = vec2(floor(xy.x / intensity) * intensity + 0.5 * intensity, floor(xy.y / intensity) * intensity + 0.5 * intensity);
    vec2 l1 = abs(mXY - xy);
    float d = max(l1[0], l1[1]);
    //float d = length(mXY -xy);
    vec2 mUV = mXY / res;

    vec4 c;
    if (d < 0.5 * intensity) {
        c = texture2D(source, mUV);
    } else {
        c = texture2D(source, v_TexCoords0);
    }
    gl_FragColor = c;
}
)"
                                                        )) {
            return false;
        }
    }

    if (!program_->link()) {
        qDebug() << "shader link error: " << program_->log();
    }
    return true;
}
