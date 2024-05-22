#include "RhiVideoWindow.h"
#include <QGuiApplication>

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
# include <private/qrhi_p.h>
# include <private/qrhivulkan_p.h>
#endif

QString graphicsApiName(QRhi::Implementation api)
{
    switch (api) {
    case QRhi::Null:
        return QLatin1String("Null (no output)");
    case QRhi::OpenGLES2:
        return QLatin1String("OpenGL");
    case QRhi::Vulkan:
        return QLatin1String("Vulkan");
    case QRhi::D3D11:
        return QLatin1String("Direct3D 11");
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QRhi::D3D12:
        return QLatin1String("Direct3D 12");
#endif
    case QRhi::Metal:
        return QLatin1String("Metal");
    }
    return QString();
}

int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);

    QRhi::Implementation graphicsApi;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QRhiSwapChain::Format format = QRhiSwapChain::Format::SDR;
#endif
// Use platform-specific defaults when no command-line arguments given.
#if defined(Q_OS_WIN)
    graphicsApi = QRhi::D3D11;
#elif (__APPLE__ + 0)// QT_CONFIG(metal)
    graphicsApi = QRhi::Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = QRhi::Vulkan;
#else
    graphicsApi = QRhi::OpenGLES2;
#endif
    auto i = app.arguments().indexOf("-rhi");
    if (i > 0) {
        const auto v = app.arguments().at(i+1).toLower();
        if (v.contains("gl"))
            graphicsApi = QRhi::OpenGLES2;
        else if (v.contains("11"))
            graphicsApi = QRhi::D3D11;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        else if (v.contains("12"))
            graphicsApi = QRhi::D3D12;
#endif
        else if (v == "vk" || v == "vulkan")
            graphicsApi = QRhi::Vulkan;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    i = app.arguments().indexOf("-hdr10");
    if (i > 0)
        format = QRhiSwapChain::Format::HDR10;
    i = app.arguments().indexOf("-srgbl");
    if (i > 0)
        format = QRhiSwapChain::Format::HDRExtendedSrgbLinear;
    i = app.arguments().indexOf("-scrgb");
    if (i > 0)
        format = QRhiSwapChain::Format::HDRExtendedSrgbLinear;
#endif
    //! [api-setup]
    // For OpenGL, to ensure there is a depth/stencil buffer for the window.
    // With other APIs this is under the application's control (QRhiRenderBuffer etc.)
    // and so no special setup is needed for those.
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    // Special case macOS to allow using OpenGL there.
    // (the default Metal is the recommended approach, though)
    // gl_VertexID is a GLSL 130 feature, and so the default OpenGL 2.1 context
    // we get on macOS is not sufficient.
#ifdef Q_OS_MACOS
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
#endif
    QSurfaceFormat::setDefaultFormat(fmt);

// For Vulkan.
#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == QRhi::Vulkan) {
        // Request validation, if available. This is completely optional
        // and has a performance impact, and should be avoided in production use.
        inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
        // Play nice with QRhi.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) // ?
        inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
#endif
        if (!inst.create()) {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = QRhi::OpenGLES2;
        }
    }
#endif
    //! [api-setup]

    RhiVideoWindow window(graphicsApi
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                          , format
#endif
                          );
    i = app.arguments().indexOf("-c:v");
    if (i > 0)
        window.setDecoders(app.arguments().at(i+1).split(","));

#if QT_CONFIG(vulkan)
    if (graphicsApi == QRhi::Vulkan)
        window.setVulkanInstance(&inst);
#endif
    window.resize(1280, 720);
    window.setTitle(QCoreApplication::applicationName() + QLatin1String(" - ") + graphicsApiName(graphicsApi));
    window.show();
    window.setMedia(app.arguments().last());
    window.play();

    int ret = app.exec();

    // RhiWindow::event() will not get invoked when the
    // PlatformSurfaceAboutToBeDestroyed event is sent during the QWindow
    // destruction. That happens only when exiting via app::quit() instead of
    // the more common QWindow::close(). Take care of it: if the QPlatformWindow
    // is still around (there was no close() yet), get rid of the swapchain
    // while it's not too late.
    if (window.handle())
        window.releaseSwapChain();

    return ret;
}
