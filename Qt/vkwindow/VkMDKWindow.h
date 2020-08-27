#pragma once
#include <QVulkanWindow>
#include <memory>

namespace mdk {
class Player;
}

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
    std::shared_ptr<mdk::Player> player_;
};

