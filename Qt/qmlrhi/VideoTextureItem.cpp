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
#if (_WIN32+0)
#include <d3d11.h>
#include <wrl/client.h>
using namespace Microsoft::WRL; //ComPtr
#endif
#if QT_CONFIG(opengl)
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#endif
#include "mdk/Player.h"
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
#if (_WIN32+0)
    ComPtr<ID3D11Texture2D> m_texture_d3d11;
    ComPtr<ID3D11RenderTargetView> m_rtv;
#endif
    std::weak_ptr<Player> m_player;
};

VideoTextureItem::VideoTextureItem()
{
    setFlag(ItemHasContents, true);
    m_player = make_shared<Player>();
    //m_player->setVideoDecoders({"VT", "MFT:d3d=11", "VAAPI", "FFmpeg"});
    m_player->setRenderCallback([=](void *){
        QMetaObject::invokeMethod(this, "update");
    });
}

VideoTextureItem::~VideoTextureItem() = default;

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
    delete texture();
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
    const QSizeF newSize = m_item->size() * m_dpr;
    bool needsNew = false;

    if (!texture())
        needsNew = true;

    if (newSize != m_size) {
        needsNew = true;
        m_size = {qRound(newSize.width()), qRound(newSize.height())};
    }

    if (!needsNew)
        return;

    delete texture();

    auto player = m_player.lock();
    if (!player)
        return;
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
        GLRenderAPI ra;
        ra.fbo = fbo_gl->handle();
        player->setRenderAPI(&ra);
        player->scale(1.0f, -1.0f); // flip y
#endif
    }
        break;
    case QSGRendererInterface::Direct3D11Rhi: {
#if (_WIN32)
        auto dev = (ID3D11Device*)rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        D3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, m_size.width(), m_size.height(), 1, 1
                                                          , D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET
                                                          , D3D11_USAGE_DEFAULT, 0, 1, 0, 0);
        if (FAILED(dev->CreateTexture2D(&desc, nullptr, &m_texture_d3d11))) {

        }
        QSGTexture *wrapper = m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture,
                                                                      m_texture_d3d11.GetAddressOf(),
                                                                      0,
                                                                      m_size);
        setTexture(wrapper);
        qDebug() << "Got QSGTexture wrapper" << wrapper << "for an D3D11 texture of size" << m_size;
        D3D11RenderAPI ra;
        ComPtr<ID3D11DeviceContext> ctx;
        dev->GetImmediateContext(&ctx);
        ra.context = ctx.Get();
        dev->CreateRenderTargetView(m_texture_d3d11.Get(), nullptr, &m_rtv);
        ra.rtv = m_rtv.Get();
        player->setRenderAPI(&ra);
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
        player->setRenderAPI(&ra);
#endif
    }
        break;
    default:
        break;
    }
    player->setVideoSurfaceSize(m_size.width(), m_size.height());
}

// This is hooked up to beforeRendering() so we can start our own render
// command encoder. If we instead wanted to use the scenegraph's render command
// encoder (targeting the window), it should be connected to
// beforeRenderPassRecording() instead.
//! [6]
void VideoTextureNode::render()
{
    auto player = m_player.lock();
    if (!player)
        return;
    player->renderVideo();
}

#include "VideoTextureItem.moc"
