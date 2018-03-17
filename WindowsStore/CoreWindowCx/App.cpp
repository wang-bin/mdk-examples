//
// This file demonstrates how to initialize EGL in a Windows Store app, using ICoreWindow.
//
#include "pch.h"
#include "app.h"
#include "mdk/Player.h"

//#define FOREIGN_EGL
using namespace std;
using namespace MDK_NS;

using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Microsoft::WRL;
using namespace Platform;

using namespace CoreWindowMDK;

// Helper to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
inline float ConvertDipsToPixels(float dips)
{
	const float dpi = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
	static const float dipsPerInch = 96.0f;
	return floor(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

// Implementation of the IFrameworkViewSource interface, necessary to run our app.
ref class SimpleApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
    {
        return ref new App();
    }
};

// The main function creates an IFrameworkViewSource for our app, and runs the app.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    setLogHandler([](LogLevel, const char* msg) {
		OutputDebugStringA(msg);
	});
    auto simpleApplicationSource = ref new SimpleApplicationSource();
    CoreApplication::Run(simpleApplicationSource);
    return 0;
}

App::App() :
    mWindowClosed(false),
    mWindowVisible(true),
    mEglDisplay(EGL_NO_DISPLAY),
    mEglContext(EGL_NO_CONTEXT),
    mEglSurface(EGL_NO_SURFACE),
	mPlayer(make_unique<Player>())
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
	// Logic for other event handlers could go here.
	// Information about the Suspending and Resuming event handlers can be found here:
	// http://msdn.microsoft.com/en-us/library/windows/apps/xaml/hh994930.aspx
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);
	window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnSizeChanged);
	// The CoreWindow has been created, so EGL can be initialized.
	InitializeEGL(window);
}

// Initializes scene resources
void App::Load(Platform::String^ entryPoint)
{
	RecreateRenderer();
}

void App::RecreateRenderer()
{
}

// This method is called after the window becomes active.
void App::Run()
{
#ifdef FOREIGN_EGL
	while (!mWindowClosed) {
		if (mWindowVisible) {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			mPlayer->renderVideo();
			// The call to eglSwapBuffers might not be successful (e.g. due to Device Lost)
			if (eglSwapBuffers(mEglDisplay, mEglSurface) != GL_TRUE) {
				CleanupEGL();
				InitializeEGL(CoreWindow::GetForCurrentThread());
				RecreateRenderer();
			}
		} else {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
	CleanupEGL();
#else
	CoreWindow::GetForCurrentThread()->Activate();
	CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
#endif //FOREIGN_EGL
}

// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handler.
void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

// Window event handlers.
void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	mWindowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	mWindowClosed = true;
}

void App::OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	// back buffer size: dpi=>pix
	mPlayer->setVideoSurfaceSize(ConvertDipsToPixels(args->Size.Width), ConvertDipsToPixels(args->Size.Height));
}

static PropertySet^ surfaceCreationProperties = ref new PropertySet();
void App::InitializeEGL(CoreWindow^ window)
{
	OutputDebugStringW(GetEnvironmentStrings());
//	SetEnvironmentVariableA("GL_MAJOR", "3");
	// Create a PropertySet and initialize with the EGLNativeWindowType.
	surfaceCreationProperties->Insert(ref new String(EGLNativeWindowTypeProperty), window);
	// You can configure the surface to render at a lower resolution and be scaled up to
	// the full window size. This scaling is often free on mobile hardware.
	//
	// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
	// Size customRenderSurfaceSize = Size(800, 600);
	// surfaceCreationProperties->Insert(ref new String(EGLRenderSurfaceSizeProperty), PropertyValue::CreateSize(customRenderSurfaceSize));
	//
	// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
	// float customResolutionScale = 0.5f;
	// surfaceCreationProperties->Insert(ref new String(EGLRenderResolutionScaleProperty), PropertyValue::CreateSingle(customResolutionScale));
#ifdef FOREIGN_EGL
	{
		const EGLint configAttributes[] = {
			EGL_BUFFER_SIZE, 32, // TODO: from Context::config()
								  // EGL_OPENGL_ES2_BIT|EGL_OPENGL_BIT may result in EGL_BAD_ATTRIBUTE error on android. On linux the results of EGL_OPENGL_ES2_BIT may have both ES2 and OPENGL bit
								  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
								  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE
		};
		const EGLint contextAttributes[] = {
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
		};
		const EGLint surfaceAttributes[] = {
			// EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
			EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
			EGL_NONE
		};
		EGLConfig config = NULL;
		mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(mEglDisplay, NULL, NULL);
		EGLint numConfigs = 0;
		if ((eglChooseConfig(mEglDisplay, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
			throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
		mEglSurface = eglCreateWindowSurface(mEglDisplay, config, reinterpret_cast<IInspectable*>(surfaceCreationProperties), surfaceAttributes);
		if (mEglSurface == EGL_NO_SURFACE) {
			char es[64];
			snprintf(es, 64, "egl error: %#x\n", eglGetError());
			OutputDebugStringA(es);
			throw Exception::CreateException(E_FAIL, L"Failed to create EGL fullscreen surface");
		}
		mEglContext = eglCreateContext(mEglDisplay, config, EGL_NO_CONTEXT, contextAttributes);
		if (mEglContext == EGL_NO_CONTEXT)
			throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
		if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE)
			throw Exception::CreateException(E_FAIL, L"Failed to make fullscreen EGLSurface current");
	}
#else
	mPlayer->updateNativeWindow(reinterpret_cast<IInspectable*>(surfaceCreationProperties));// reinterpret_cast<IInspectable*>(window));
	//mPlayer->updateNativeWindow(reinterpret_cast<IInspectable*>(window));
#endif //FOREIGN_EGL
	mPlayer->setVideoSurfaceSize(ConvertDipsToPixels(window->Bounds.Width), ConvertDipsToPixels(window->Bounds.Height));
	mPlayer->setVideoDecoders({ "D3D11", "FFmpeg" });
	mPlayer->setMedia("rtmp://live.hkstv.hk.lxdns.com/live/hks");
	mPlayer->setState(PlaybackState::Playing);
}

void App::CleanupEGL()
{
	if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE)
		eglDestroySurface(mEglDisplay, mEglSurface);
	mEglSurface = EGL_NO_SURFACE;
	if (mEglDisplay != EGL_NO_DISPLAY && mEglContext != EGL_NO_CONTEXT)
		eglDestroyContext(mEglDisplay, mEglContext);
	mEglContext = EGL_NO_CONTEXT;
	if (mEglDisplay != EGL_NO_DISPLAY)
		eglTerminate(mEglDisplay);
	mEglDisplay = EGL_NO_DISPLAY;
}