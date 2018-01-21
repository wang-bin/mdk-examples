#include "pch.h"
#include "OpenGLESPage.xaml.h"
#include "mdk/Player.h"
using namespace std;
using namespace MDK_NS;

using namespace XamlMDK;
using namespace Platform;
using namespace Concurrency;
using namespace Windows::Foundation;

// Helper to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
inline float ConvertDipsToPixels(float dips)
{
	const float dpi = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
	static const float dipsPerInch = 96.0f;
	return floor(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

OpenGLESPage::OpenGLESPage() :
    OpenGLESPage(nullptr)
{
}

OpenGLESPage::OpenGLESPage(OpenGLES* openGLES) :
    mOpenGLES(openGLES),
    mRenderSurface(EGL_NO_SURFACE),
	mPlayer(make_unique<Player>())
{
	setLogHandler([](LogLevel, const char* msg) {
		OutputDebugStringA(msg);
	});
    InitializeComponent();

    Windows::UI::Core::CoreWindow^ window = Windows::UI::Xaml::Window::Current->CoreWindow;
    window->VisibilityChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^>(this, &OpenGLESPage::OnVisibilityChanged);
    this->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &OpenGLESPage::OnPageLoaded);
}

OpenGLESPage::~OpenGLESPage()
{
	mPlayer->setState(State::Stopped);
    StopRenderLoop();
    DestroyRenderSurface();
}

void OpenGLESPage::OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    // The SwapChainPanel has been created and arranged in the page layout, so EGL can be initialized.
    CreateRenderSurface();
    StartRenderLoop();
}

void OpenGLESPage::OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	// swapChainPanel size is already in pixels
	mPlayer->setVideoSurfaceSize(e->NewSize.Width, e->NewSize.Height);
}

void OpenGLESPage::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
    if (args->Visible && mRenderSurface != EGL_NO_SURFACE)
        StartRenderLoop();
    else
        StopRenderLoop();
}

void OpenGLESPage::CreateRenderSurface()
{
#ifdef FOREIGN_EGL
	if (mOpenGLES && mRenderSurface == EGL_NO_SURFACE) {
        // The app can configure the the SwapChainPanel which may boost performance.
        // By default, this template uses the default configuration.
        mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, nullptr);
        // You can configure the SwapChainPanel to render at a lower resolution and be scaled up to
        // the swapchain panel size. This scaling is often free on mobile hardware.
        //
        // One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
        // Size customRenderSurfaceSize = Size(800, 600);
        // mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, &customRenderSurfaceSize, nullptr);
        //
        // Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
        // float customResolutionScale = 0.5f;
        // mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, &customResolutionScale);
    }
#else
	OutputDebugStringW(GetEnvironmentStrings());
	SetEnvironmentVariableA("GL_MAJOR", "3");
	SetEnvironmentVariableA("GL_MINOR", "2");
	mPlayer->updateNativeWindow(reinterpret_cast<IInspectable*>(swapChainPanel));// reinterpret_cast<IInspectable*>(window));
#endif
	swapChainPanel->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &OpenGLESPage::OnSizeChanged);
	mPlayer->setVideoSurfaceSize(swapChainPanel->Width, swapChainPanel->Height);
	mPlayer->setVideoDecoders({ "D3D11", "FFmpeg" });
	mPlayer->setMedia("rtmp://live.hkstv.hk.lxdns.com/live/hks");
	mPlayer->setState(PlaybackState::Playing);
}

void OpenGLESPage::DestroyRenderSurface()
{
    if (mOpenGLES)
        mOpenGLES->DestroySurface(mRenderSurface);
    mRenderSurface = EGL_NO_SURFACE;
}

void OpenGLESPage::RecoverFromLostDevice()
{
    // Stop the render loop, reset OpenGLES, recreate the render surface
    // and start the render loop again to recover from a lost device.
    StopRenderLoop();
    {
        critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);
        DestroyRenderSurface();
        mOpenGLES->Reset();
        CreateRenderSurface();
    }
    StartRenderLoop();
}

void OpenGLESPage::StartRenderLoop()
{
#ifndef FOREIGN_EGL
	return;
#endif
    // If the render loop is already running then do not start another thread.
    if (mRenderLoopWorker != nullptr && mRenderLoopWorker->Status == Windows::Foundation::AsyncStatus::Started)
        return;
    // Create a task for rendering that will be run on a background thread.
    auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler([this](Windows::Foundation::IAsyncAction ^ action)
    {
        critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);
        mOpenGLES->MakeCurrent(mRenderSurface);
        while (action->Status == Windows::Foundation::AsyncStatus::Started) {
            // Logic to update the scene could go here
			mPlayer->renderVideo();
            // The call to eglSwapBuffers might not be successful (i.e. due to Device Lost)
            // If the call fails, then we must reinitialize EGL and the GL resources.
            if (mOpenGLES->SwapBuffers(mRenderSurface) != GL_TRUE) {
                // XAML objects like the SwapChainPanel must only be manipulated on the UI thread.
                swapChainPanel->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([=]()
                {
                    RecoverFromLostDevice();
                }, CallbackContext::Any));
                return;
            }
        }
    });
    // Run task on a dedicated high priority background thread.
    mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void OpenGLESPage::StopRenderLoop()
{
    if (mRenderLoopWorker)
        mRenderLoopWorker->Cancel();
	mRenderLoopWorker = nullptr;
}