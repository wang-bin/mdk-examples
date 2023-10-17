/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureItem.h"
#include <QtQuick/QQuickWindow>
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
#include "VideoTextureNode.h" // TODO: remove
#include "mdk/Player.h"
using namespace std;

VideoTextureNode* createNode(VideoTextureItem* item);
VideoTextureNode* createNodePriv(VideoTextureItem* item);

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

VideoTextureItem::VideoTextureItem()
{
    static once_flag initFlag;
    call_once(initFlag, InitEnv);

    setFlag(ItemHasContents, true);
    m_player = make_shared<Player>();
    m_player->setDecoders(MediaType::Video, {"VT", "MFT:d3d=11", "VAAPI", "FFmpeg"});
    m_player->setRenderCallback([=](void *){
        QMetaObject::invokeMethod(this, "update");
    });
    // window() is null here
}

VideoTextureItem::~VideoTextureItem() = default;

// The beauty of using a true QSGNode: no need for complicated cleanup
// arrangements, unlike in other examples like metalunderqml, because the
// scenegraph will handle destroying the node at the appropriate time.
void VideoTextureItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    m_node = nullptr;
}

void VideoTextureItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    m_node = nullptr;
}

QSGNode *VideoTextureItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    auto n = static_cast<VideoTextureNode*>(node);

    if (!n && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!n) {
        m_node = createNodePriv(this);
        n = m_node;
        n->setAutoHDR(m_autoHDR);
    }

    m_node->sync();

    window()->update(); // ensure getting to beforeRendering() at some point

    return n;
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void VideoTextureItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void VideoTextureItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QQuickItem::geometryChange(newGeometry, oldGeometry);
#else
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
#endif

    if (newGeometry.size() != oldGeometry.size())
        update();
}

void VideoTextureItem::setSource(const QString & s)
{
    m_player->setMedia(s.toUtf8().data());
    m_source = s;
    emit sourceChanged();
    if (autoPlay())
        play();
}

void VideoTextureItem::setAutoPlay(bool value)
{
    if (m_autoPlay == value)
        return;
    m_autoPlay = value;
    emit autoPlayChanged();
}

void VideoTextureItem::setAutoHDR(bool value)
{
    if (m_autoHDR == value)
        return;
    m_autoHDR = value;
    emit autoHDRChanged();
}

void VideoTextureItem::play()
{
    m_player->set(PlaybackState::Playing);
}
