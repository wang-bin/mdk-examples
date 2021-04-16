/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureNode.h"
#include <QQuickWindow>
#if (__APPLE__+0)
#include <Metal/Metal.h>
#endif
#if (_WIN32+0)
#include <d3d11.h>
#include <wrl/client.h>
using namespace Microsoft::WRL; //ComPtr
#endif
#if QT_CONFIG(opengl)
#include <QOpenGLFramebufferObject>
#endif
#if __has_include(<vulkan/vulkan_core.h>)
#include <QVulkanInstance>
#include <QVulkanFunctions>
#endif
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"
using namespace std;

#define VK_ENSURE(x, ...) VK_RUN_CHECK(x, return __VA_ARGS__)
#define VK_WARN(x, ...) VK_RUN_CHECK(x)
#define VK_RUN_CHECK(x, ...) do { \
        VkResult __vkret__ = x; \
        if (__vkret__ != VK_SUCCESS) { \
          qDebug() << #x " ERROR: " << __vkret__ << " @" << __LINE__ << __func__; \
          __VA_ARGS__; \
        } \
    } while(false)

class VideoTextureNodePub final: public VideoTextureNode
{
public:
    VideoTextureNodePub(VideoTextureItem *item): VideoTextureNode(item) {}

    ~VideoTextureNodePub() override {
        // release gfx resources
#if QT_CONFIG(opengl)
        fbo_gl.reset();
#endif
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
        freeTexture();
#endif
    }

private:
    QSGTexture* ensureTexture(Player* player, const QSize& size) override;
#if (__APPLE__+0)
    id<MTLTexture> m_texture_mtl = nil;
#endif
#if QT_CONFIG(opengl)
    unique_ptr<QOpenGLFramebufferObject> fbo_gl;
#endif
#if (_WIN32+0)
    ComPtr<ID3D11Texture2D> m_texture_d3d11;
#endif
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
    bool buildTexture(const QSize &size);
    void freeTexture();

    VkImage m_texture_vk = VK_NULL_HANDLE;
    VkDeviceMemory m_textureMemory = VK_NULL_HANDLE;

    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
#endif
};

VideoTextureNode* createNode(VideoTextureItem* item)
{
    return new VideoTextureNodePub(item);
}

QSGTexture* VideoTextureNodePub::ensureTexture(Player* player, const QSize& size)
{
    QSGRendererInterface *rif = m_window->rendererInterface();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    intmax_t nativeObj = 0;
    int nativeLayout = 0;
#endif
    switch (rif->graphicsApi()) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    case QSGRendererInterface::OpenGL: // same as OpenGLRhi in qt6
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    case QSGRendererInterface::OpenGLRhi:
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    {
#if QT_CONFIG(opengl)
        m_tx = TextureCoordinatesTransformFlag::MirrorVertically;
        fbo_gl.reset(new QOpenGLFramebufferObject(size));
        GLRenderAPI ra;
        ra.fbo = fbo_gl->handle();
        player->setRenderAPI(&ra);
        auto tex = fbo_gl->texture();
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(tex);
#   if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
        return m_window->createTextureFromId(tex, size);
#   endif // QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
# else
        if (tex)
            return QNativeInterface::QSGOpenGLTexture::fromNative(tex, m_window, size);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // if QT_CONFIG(opengl)
    }
        break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    case QSGRendererInterface::Direct3D11Rhi: {
#if (_WIN32)
        auto dev = (ID3D11Device*)rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        D3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, size.width(), size.height(), 1, 1
                                                          , D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET
                                                          , D3D11_USAGE_DEFAULT, 0, 1, 0, 0);
        if (FAILED(dev->CreateTexture2D(&desc, nullptr, &m_texture_d3d11))) {

        }
        D3D11RenderAPI ra;
        ra.rtv = m_texture_d3d11.Get();
        player->setRenderAPI(&ra);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(m_texture_d3d11.Get());
# else
        if (m_texture_d3d11)
            return QNativeInterface::QSGD3D11Texture::fromNative(m_texture_d3d11.Get(), m_window, size);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // (_WIN32)
    }
        break;
    case QSGRendererInterface::MetalRhi: {
#if (__APPLE__+0)
        auto dev = (__bridge id<MTLDevice>)rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(dev);

        MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureType2D;
        desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        desc.width = size.width();
        desc.height = size.height();
        desc.mipmapLevelCount = 1;
        desc.resourceOptions = MTLResourceStorageModePrivate;
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        m_texture_mtl = [dev newTextureWithDescriptor: desc];
        MetalRenderAPI ra{};
        ra.texture = (__bridge void*)m_texture_mtl;
        ra.device = (__bridge void*)dev;
        ra.cmdQueue = rif->getResource(m_window, QSGRendererInterface::CommandQueueResource);
        player->setRenderAPI(&ra);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(ra.texture);
# else
        if (m_texture_mtl)
            return QNativeInterface::QSGMetalTexture::fromNative(m_texture_mtl, m_window, size);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // (__APPLE__+0)
    }
        break;
    case QSGRendererInterface::VulkanRhi: {
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        auto inst = reinterpret_cast<QVulkanInstance *>(rif->getResource(m_window, QSGRendererInterface::VulkanInstanceResource));
        m_physDev = *static_cast<VkPhysicalDevice *>(rif->getResource(m_window, QSGRendererInterface::PhysicalDeviceResource));
        auto newDev = *static_cast<VkDevice *>(rif->getResource(m_window, QSGRendererInterface::DeviceResource));
        qDebug("old device: %p, newDev: %p", (void*)m_dev, (void*)newDev);
        // TODO: why m_dev is 0 if device lost
        freeTexture();
        m_dev = newDev;
        m_devFuncs = inst->deviceFunctions(m_dev);

        buildTexture(size);
# if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        nativeObj = decltype(nativeObj)(m_texture_vk);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))

        VulkanRenderAPI ra{};
        ra.device =m_dev;
        ra.phy_device = m_physDev;
        ra.opaque = this;
        ra.rt = m_texture_vk;
        ra.renderTargetInfo = [](void* opaque, int* w, int* h, VkFormat* fmt, VkImageLayout* layout) {
            auto node = static_cast<VideoTextureNodePub*>(opaque);
            *w = node->m_size.width();
            *h = node->m_size.height();
            *fmt = VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        ra.currentCommandBuffer = [](void* opaque){
            auto node = static_cast<VideoTextureNodePub*>(opaque);
            QSGRendererInterface *rif = node->m_window->rendererInterface();
            auto cmdBuf = *static_cast<VkCommandBuffer *>(rif->getResource(node->m_window, QSGRendererInterface::CommandListResource));
            return cmdBuf;
        };
        player->setRenderAPI(&ra);
# if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        if (m_texture_vk)
            return QNativeInterface::QSGVulkanTexture::fromNative(m_texture_vk, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_window, size);
# endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#endif // (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
    }
        break;
#endif //QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
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

#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
bool VideoTextureNodePub::buildTexture(const QSize &size)
{
    VkImageCreateInfo imageInfo;
    memset(&imageInfo, 0, sizeof(imageInfo));
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // qtquick hardcoded
    imageInfo.extent.width = uint32_t(size.width());
    imageInfo.extent.height = uint32_t(size.height());
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImage image = VK_NULL_HANDLE;
    VK_ENSURE(m_devFuncs->vkCreateImage(m_dev, &imageInfo, nullptr, &image), false);

    m_texture_vk = image;

    VkMemoryRequirements memReq;
    m_devFuncs->vkGetImageMemoryRequirements(m_dev, image, &memReq);

    quint32 memIndex = 0;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    m_window->vulkanInstance()->functions()->vkGetPhysicalDeviceMemoryProperties(m_physDev, &physDevMemProps);
    for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        if (!(memReq.memoryTypeBits & (1 << i)))
            continue;
        memIndex = i;
    }

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        memIndex
    };

    VK_ENSURE(m_devFuncs->vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_textureMemory), false);
    VK_ENSURE(m_devFuncs->vkBindImageMemory(m_dev, image, m_textureMemory, 0), false);
    return true;
}

void VideoTextureNodePub::freeTexture()
{
    if (!m_texture_vk)
        return;
    VK_WARN(m_devFuncs->vkDeviceWaitIdle(m_dev));
    m_devFuncs->vkFreeMemory(m_dev, m_textureMemory, nullptr);
    m_textureMemory = VK_NULL_HANDLE;
    m_devFuncs->vkDestroyImage(m_dev, m_texture_vk, nullptr);
    m_texture_vk = VK_NULL_HANDLE;
}
#endif //(VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
