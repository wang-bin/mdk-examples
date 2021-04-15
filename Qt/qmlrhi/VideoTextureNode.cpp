/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
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
    delete texture();
    // when device lost occurs
    auto player = m_player.lock();
    if (!player)
        return;
    player->setVideoSurfaceSize(-1, -1);
    qDebug("renderer destroyed");
}

QSGTexture *VideoTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void VideoTextureNode::sync()
{
    const auto dpr = m_window->effectiveDevicePixelRatio();
    const QSizeF newSize = m_item->size() * dpr; // QQuickItem.size(): since 5.10
    if (texture() && newSize == m_size)
        return;
    m_size = {qRound(newSize.width()), qRound(newSize.height())};
    delete texture();

    auto player = m_player.lock();
    if (!player)
        return;
    auto tex = ensureTexture(player.get(), m_size);
    if (tex)
        setTexture(tex);
    player->setVideoSurfaceSize(m_size.width(), m_size.height());
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
    player->renderVideo();
}
