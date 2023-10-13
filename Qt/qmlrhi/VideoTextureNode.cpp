/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureNode.h"
#include "VideoTextureItem.h"
#include <QQuickWindow>
#include <QtGui/QScreen>
#include "mdk/Player.h"

VideoTextureNode::VideoTextureNode(VideoTextureItem *item)
    : m_item(item)
    , m_player(item->m_player)
{
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &VideoTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        m_item->update();
    });
}

VideoTextureNode::~VideoTextureNode()
{
    // when device lost occurs
    auto player = m_player.lock();
    if (!player)
        return;
    player->setVideoSurfaceSize(-1, -1, this);
    qDebug("renderer destroyed");
    delete texture();
}

QSGTexture *VideoTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void VideoTextureNode::sync()
{
    const auto dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = QSizeF(m_item->size() * dpr).toSize(); // QQuickItem.size(): since 5.10
    if (texture() && newSize == m_size)
        return;

    auto player = m_player.lock();
    if (!player)
        return;
    m_size = newSize;
    auto tex = ensureTexture(player.get(), m_size);
    if (!tex)
        return;
    delete texture();
    setTexture(tex);
    setTextureCoordinatesTransform(m_tx); // MUST set when texture() is available
    setFiltering(QSGTexture::Linear);
    setRect(0, 0, m_item->width(), m_item->height());
    // if qsg render loop is threaded, a new render thread will be created when item's window changes, so mdk vo_opaque parameter must be bound to item window
    player->setVideoSurfaceSize(m_size.width(), m_size.height(), this);
}

// This is hooked up to beforeRendering() so we can start our own render
// command encoder. If we instead wanted to use the scenegraph's render command
// encoder (targeting the window), it should be connected to
// beforeRenderPassRecording() instead.
//! [6]
void VideoTextureNode::render()
{
    auto player = m_player.lock();
    if (!player)
        return;
    //m_window->beginExternalCommands();
    player->renderVideo(this);
    //m_window->endExternalCommands();
}
