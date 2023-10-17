/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureNode.h"
#include <QQuickWindow>
#include <private/qquickitem_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
# include <rhi/qrhi.h>
# include <private/qrhigles2_p.h>
#else
# include <private/qrhi_p.h>
# include <private/qrhigles2_p_p.h>
#endif
#if (_WIN32+0)
#include <d3d11.h>
#include <d3d12.h>
#endif
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
    auto format = QRhiTexture::RGBA8;
    QSGRendererInterface *rif = m_window->rendererInterface();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
    auto sc = (QRhiSwapChain*)rif->getResource(m_window, QSGRendererInterface::RhiSwapchainResource);
    if (sc) {
        if (sc->format() == QRhiSwapChain::Format::HDR10) {
            player->set(ColorSpaceBT2100_PQ, this);
            //format = QRhiTexture::RGB10A2; // rgba8 works fine on windows
        } else if (sc->format() == QRhiSwapChain::Format::HDRExtendedSrgbLinear) {
            format = QRhiTexture::RGBA16F;
            player->set(ColorSpaceSCRGB, this);
            csResetHack("scrgb");
# if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)) // p3
        } else if (sc->format() == QRhiSwapChain::Format::HDRExtendedDisplayP3Linear) {
            format = QRhiTexture::RGBA16F;
            player->set(ColorSpaceExtendedLinearDisplayP3, this);
            csResetHack("p3");
# endif
        } else {
            player->set(ColorSpaceBT709, this);
        }
    }
#endif
    m_texture = rhi->newTexture(format, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    //if (!m_texture->build()) { // qt5
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_texture->create()) {
#else
    if (!m_texture->build()) {
#endif
        releaseResources();
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    intmax_t nativeObj = 0;
    int nativeLayout = 0;
#endif
    switch (rif->graphicsApi()) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    case QSGRendererInterface::OpenGL: // same as OpenGLRhi in qt6
#endif
    case QSGRendererInterface::OpenGLRhi:
    {
#if QT_CONFIG(opengl)
        m_tx = TextureCoordinatesTransformFlag::MirrorVertically;
        auto glrt = static_cast<QGles2TextureRenderTarget*>(m_rt);
        GLRenderAPI ra;
        ra.fbo = glrt->framebuffer;
        player->setRenderAPI(&ra, this);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        auto tex = GLuint((quintptr)m_texture->nativeTexture().object);
        nativeObj = decltype(nativeObj)(tex);
#   if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
        return m_window->createTextureFromId(tex, size);
#   endif // QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
# elif (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
        auto tex = GLuint(m_texture->nativeTexture().object);
        if (tex)
            return QNativeInterface::QSGOpenGLTexture::fromNative(tex, m_window, size, QQuickWindow::TextureHasAlphaChannel);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // if QT_CONFIG(opengl)
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
# if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0))
        auto sc = (QRhiSwapChain*)rif->getResource(m_window, QSGRendererInterface::RhiSwapchainResource);
        ra.layer = sc->proxyData().reserved[0];
# endif
        player->setRenderAPI(&ra, this);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(ra.texture);
# elif (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
        if (ra.texture)
            return QNativeInterface::QSGMetalTexture::fromNative((__bridge id<MTLTexture>)ra.texture, m_window, size, QQuickWindow::TextureHasAlphaChannel);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // (__APPLE__+0)
    }
        break;
#if (_WIN32+0)
    case QSGRendererInterface::Direct3D11Rhi: {
        D3D11RenderAPI ra;
        ra.rtv = reinterpret_cast<ID3D11DeviceChild*>(quintptr(m_texture->nativeTexture().object));
        player->setRenderAPI(&ra, this);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(ra.rtv);
# elif (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
        if (ra.rtv)
            return QNativeInterface::QSGD3D11Texture::fromNative(ra.rtv, m_window, size, QQuickWindow::TextureHasAlphaChannel);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    }
        break;
# if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QSGRendererInterface::Direct3D12: {
        D3D12RenderAPI ra;
        ra.cmdQueue = reinterpret_cast<ID3D12CommandQueue*>(rif->getResource(m_window, QSGRendererInterface::CommandQueueResource));
        ra.rt = reinterpret_cast<ID3D12Resource*>(quintptr(m_texture->nativeTexture().object));
        player->setRenderAPI(&ra, this);
    }
        break;
# endif
#endif // (_WIN32)
    case QSGRendererInterface::VulkanRhi: {
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
        VulkanRenderAPI ra{};
        ra.device =*static_cast<VkDevice *>(rif->getResource(m_window, QSGRendererInterface::DeviceResource));
        ra.phy_device = *static_cast<VkPhysicalDevice *>(rif->getResource(m_window, QSGRendererInterface::PhysicalDeviceResource));
        ra.opaque = this;
        ra.rt = VkImage(m_texture->nativeTexture().object);
        ra.renderTargetInfo = [](void* opaque, int* w, int* h, VkFormat* fmt, VkImageLayout* layout) {
            auto node = static_cast<VideoTextureNodePriv*>(opaque);
            *w = node->m_size.width();
            *h = node->m_size.height();
            *fmt = VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        ra.currentCommandBuffer = [](void* opaque){
            auto node = static_cast<VideoTextureNodePriv*>(opaque);
            QSGRendererInterface *rif = node->m_window->rendererInterface();
            auto cmdBuf = *static_cast<VkCommandBuffer *>(rif->getResource(node->m_window, QSGRendererInterface::CommandListResource));
            return cmdBuf;
        };
        player->setRenderAPI(&ra, this);
# if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        if (ra.rt)
            return QNativeInterface::QSGVulkanTexture::fromNative(ra.rt, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_window, size, QQuickWindow::TextureHasAlphaChannel);
# elif (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
        nativeLayout = m_texture->nativeTexture().layout;
        nativeObj = decltype(nativeObj)(ra.rt);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#else
        qFatal("Rebuild with Vulkan!");
#endif // (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
    }
        break;
    default:
        break;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0))
    // the only way to create sg texture with a correct format
    return m_window->createTextureFromRhiTexture(m_texture, QQuickWindow::TextureHasAlphaChannel);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
# if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (nativeObj)
        return m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &nativeObj, nativeLayout, size, QQuickWindow::TextureHasAlphaChannel);
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
