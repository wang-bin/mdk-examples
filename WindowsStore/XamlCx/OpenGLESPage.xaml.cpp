#include "pch.h"
#include "OpenGLESPage.xaml.h"
#include "mdk/Player.h"
#include <Inspectable.h>

using namespace std;
using namespace MDK_NS;
using namespace XamlMDK;

static struct init {
	init() {
		setLogHandler([](LogLevel, const char* msg) {
			OutputDebugStringA(msg);
		});
	}
} ___;

OpenGLESPage::OpenGLESPage() :
    mPlayer(make_unique<Player>())
{
    InitializeComponent();

	Windows::UI::Core::CoreWindow^ window = Windows::UI::Xaml::Window::Current->CoreWindow;
    window->VisibilityChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^>(this, &OpenGLESPage::OnVisibilityChanged);
    this->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &OpenGLESPage::OnPageLoaded);
}

OpenGLESPage::~OpenGLESPage()
{
	mPlayer->setState(State::Stopped);
}

void OpenGLESPage::OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    // The SwapChainPanel has been created and arranged in the page layout, so EGL can be initialized.
	mPlayer->updateNativeWindow(reinterpret_cast<IInspectable*>(swapChainPanel));// reinterpret_cast<IInspectable*>(window));
	//mPlayer->setVideoSurfaceSize(swapChainPanel->Width, swapChainPanel->Height); // FIXME: why invalid size?
	mPlayer->setVideoDecoders({ "D3D11", "FFmpeg" });
	mPlayer->setMedia("rtmp://live.hkstv.hk.lxdns.com/live/hks");
	mPlayer->setState(PlaybackState::Playing);
}

void OpenGLESPage::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
    if (args->Visible)
        mPlayer->setState(State::Playing); //
    else
        mPlayer->setState(State::Paused);
}