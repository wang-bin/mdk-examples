/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureItem.h"
#include <QtQuick/QQuickWindow>
#include "VideoTextureNode.h" // TODO: remove
#include "mdk/Player.h"
using namespace std;


VideoTextureNode* createNode(VideoTextureItem* item);
VideoTextureNode* createNodePriv(VideoTextureItem* item);

VideoTextureItem::VideoTextureItem()
{
    setFlag(ItemHasContents, true);
    m_player = make_shared<Player>();
    //m_player->setDecoders(MediaType::Video, {"VT", "MFT:d3d=11", "VAAPI", "FFmpeg"});
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
        m_node = createNode(this);
        n = m_node;
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

void VideoTextureItem::play()
{
    m_player->set(PlaybackState::Playing);
}
