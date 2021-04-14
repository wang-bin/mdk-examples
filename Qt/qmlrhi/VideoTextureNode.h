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
    Q_OBJECT
public:
    VideoTextureNode(VideoTextureItem *item);
    ~VideoTextureNode();

    QSGTexture *texture() const override;

    void sync();
private slots:
    void render();

private:
    virtual QSGTexture* ensureTexture(Player* player, const QSize& size) = 0;

    QSize m_size;
    qreal m_dpr;

    std::weak_ptr<Player> m_player;

protected:
    QQuickWindow *m_window;
    QQuickItem *m_item;
};
