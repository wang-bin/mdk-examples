#include "VkMDKWindow.h"
#include <QVulkanFunctions>
#include <QGuiApplication>
#include "mdk/RenderAPI.h"
#include "mdk/Player.h"

using namespace MDK_NS;
using namespace std;

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow *w, Player* player)
        : m_window(w)
    , player_(player) {

    }

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    QVulkanWindow *m_window;
    Player* player_ = nullptr;
};

QVulkanWindowRenderer *VkMDKWindow::createRenderer()
{
    return new VulkanRenderer(this, player_.get());
}

void VulkanRenderer::initResources()
{
    qDebug("initResources");
}

void VulkanRenderer::initSwapChainResources()
{
    const auto size = m_window->swapChainImageSize();
    player_->setVideoSurfaceSize(size.width(), size.height());
    qDebug("initSwapChainResources");
    // called when swapchain is recreated, e.g. device lost, resize
    VulkanRenderAPI ra;
    ra.instance = m_window->vulkanInstance()->vkInstance();
    ra.device = m_window->device();
    ra.phy_device = m_window->physicalDevice();
    ra.graphics_queue = m_window->graphicsQueue(); // optional but recommended
    ra.graphics_family = m_window->graphicsQueueFamilyIndex(); // must
    ra.render_pass = m_window->defaultRenderPass(); //
    ra.opaque = m_window;
    ra.renderTargetInfo = [](void* opaque, int* w, int* h, VkFormat*, VkImageLayout*) {
        auto win = static_cast<QVulkanWindow*>(opaque);
        const auto s = win->swapChainImageSize();
        *w = s.width();
        *h = s.height();
        return win->swapChainImageCount();
    };
    ra.beginFrame = [](void* opaque, VkImageView* view/* = nullptr*/, VkFramebuffer* fb/*= nullptr*/, VkSemaphore* imgSem/* = nullptr*/){
        auto win = static_cast<QVulkanWindow*>(opaque);
        *fb = win->currentFramebuffer();
        return win->currentSwapChainImageIndex();
    };
    ra.currentCommandBuffer = [](void* opaque){
        auto win = static_cast<QVulkanWindow*>(opaque);
        return win->currentCommandBuffer();
    };
    player_->setRenderAPI(&ra); // will recreate resources


}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");
    player_->setVideoSurfaceSize(-1, -1);
}

void VulkanRenderer::startNextFrame()
{
    player_->renderVideo();

    m_window->frameReady();
    // TODO: why not update in callback of setRenderCallback()
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate. call on gui thread only
}


VkMDKWindow::VkMDKWindow(QWindow *parent)
    : QVulkanWindow(parent)
    , player_(std::make_shared<Player>())
{
    // no need to set callback if requestUpdate in startNextFrame()
    //player_->setRenderCallback([this](void*){
        //QCoreApplication::instance()->postEvent(this, new QEvent(QEvent::UpdateRequest), INT_MAX);
    //});
}

VkMDKWindow::~VkMDKWindow() = default;

void VkMDKWindow::setDecoders(const QStringList &dec)
{
    std::vector<std::string> v;
    foreach (QString d, dec) {
        v.push_back(d.toStdString());
    }
    player_->setDecoders(MediaType::Video, (v);
}

void VkMDKWindow::setMedia(const QString &url)
{
    player_->setMedia(url.toUtf8().constData());
}

void VkMDKWindow::play()
{
    player_->set(State::Playing);
}

void VkMDKWindow::pause()
{
    player_->set(State::Paused);
}

void VkMDKWindow::stop()
{
    player_->set(State::Stopped);
}

bool VkMDKWindow::isPaused() const
{
    return player_->state() == State::Paused;
}

void VkMDKWindow::seek(qint64 ms)
{
    player_->seek(ms);
}

qint64 VkMDKWindow::position() const
{
    return player_->position();
}
