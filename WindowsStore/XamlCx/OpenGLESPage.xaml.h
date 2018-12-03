#pragma once
#include "OpenGLESPage.g.h"
#include "mdk/Player.h"

namespace XamlMDK
{
    public ref class OpenGLESPage sealed
    {
    public:
        OpenGLESPage();
        virtual ~OpenGLESPage();
    private:
		void OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnPanelSelected(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e);
        void OnModeSelected(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e);
        void OnSelectFiles(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e);
        std::unique_ptr<MDK_NS::Player> mPlayer;
        Windows::Storage::StorageFile^ mFile;
	};
}
