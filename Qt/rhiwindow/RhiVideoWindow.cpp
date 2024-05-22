#include "RhiVideoWindow.h"
#include <QPlatformSurfaceEvent>

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
# include <private/qrhigles2_p.h>
# include <private/qrhinull_p.h>
# ifdef Q_OS_WIN
#  include <private/qrhid3d11_p.h>
# endif
# if (__APPLE__ +0)
#  include <private/qrhimetal_p.h>
# endif
# if QT_CONFIG(vulkan)
#  include <private/qrhivulkan_p.h>
# endif
#endif
#include <vector>
#ifdef Q_OS_WIN
#include <d3d11.h>
#include <d3d12.h>
#endif
#if (__OBJC__ + 0)
#import <Metal/MTLPixelFormat.h>.h>
#import <QuartzCore/CAMetalLayer.h>
#endif
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
#define Direct3DSurface OpenGLSurface
#endif

RhiWindow::RhiWindow(QRhi::Implementation graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                     , QRhiSwapChain::Format format
#endif
                     )
    : m_graphicsApi(graphicsApi)
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    , m_format(format)
#endif
{
    switch (graphicsApi) {
    case QRhi::OpenGLES2:
        setSurfaceType(OpenGLSurface);
        break;
    case QRhi::Vulkan:
        setSurfaceType(VulkanSurface);
        break;
    case QRhi::D3D11:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QRhi::D3D12:
#endif
        setSurfaceType(Direct3DSurface); // OpenGLSurface before qt6.1
        break;
    case QRhi::Metal:
        setSurfaceType(MetalSurface);
        break;
    case QRhi::Null:
        break; // RasterSurface
    }
}

//! [expose]
void RhiWindow::exposeEvent(QExposeEvent *)
{
    // initialize and start rendering when the window becomes usable for graphics purposes
    if (isExposed() && !m_initialized) {
        init();
        resizeSwapChain();
        m_initialized = true;
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

    // stop pushing frames when not exposed (or size is 0)
    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
        m_notExposed = true;

    // Continue when exposed again and the surface has a valid size. Note that
    // surfaceSize can be (0, 0) even though size() reports a valid one, hence
    // trusting surfacePixelSize() and not QWindow.
    if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    // always render a frame on exposeEvent() (when exposed) in order to update
    // immediately on window resize.
    if (isExposed() && !surfaceSize.isEmpty())
        render();
}
//! [expose]

//! [event]
bool RhiWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}
//! [event]

//! [rhi-init]
void RhiWindow::init()
{
    if (m_graphicsApi == QRhi::Null) {
        QRhiNullInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Null, &params));
    }

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    }
#endif
#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
        QRhiD3D11InitParams params;
        // Enable the debug layer, if available. This is optional
        // and should be avoided in production builds.
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    } else if (m_graphicsApi == QRhi::D3D12) {
        QRhiD3D12InitParams params;
        // Enable the debug layer, if available. This is optional
        // and should be avoided in production builds.
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D12, &params));
#endif
    }
#endif

#if (__APPLE__ + 0) //QT_CONFIG(metal)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params));
    }
#endif

    if (!m_rhi)
        qFatal("Failed to create RHI backend");
    //! [rhi-init]

    //! [swapchain-init]
    m_sc.reset(m_rhi->newSwapChain());
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    m_sc->setFormat(m_format);
#endif
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                      QSize(), // no need to set the size here, due to UsedWithSwapChainOnly
                                      1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());
    //! [swapchain-init]

    customInit();
}

//! [swapchain-resize]
void RhiWindow::resizeSwapChain()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds
#else
    m_hasSwapChain = m_sc->buildOrResize(); // also handles m_ds
#endif
    const QSize outputSize = m_sc->currentPixelSize();
    //m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    // TODO:
}
//! [swapchain-resize]

void RhiWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_sc->destroy();
#else
        m_sc->release();
#endif
    }
}

//! [render-precheck]
void RhiWindow::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;
    //! [render-precheck]

    //! [render-resize]
    // If the window got resized or newly exposed, resize the swapchain. (the
    // newly-exposed case is not actually required by some platforms, but is
    // here for robustness and portability)
    //
    // This (exposeEvent + the logic here) is the only safe way to perform
    // resize handling. Note the usage of the RHI's surfacePixelSize(), and
    // never QWindow::size(). (the two may or may not be the same under the hood,
    // depending on the backend and platform)
    //
    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }
    //! [render-resize]

    //! [beginframe]
    QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        result = m_rhi->beginFrame(m_sc.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, will retry", result);
        requestUpdate();
        return;
    }

    customRender();
    //! [beginframe]

    //! [request-update]
    m_rhi->endFrame(m_sc.get());

    // Always request the next frame via requestUpdate(). On some platforms this is backed
    // by a platform-specific solution, e.g. CVDisplayLink on macOS, which is potentially
    // more efficient than a timer, queued metacalls, etc.
    //requestUpdate();
}


using namespace MDK_NS;
using namespace std;

RhiVideoWindow::RhiVideoWindow(QRhi::Implementation graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                               , QRhiSwapChain::Format format
#endif
                               )
    : RhiWindow(graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                , format
#endif
                )
    , m_player(make_unique<Player>())
{
    setLogHandler([](auto, auto msg){
        qDebug("%s", msg);
    });
    m_player->setLoop(-1);
    m_player->setRenderCallback([](void* vid) {
        if (!vid)
            return;
        auto win = (RhiVideoWindow*)vid;
        if (!win->m_vsync) {
            QMetaObject::invokeMethod(win, "requestUpdate", Qt::QueuedConnection);
        }
    });
}

RhiVideoWindow::~RhiVideoWindow()
{
    m_player->setVideoSurfaceSize(-1, -1, this);
}

void RhiVideoWindow::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    m_player->setDecoders(MediaType::Video, v);
}

void RhiVideoWindow::setMedia(const QString &url)
{
    m_player->setMedia(url.toUtf8().constData());
}

void RhiVideoWindow::play()
{
    m_player->set(State::Playing);
}

void RhiVideoWindow::pause(bool value)
{
    if (value)
        m_player->set(State::Paused);
    else
        play();
}

void RhiVideoWindow::stop()
{
    m_player->set(State::Stopped);
}

bool RhiVideoWindow::isPaused() const
{
    return m_player->state() == State::Paused;
}

void RhiVideoWindow::seek(qint64 ms)
{
    m_player->seek(ms);
}

qint64 RhiVideoWindow::position() const
{
    return m_player->position();
}


void RhiVideoWindow::customInit()
{
    const QRhiNativeHandles *nat = rhi()->nativeHandles();
    if (!nat)
        return;

    switch (rhi()->backend()) {
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: break;
#endif
#if (__APPLE__ + 0)
    case QRhi::Metal: {
        const auto *mtlnat = static_cast<const QRhiMetalNativeHandles*>(nat);
        MetalRenderAPI ra{};
        ra.opaque = this;
        ra.currentCommand = [](const void** enc, const void** cmdBuf, const void *opaque) {
            const auto win = (RhiVideoWindow*)opaque;
            const auto cb = win->swapChain()->currentFrameCommandBuffer();
            const auto mtlCbNat = static_cast<const QRhiMetalCommandBufferNativeHandles*>(cb->nativeHandles());
            *enc = mtlCbNat->encoder;
            *cmdBuf = mtlCbNat->commandBuffer;
        };
        ra.device = mtlnat->dev;
        ra.cmdQueue = mtlnat->cmdQueue;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)) && defined(__OBJC__)
        auto layer = (CAMetalLayer*)swapChain()->proxyData().reserved[0];
        //ra.format = layer.pixelFormat;
#endif
        ra.colorFormat = 80;
        ra.depthStencilFormat = 260; // MTLPixelFormatDepth32Float_Stencil8. TODO:
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        switch (swapChain()->format()) {
        case QRhiSwapChain::Format::HDRExtendedSrgbLinear:
            ra.colorFormat = 115;//MTLPixelFormatRGBA16Float;
            break;
        default: {
            if (swapChain()->flags().testFlag(QRhiSwapChain::Flag::sRGB))
                ra.colorFormat = 81;//MTLPixelFormatBGRA8Unorm_sRGB;
            else
                ra.colorFormat = 80;//MTLPixelFormatBGRA8Unorm;
        }
            break;
        }
#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_player->setRenderAPI(&ra, this);
    }
        break;
#endif
#if (VK_VERSION_1_0+0) && QT_CONFIG(vulkan)
    case QRhi::Vulkan: {
        auto vknat = static_cast<const QRhiVulkanNativeHandles*>(nat);
        const auto vkrpnat = static_cast<const QRhiVulkanRenderPassNativeHandles*>(renderPassDescriptor()->nativeHandles());
        VulkanRenderAPI ra{};
        ra.device = vknat->dev;
        ra.phy_device = vknat->physDev;
        ra.graphics_family = vknat->gfxQueueFamilyIdx;
        ra.graphics_queue = vknat->gfxQueue;
        ra.opaque = this;
        ra.render_pass = vkrpnat->renderPass;
        //ra.rt = (VkImage)tex->nativeTexture().object;
        ra.renderTargetInfo = [](void* opaque, int* w, int* h, VkFormat* fmt, VkImageLayout* layout) {
            auto win = static_cast<RhiVideoWindow*>(opaque);
            const auto sz = win->swapChain()->currentPixelSize();
            *w = sz.width();
            *h = sz.height();
            *fmt = VK_FORMAT_R8G8B8A8_UNORM;// TODO: tf == QRhiWidget::TextureFormat::RGBA16F ? VK_FORMAT_R16G16B16A16_SFLOAT : tf == QRhiWidget::TextureFormat::RGB10A2 ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 2;
        };
        ra.currentCommandBuffer = [](void* opaque) {
            auto win = static_cast<RhiVideoWindow*>(opaque);
            const auto cb = win->swapChain()->currentFrameCommandBuffer();
            auto vkCbNat = static_cast<const QRhiVulkanCommandBufferNativeHandles*>(cb->nativeHandles());
            return vkCbNat->commandBuffer;
        };
        m_player->setRenderAPI(&ra, this);
    }
        break;
#endif
#ifdef Q_OS_WIN
    case QRhi::D3D11: {
        auto d3dnat = static_cast<const QRhiD3D11NativeHandles*>(nat);
        D3D11RenderAPI ra{};
        ra.context = (ID3D11DeviceContext*)d3dnat->context;
        m_player->setRenderAPI(&ra, this);
    }
        break;
# if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QRhi::D3D12: {
        auto d3dnat = static_cast<const QRhiD3D12NativeHandles*>(nat);
        D3D12RenderAPI ra{};
        ra.cmdQueue = reinterpret_cast<ID3D12CommandQueue*>(d3dnat->commandQueue);
        switch (swapChain()->format()) {
        case QRhiSwapChain::Format::HDRExtendedSrgbLinear:
            ra.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            break;
        case QRhiSwapChain::Format::HDR10:
            ra.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
            break;
        default: {
            if (swapChain()->flags().testFlag(QRhiSwapChain::Flag::sRGB))
                ra.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            else
                ra.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        break;
        }
        ra.opaque = this;
        ra.currentCommandList = [](const void* opaque) {
            auto win = static_cast<const RhiVideoWindow*>(opaque);
            const auto cb = win->swapChain()->currentFrameCommandBuffer();
            auto cbnat = static_cast<const QRhiD3D12CommandBufferNativeHandles*>(cb->nativeHandles());
            return (ID3D12GraphicsCommandList*)cbnat->commandList;
        };
        m_player->setRenderAPI(&ra, this);
    }
        break;
# endif
#endif
    default:
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    switch (swapChain()->format()) {
    case QRhiSwapChain::Format::HDR10:
        m_player->set(ColorSpaceBT2100_PQ, this);
        break;
    case QRhiSwapChain::Format::HDRExtendedSrgbLinear: {
# if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
        if (swapChain()->hdrInfo().luminanceBehavior == QRhiSwapChainHdrInfo::SceneReferred) {
            m_player->set(ColorSpaceSCRGB, this);
        } else {
            m_player->set(ColorSpaceExtendedLinearSRGB, this);
        }
#else
# if (__APPLE__+0)
        m_player->set(ColorSpaceExtendedLinearSRGB, this);
# else
        m_player->set(ColorSpaceSCRGB, this);
# endif
#endif
        //csResetHack("scrgb");
    }
        break;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)) // p3
    case QRhiSwapChain::Format::HDRExtendedDisplayP3Linear:
        m_player->set(ColorSpaceExtendedLinearDisplayP3, this);
        //csResetHack("p3");
        break;
#endif
    default:
        break;
    }
#endif
}

void RhiVideoWindow::customRender()
{
    QRhiResourceUpdateBatch *resourceUpdates = rhi()->nextResourceUpdateBatch();
//! [render-1]

//! [render-cb]
    QRhiCommandBuffer *cb = swapChain()->currentFrameCommandBuffer();
    const QSize sz = swapChain()->currentPixelSize();
    m_player->setVideoSurfaceSize(sz.width(), sz.height(), this);
//! [render-cb]

//! [render-pass]
    cb->beginPass(swapChain()->currentFrameRenderTarget(), Qt::red, { 1.0f, 0 }, resourceUpdates
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                  , QRhiCommandBuffer::BeginPassFlag::ExternalContent
#endif
                  );
//! [render-pass]
    cb->setViewport({ 0, 0, float(sz.width()), float(sz.height()) });
    cb->beginExternal();

    m_player->renderVideo(this);

    cb->endExternal();
    cb->endPass();

    if (m_vsync)
        requestUpdate();
}
