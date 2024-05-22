#ifndef MDK_RHIVIDEOWINDOW_H
#define MDK_RHIVIDEOWINDOW_H


#include <QWindow>
#include <QOffscreenSurface>

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
# include <private/qrhi_p.h>
#else
# include <rhi/qrhi.h>
#endif
#include "mdk/global.h"

namespace MDK_NS {
class Player;
}

class RhiWindow : public QWindow
{
public:
    RhiWindow(QRhi::Implementation graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
              , QRhiSwapChain::Format format = QRhiSwapChain::Format::SDR
#endif
              );
    void releaseSwapChain();

protected:
    virtual void customInit() {}
    virtual void customRender() {}

    QRhi* rhi() const { return m_rhi.get(); }
    QRhiSwapChain* swapChain() const { return m_sc.get(); }
    QRhiRenderPassDescriptor* renderPassDescriptor() const { return m_rp.get(); }

private:
// destruction order matters to a certain degree: the fallbackSurface must
// outlive the rhi, the rhi must outlive all other resources.  The resources
// need no special order when destroying.
#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;
    //! [swapchain-data]
    std::unique_ptr<QRhiSwapChain> m_sc;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;
    //! [swapchain-data]
    bool m_hasSwapChain = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QRhiSwapChain::Format m_format = QRhiSwapChain::Format::SDR;
#endif
private:
    void init();
    void resizeSwapChain();
    void render();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    QRhi::Implementation m_graphicsApi;
    bool m_initialized = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;
};


class RhiVideoWindow : public RhiWindow
{
    Q_OBJECT
public:
    RhiVideoWindow(QRhi::Implementation graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                   , QRhiSwapChain::Format format = QRhiSwapChain::Format::SDR
#endif
                   );
    ~RhiVideoWindow();

    void setVsync(bool value) { m_vsync = value; }

    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    bool isPaused() const;
    void seek(qint64 ms);
    qint64 position() const;

public slots:
    void play();
    void pause(bool value = true);
    void stop();
protected:
    void customInit() override;
    void customRender() override;
private:
    std::unique_ptr<MDK_NS::Player> m_player;
    bool m_vsync = false;
};

#endif // MDK_RHIVIDEOWINDOW_H
