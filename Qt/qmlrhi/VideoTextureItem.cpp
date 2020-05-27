/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#include "VideoTextureItem.h"
#include <QtGui/QScreen>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>
#if (__APPLE__+0)
#include <Metal/Metal.h>
#endif

#if QT_CONFIG(opengl)
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#endif

#include "mdk/RenderAPI.h"
using namespace std;

//! [1]
class VideoTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT
public:
    VideoTextureNode(VideoTextureItem *item);
    ~VideoTextureNode();

    QSGTexture *texture() const override;

    void sync();
//! [1]
private slots:
    void render();

private:
    QQuickItem *m_item;
    QQuickWindow *m_window;
    QSize m_size;
    qreal m_dpr;
#if (__APPLE__+0)
    id<MTLTexture> m_texture_mtl = nil;
#endif
#if QT_CONFIG(opengl)
    unique_ptr<QOpenGLFramebufferObject> fbo_gl;
#endif

    std::shared_ptr<Player> m_player;
};

VideoTextureItem::VideoTextureItem()
{
    setFlag(ItemHasContents, true);
    m_player = make_shared<Player>();
    //m_player->setVideoDecoders({"VT", "FFmpeg"});
    m_player->setRenderCallback([=](void *){
        QMetaObject::invokeMethod(this, "update"); // FIXME: crash on quit
    });
}

// The beauty of using a true QSGNode: no need for complicated cleanup
// arrangements, unlike in other examples like metalunderqml, because the
// scenegraph will handle destroying the node at the appropriate time.
void VideoTextureItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    m_node = nullptr;
}

void VideoTextureItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    m_node = nullptr;
}

//! [2]
QSGNode *VideoTextureItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    auto n = static_cast<VideoTextureNode*>(node);

    if (!n && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!n) {
        m_node = new VideoTextureNode(this);
        n = m_node;
    }

    m_node->sync();

    n->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    n->setFiltering(QSGTexture::Linear);
    n->setRect(0, 0, width(), height());

    window()->update(); // ensure getting to beforeRendering() at some point

    return n;
}
//! [2]

void VideoTextureItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
        update();
}

void VideoTextureItem::setSource(const QString & s)
{
    m_player->setMedia(s.toLocal8Bit().data());
    m_source = s;
    emit sourceChanged();
}

void VideoTextureItem::play()
{
    m_player->setState(PlaybackState::Playing);
}

//! [3]
VideoTextureNode::VideoTextureNode(VideoTextureItem *item)
    : m_item(item)
    , m_player(item->m_player)
{
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &VideoTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });
//! [3]
}

VideoTextureNode::~VideoTextureNode()
{
    // release gfx resources
#if QT_CONFIG(opengl)
    fbo_gl.reset();
#endif
    qDebug("renderer destroyed");
}

QSGTexture *VideoTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void VideoTextureNode::sync()
{
    m_dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_window->size() * m_dpr;
    bool needsNew = false;

    if (!texture())
        needsNew = true;

    if (newSize != m_size) {
        needsNew = true;
        m_size = newSize;
    }

    if (!needsNew)
        return;

    delete texture();
    QSGRendererInterface *rif = m_window->rendererInterface();
    switch (rif->graphicsApi()) {
    case QSGRendererInterface::OpenGL:
        Q_FALLTHROUGH();
    case QSGRendererInterface::OpenGLRhi: { // FIXME: OpenGLRhi does not work
#if QT_CONFIG(opengl)
        fbo_gl.reset(new QOpenGLFramebufferObject(m_size));
        auto tex = fbo_gl->texture();
        QSGTexture *wrapper = m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture,
                                                                      &tex,
                                                                      0,
                                                                      m_size);
        setTexture(wrapper);
        qDebug() << "Got QSGTexture wrapper" << wrapper << "for an OpenGL texture '" << tex << "' of size" << m_size;
        m_player->scale(1.0f, -1.0f); // flip y
#endif
    }
        break;
    case QSGRendererInterface::MetalRhi: {
#if (__APPLE__+0)
        auto dev = (__bridge id<MTLDevice>)rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(dev);

        MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureType2D;
        desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        desc.width = m_size.width();
        desc.height = m_size.height();
        desc.mipmapLevelCount = 1;
        desc.resourceOptions = MTLResourceStorageModePrivate;
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        m_texture_mtl = [dev newTextureWithDescriptor: desc];

        QSGTexture *wrapper = m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture,
                                                                      &m_texture_mtl,
                                                                      0,
                                                                      m_size);
        setTexture(wrapper);

        qDebug() << "Got QSGTexture wrapper" << wrapper << "for an MTLTexture of size" << m_size;

        MetalRenderAPI ra{};
        ra.texture = (__bridge void*)m_texture_mtl;
        ra.device = (__bridge void*)dev;
        ra.cmdQueue = rif->getResource(m_window, QSGRendererInterface::CommandQueueResource);
        m_player->setRenderAPI(&ra);
#endif
    }
        break;
    default:
        break;
    }
    m_player->setVideoSurfaceSize(m_size.width(), m_size.height());
}

// This is hooked up to beforeRendering() so we can start our own render
// command encoder. If we instead wanted to use the scenegraph's render command
// encoder (targeting the window), it should be connected to
// beforeRenderPassRecording() instead.
//! [6]
void VideoTextureNode::render()
{
#if QT_CONFIG(opengl)
    GLuint prevFbo = 0;
    if (fbo_gl) {
        auto f = QOpenGLContext::currentContext()->functions();
        f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&prevFbo);
        fbo_gl->bind();
    }
#endif
    m_player->renderVideo();
#if QT_CONFIG(opengl)
    if (fbo_gl) {
        auto f = QOpenGLContext::currentContext()->functions();
        f->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
        //fbo_gl->toImage().save("/tmp/fbo.jpg");
    }
#endif
}

#include "VideoTextureItem.moc"
