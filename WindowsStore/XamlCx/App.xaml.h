#pragma once

#include "app.g.h"
#include "openglespage.xaml.h"

namespace XamlMDK
{
    ref class App sealed
    {
    public:
        App();
        virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

    private:
        OpenGLESPage^ mPage;
    };
}
