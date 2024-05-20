#ifndef MDK_RHIVIDEOWIDGET_H
#define MDK_RHIVIDEOWIDGET_H

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include "mdk/global.h"

namespace MDK_NS {
class Player;
}
class RhiVideoWidget : public QRhiWidget
{
    Q_OBJECT

public:
    RhiVideoWidget(QWidget *parent = nullptr);
    ~RhiVideoWidget();

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
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
private:
    std::unique_ptr<MDK_NS::Player> m_player;
    QRhiCommandBuffer *m_cb = nullptr;
    bool m_vsync = false;
};
#endif // MDK_RHIVIDEOWIDGET_H
