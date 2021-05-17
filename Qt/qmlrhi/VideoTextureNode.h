/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#pragma once

#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>

#include "mdk/global.h"
namespace MDK_NS {
class Player;
}
using namespace MDK_NS;
class QQuickItem;
class QQuickWindow;
class VideoTextureItem;
class VideoTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
public:
    VideoTextureNode(VideoTextureItem *item);
    ~VideoTextureNode();

    QSGTexture *texture() const override;

    void sync();
private:
    void render();
    virtual QSGTexture* ensureTexture(Player* player, const QSize& size) = 0;

protected:
    TextureCoordinatesTransformMode m_tx = TextureCoordinatesTransformFlag::NoTransform;
    QQuickWindow *m_window;
    QQuickItem *m_item;
    QSize m_size;
private:
    std::weak_ptr<Player> m_player;
};
