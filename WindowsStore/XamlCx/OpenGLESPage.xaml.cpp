#include "pch.h"
#include "OpenGLESPage.xaml.h"
#include "mdk/Player.h"
#include <locale>
#include <codecvt>
#include <Inspectable.h>
#include <collection.h>
#include <ppltasks.h>
using namespace concurrency;
using namespace std;
using namespace MDK_NS;
using namespace XamlMDK;

using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;

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

	mPlayer->updateNativeSurface(reinterpret_cast<IInspectable*>(swapChainPanel_0));
    mPlayer->setVideoDecoders({ "MFT:d3d=11", "D3D11", "FFmpeg" });
    mPlayer->setMedia("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");
}

void OpenGLESPage::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
    if (args->Visible)
        mPlayer->setState(State::Playing); //
    else
        mPlayer->setState(State::Paused);
}

void OpenGLESPage::OnPanelSelected(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e)
{
	auto panel = safe_cast<Windows::UI::Xaml::Controls::SwapChainPanel^>(sender);
	mPlayer->updateNativeSurface(reinterpret_cast<IInspectable*>(panel));
}

void OpenGLESPage::OnModeSelected(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e)
{
	auto box = safe_cast<Windows::UI::Xaml::Controls::ComboBox^>(sender);
	const auto vis = box->SelectedIndex == 0 ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
	if (!swapChainPanel_1)
		return;
	swapChainPanel_1->Visibility = vis;
}

void OpenGLESPage::OnSelectFiles(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e)
{
  FileOpenPicker^ openPicker = ref new FileOpenPicker();
  openPicker->ViewMode = PickerViewMode::List;
  openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
  openPicker->FileTypeFilter->Append("*");
  create_task(openPicker->PickMultipleFilesAsync()).then([this](IVectorView<StorageFile^>^ files) {
    if (files->Size > 0) {
      std::string filename;
      for (auto file : files) { // TODO: play list
        auto filenamew = file->Path;
        wstring_convert<codecvt_utf8<wchar_t>> cvt;
        filename = cvt.to_bytes(filenamew->Data());
        mFile = file;
      }
      auto ptr = reinterpret_cast<IInspectable*>(mFile);
      OutputDebugStringA(filename.data());
      auto url = std::string("winrt:IStorageFile@").append(std::to_string((long long)ptr));
      mPlayer->setState(State::Stopped);
      mPlayer->waitFor(State::Stopped);
      mPlayer->setMedia(url.data());
      mPlayer->setState(PlaybackState::Playing);
    }
  });
}