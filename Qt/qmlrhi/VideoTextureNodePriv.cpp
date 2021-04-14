/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureNode.h"
#include <QQuickWindow>
#include <private/qquickitem_p.h>
#include <private/qrhi_p.h>

#include "mdk/Player.h"
#include "mdk/RenderAPI.h"
using namespace std;

class VideoTextureNodePriv final: public VideoTextureNode
{
public:
    VideoTextureNodePriv(VideoTextureItem *item): VideoTextureNode(item) {}

    ~VideoTextureNodePriv() override {
        // release gfx resources
        releaseResources();
    }
private:
    QSGTexture* ensureTexture(Player* player, const QSize& size) override;

    void releaseResources();

    QRhiTexture* m_texture = nullptr;
    QRhiTextureRenderTarget* m_rt = nullptr;
    QRhiRenderPassDescriptor* m_rtRp = nullptr;
};

VideoTextureNode* createNodePriv(VideoTextureItem* item)
{
    return new VideoTextureNodePriv(item);
}


QSGTexture* VideoTextureNodePriv::ensureTexture(Player* player, const QSize& size)
{
    auto sgrc = QQuickItemPrivate::get(m_item)->sceneGraphRenderContext();
    auto rhi = sgrc->rhi();
    m_texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    //if (!m_texture->build()) { // qt5
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_texture->create()) {
#else
    if (!m_texture->build()) {
#endif
        // TODO: release
        return nullptr;
    }
    QRhiColorAttachment color0(m_texture);
    m_rt = rhi->newTextureRenderTarget({color0});
    if (!m_rt) {
        releaseResources();
        return nullptr;
    }
    m_rtRp = m_rt->newCompatibleRenderPassDescriptor();
    if (!m_rtRp) {
        releaseResources();
        return nullptr;
    }
    m_rt->setRenderPassDescriptor(m_rtRp);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_rt->create()) {
#else
    if (!m_rt->build()) {
#endif
        releaseResources();
        return nullptr;
    }
    QSGRendererInterface *rif = m_window->rendererInterface();
    switch (rif->graphicsApi()) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    case QSGRendererInterface::OpenGL: // same as OpenGLRhi in qt6
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    case QSGRendererInterface::OpenGLRhi:
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    {
        // TODO:
    }
        break;
    case QSGRendererInterface::MetalRhi: {
#if (__APPLE__+0)
        auto dev = rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(dev);

        MetalRenderAPI ra{};
        ra.texture = reinterpret_cast<const void*>(quintptr(m_texture->nativeTexture().object)); // 5.15+
        ra.device = dev;
        ra.cmdQueue = rif->getResource(m_window, QSGRendererInterface::CommandQueueResource);
        player->setRenderAPI(&ra);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(ra.texture);
# else
        auto nativeObj = ra.texture;
        if (nativeObj)
            return QNativeInterface::QSGMetalTexture::fromNative((__bridge id<MTLTexture>)nativeObj, m_window, size);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // (__APPLE__+0)
    }
        break;
    default:
        break;
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
# if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (nativeObj)
        return m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &nativeObj, nativeLayout, size);
# endif
#endif
    return nullptr;
}

void VideoTextureNodePriv::releaseResources()
{
    delete m_rt;
    m_rt = nullptr;

    if (m_rtRp) {
        delete m_rtRp;
        m_rtRp = nullptr;
    }

    delete m_texture;
    m_texture = nullptr;
}
