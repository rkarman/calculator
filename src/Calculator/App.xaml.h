// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"

namespace CalculatorApp
{
    namespace ApplicationResourceKeys
    {
        extern Platform::StringReference AppMinWindowHeight;
        extern Platform::StringReference AppMinWindowWidth;
    }

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    ref class App sealed
    {
    public:
        App();
        virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args) override;
        virtual void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args) override;

    internal:

    private:
        static Windows::UI::Xaml::Controls::Frame ^ CreateFrame();
        static void SetMinWindowSizeAndActivate(Windows::UI::Xaml::Controls::Frame ^ rootFrame, Windows::Foundation::Size minWindowSize);

        void OnAppLaunch(Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args, Platform::String ^ argument);
        void DismissedEventHandler(Windows::ApplicationModel::Activation::SplashScreen ^ sender, Platform::Object ^ e);
        void RegisterDependencyProperties();
        void OnSuspending(Platform::Object ^ sender, Windows::ApplicationModel::SuspendingEventArgs ^ args);
        void IsPhoneStyleDevice();


    private:
        concurrency::task<void> SetupJumpList();
        bool m_preLaunched;
        HANDLE m_singleInstanceMutex;

        Windows::UI::Xaml::Controls::Primitives::Popup ^ m_aboutPopup;
    };
}
