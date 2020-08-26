#pragma once
#include <QVulkanWindow>
#include <memory>

#include "mdk/RenderAPI.h"
#include "mdk/Player.h"

using namespace MDK_NS;

class VkMDKWindow : public QVulkanWindow
{
public:
    VkMDKWindow(QWindow *parent = nullptr);
    ~VkMDKWindow();
    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    void play();
    void pause();
    void stop();
    bool isPaused() const;
    void seek(qint64 ms);
    qint64 position() const;
    //void snapshot();

    QVulkanWindowRenderer *createRenderer() override;
private:
    std::shared_ptr<Player> player_;
};

