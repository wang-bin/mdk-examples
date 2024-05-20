#include "RhiVideoWidget.h"
#include <vector>
#include <private/qrhigles2_p.h>
#ifdef Q_OS_WIN
#include <d3d11.h>
#include <d3d12.h>
#endif
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"

using namespace MDK_NS;
using namespace std;

RhiVideoWidget::RhiVideoWidget(QWidget *parent)
    : QRhiWidget(parent)
    , m_player(make_unique<Player>())
{
    // if no vsync
    m_player->setRenderCallback([](void* vid) {
        if (!vid)
            return;
        auto wgt = (RhiVideoWidget*)vid;
        if (!wgt->m_vsync)
            wgt->update();
    });
}

RhiVideoWidget::~RhiVideoWidget()
{
    m_player->setVideoSurfaceSize(-1, -1, this);
}

void RhiVideoWidget::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    m_player->setDecoders(MediaType::Video, v);
}

void RhiVideoWidget::setMedia(const QString &url)
{
    m_player->setMedia(url.toUtf8().constData());
}

void RhiVideoWidget::play()
{
    m_player->set(State::Playing);
}

void RhiVideoWidget::pause(bool value)
{
    if (value)
        m_player->set(State::Paused);
    else
        play();
}

void RhiVideoWidget::stop()
{
    m_player->set(State::Stopped);
}

bool RhiVideoWidget::isPaused() const
{
    return m_player->state() == State::Paused;
}

void RhiVideoWidget::seek(qint64 ms)
{
    m_player->seek(ms);
}

qint64 RhiVideoWidget::position() const
{
    return m_player->position();
}


void RhiVideoWidget::initialize(QRhiCommandBuffer *cb)
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
            auto widget = static_cast<RhiVideoWidget*>(opaque);
            const auto tf = widget->colorBufferFormat();
            const auto sz = widget->renderTarget()->pixelSize();
            *w = sz.width();
            *h = sz.height();
            *fmt = tf == QRhiWidget::TextureFormat::RGBA16F ? VK_FORMAT_R16G16B16A16_SFLOAT : tf == QRhiWidget::TextureFormat::RGB10A2 ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        ra.currentCommandBuffer = [](void* opaque) -> VkCommandBuffer{
            auto widget = static_cast<RhiVideoWidget*>(opaque);
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
        m_player->setRenderAPI(&ra, this);
    }
        break;
#endif
    default:
        return;
    }
    const auto rtSize = renderTarget()->pixelSize();
    m_player->setVideoSurfaceSize(rtSize.width(), rtSize.height(), this);
}

void RhiVideoWidget::render(QRhiCommandBuffer *cb)
{
    if (api() == Api::OpenGL) {
        cb->beginPass(renderTarget(), {}, {1.0f, 0}, nullptr, QRhiCommandBuffer::BeginPassFlag::ExternalContent);
        cb->beginExternal();
    }

    m_cb = cb;
    m_player->renderVideo(this);

    if (api() == Api::OpenGL) {
        cb->endExternal();
        cb->endPass();
    }

    if (m_vsync)
        update();
}
