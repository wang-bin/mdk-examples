/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
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

    void setAutoHDR(bool value) { m_autoHDR = value; }
    bool autoHDR() const { return m_autoHDR;}
private:
    void render();
    virtual QSGTexture* ensureTexture(Player* player, const QSize& size) = 0;

protected:
    TextureCoordinatesTransformMode m_tx = TextureCoordinatesTransformFlag::NoTransform;
    QQuickWindow *m_window;
    QQuickItem *m_item;
    QSize m_size;
    void csResetHack(const char* csName);
private:
    void csReset();

    std::weak_ptr<Player> m_player;
    bool m_autoHDR = false;
};
