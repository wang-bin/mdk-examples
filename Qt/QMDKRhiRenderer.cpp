/*
 * Copyright (c) 2025 WangBin <wbsecg1 at gmail.com>
 */
#include "QMDKRenderer.h"
#include "QMDKPlayer.h"
#include <vector>
#include <private/qrhigles2_p.h>
#ifdef Q_OS_WIN
#include <d3d11.h>
#include <d3d12.h>
#endif
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"

#include <QCoreApplication>
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

using namespace mdk;

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

static std::once_flag initFlag;


QMDKRhiWidgetRenderer::QMDKRhiWidgetRenderer(QWidget *parent)
    : QRhiWidget(parent)
{
    std::call_once(initFlag, InitEnv);

    setAutoRenderTarget(true);
    //setFixedColorBufferSize({1920, 1080});
}

QMDKRhiWidgetRenderer::~QMDKRhiWidgetRenderer() = default;


void QMDKRhiWidgetRenderer::setSource(QMDKPlayer* player)
{
    setProperty("devicePixelRatio", devicePixelRatio());
    player->addRenderer(this); // use QWidget* instead of QOpenGLContext* as vo_opaque must ensure context() will not change, e.g. no reparenting the widget via setParent()
    m_player = player;
    if (player) {
        // should skip rendering if player object is destroyed
        connect(player, &QObject::destroyed, [this](QObject* obj){
            auto p = reinterpret_cast<QMDKPlayer*>(obj); // why qobject_cast is null? destroying and sub class dtor is finished?
            if (m_player == p)
                m_player = nullptr;
        });
        connect(player, &QMDKPlayer::videoSizeChanged, this, &QMDKRhiWidgetRenderer::videoSizeChanged, Qt::DirectConnection);
    }
}

void QMDKRhiWidgetRenderer::setROI(float x, float y, float w, float h)
{
    float roi[] = { x, y, x + w, y + h};
    qDebug("roi: (%f, %f) (%f, %f)", x, y, w, h);
    m_player->setROI(this, roi);
}


void QMDKRhiWidgetRenderer::initialize(QRhiCommandBuffer *cb)
{
    const QRhiNativeHandles *nat = rhi()->nativeHandles();
    if (!nat)
        return;

    m_cb = cb;
    auto tex = colorTexture(); // assume no msaa
    switch (api()) {
#if QT_CONFIG(opengl)
    case Api::OpenGL: {
        auto glrt = static_cast<QGles2TextureRenderTarget*>(renderTarget());
        GLRenderAPI ra{};
        ra.fbo = glrt->framebuffer;
        qDebug("rt: %p, fbo: %u", renderTarget(), ra.fbo);
        //m_player->setRenderAPI(&ra, this); // FIXME: fbo bind error in mdk after resize if no beginPass+beginExternal
    }
    break;
#endif
#if (__APPLE__ + 0)
    case Api::Metal: {
        const auto *mtlnat = static_cast<const QRhiMetalNativeHandles*>(nat);
        MetalRenderAPI ra{};
        ra.texture = reinterpret_cast<const void*>(quintptr(tex->nativeTexture().object)); // 5.15+
        ra.device = mtlnat->dev;
        ra.cmdQueue = mtlnat->cmdQueue;
        ra.currentCommand = [](const void** enc, const void** cmdBuf, const void* opaque) {
            auto widget = static_cast<const ThisClass*>(opaque);
            if (!widget->m_cb)
                return;
            auto mtlCbNat = static_cast<const QRhiMetalCommandBufferNativeHandles*>(widget->m_cb->nativeHandles());
            *enc = mtlCbNat->encoder;
            *cmdBuf = mtlCbNat->commandBuffer;
        };
        ra.opaque = this;
        m_player->setRenderAPI(&ra, this);
    }
    break;
#endif
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
    case Api::Vulkan: {
        auto vknat = static_cast<const QRhiVulkanNativeHandles*>(nat);
        VulkanRenderAPI ra{};
        ra.device = vknat->dev;
        ra.phy_device = vknat->physDev;
        ra.opaque = this;
        ra.rt = (VkImage)tex->nativeTexture().object;
        ra.renderTargetInfo = [](void* opaque, int* w, int* h, VkFormat* fmt, VkImageLayout* layout) {
            auto widget = static_cast<ThisClass*>(opaque);
            const auto tf = widget->colorBufferFormat();
            const auto sz = widget->renderTarget()->pixelSize();
            *w = sz.width();
            *h = sz.height();
            *fmt = tf == QRhiWidget::TextureFormat::RGBA16F ? VK_FORMAT_R16G16B16A16_SFLOAT : tf == QRhiWidget::TextureFormat::RGB10A2 ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        ra.currentCommandBuffer = [](void* opaque) -> VkCommandBuffer{
            auto widget = static_cast<ThisClass*>(opaque);
            if (!widget->m_cb)
                return VK_NULL_HANDLE;
            auto vkCbNat = static_cast<const QRhiVulkanCommandBufferNativeHandles*>(widget->m_cb->nativeHandles());
            return vkCbNat->commandBuffer;
        };
        m_player->setRenderAPI(&ra, this);
    }
    break;
#endif
#ifdef Q_OS_WIN
    case Api::Direct3D11: {
        D3D11RenderAPI ra{};
        ra.rtv = reinterpret_cast<ID3D11DeviceChild*>(quintptr(tex->nativeTexture().object));
        m_player->setRenderAPI(&ra, this);
    }
    break;
    case Api::Direct3D12: {
        auto d3dnat = static_cast<const QRhiD3D12NativeHandles*>(nat);
        D3D12RenderAPI ra{};
        ra.cmdQueue = reinterpret_cast<ID3D12CommandQueue*>(d3dnat->commandQueue);
        ra.rt = reinterpret_cast<ID3D12Resource*>(quintptr(tex->nativeTexture().object));
        ra.currentCommandList = [](const void* opaque) {
            auto widget = static_cast<const ThisClass*>(opaque);
            if (!widget->m_cb)
                return (ID3D12GraphicsCommandList*)nullptr;
            auto d3d12CbNat = static_cast<const QRhiD3D12CommandBufferNativeHandles*>(widget->m_cb->nativeHandles());
            return (ID3D12GraphicsCommandList*)d3d12CbNat->commandList;
        };
        ra.opaque = this;
        m_player->setRenderAPI(&ra, this);
    }
    break;
#endif
    default:
        return;
    }
    const auto rtSize = renderTarget()->pixelSize();
    m_player->updateRenderer(this, rtSize.width(), rtSize.height());
    //m_player->setBackgroundColor(0.6f, 0.0f, 0.0f, 0.9f, this);
}

void QMDKRhiWidgetRenderer::render(QRhiCommandBuffer *cb)
{
    // legacy gfx api: emulated command encoder
    // modern api: RenderAPI must get encoder if use begin/endPass
    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 0.8f);
    cb->beginPass(renderTarget(), clearColor, {1.0f, 0}, nullptr, QRhiCommandBuffer::BeginPassFlag::ExternalContent);
    cb->beginExternal();  // execute commands. gl/dx11 commands(timestamp query etc.) are queued(emulated command buffer) by qrhi

    m_cb = cb;
    m_player->renderVideo(this);

    cb->endExternal();
    cb->endPass();

    if (m_vsync)
        update();
}
