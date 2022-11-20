#pragma once
#include <QGraphicsWidget>
#include <QOpenGLTextureBlitter>
#include <QOpenGLFramebufferObject>

namespace mdk {
class Player;
}
class GraphicsVideoItem : public QGraphicsWidget
{
public:
    GraphicsVideoItem(QGraphicsItem * parent = nullptr);
    ~GraphicsVideoItem();
    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    void play();
    void pause();
    void stop();
    bool isPaused() const;
    void seek(qint64 ms, bool accurate = false);
    qint64 position() const;
    qint64 duration() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
protected:
    bool event(QEvent *event) override;

private:
    std::shared_ptr<mdk::Player> player_;
    QOpenGLFramebufferObject *fbo_ = nullptr;
    QOpenGLTextureBlitter *tb_ = nullptr;
    QSize fboSize_ = {};
};
