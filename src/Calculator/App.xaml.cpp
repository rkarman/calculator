// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "App.xaml.h"
#include "CalcViewModel/Common/TraceLogger.h"
#include "CalcViewModel/Common/Automation/NarratorNotifier.h"
#include "CalcViewModel/Common/AppResourceProvider.h"
#include "CalcViewModel/Common/LocalizationSettings.h"
#include "Views/MainPage.xaml.h"

using namespace CalculatorApp;
using namespace CalculatorApp::Common;
using namespace CalculatorApp::Common::Automation;

using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::UI::StartScreen;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Activation;

namespace CalculatorApp
{
    namespace ApplicationResourceKeys
    {
        StringReference AppMinWindowHeight(L"AppMinWindowHeight");
        StringReference AppMinWindowWidth(L"AppMinWindowWidth");
    }
}

/// <summary>
/// Initializes the singleton application object. This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

    m_preLaunched = false;

    RegisterDependencyProperties();

    // TODO: MSFT 14645325: Set this directly from XAML.
    // Currently this is bugged so the property is only respected from code-behind.
    this->HighContrastAdjustment = ApplicationHighContrastAdjustment::None;

    this->Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);

#if _DEBUG
    this->DebugSettings->IsBindingTracingEnabled = true;
    this->DebugSettings->BindingFailed += ref new BindingFailedEventHandler([](_In_ Object ^ /*sender*/, _In_ BindingFailedEventArgs ^ e) {
        if (IsDebuggerPresent())
        {
            ::Platform::String ^ errorMessage = e->Message;
            __debugbreak();
        }
    });
#endif
}

void App::IsPhoneStyleDevice()
{
    // Check to see if we are on a Phone style device. Note, in Continuum mode the interaction mode is Keyboard
    // and we want desktop style behavior there, so we have to check for both user interaction mode Touch and
    // the existence of hardware buttons.
    if ((UIViewSettings::GetForCurrentView()->UserInteractionMode == UserInteractionMode::Touch)
        && (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.Phone.UI.Input.HardwareButtons")))
    {
        // If we're on a phone style device we create a mutex that other activations will
        // check for. As long as the mutex exists a new activation will redirect to this app instance
        // instead of creating a new one.
        m_singleInstanceMutex = CreateMutexW(NULL, true, L"SingleInstanceCalculator");
    }
}


#pragma optimize("", off) // Turn off optimizations to work around coroutine optimization bug
task<void> App::SetupJumpList()
{
    try
    {
        auto calculatorOptions = NavCategoryGroup::CreateCalculatorCategory();

        auto jumpList = co_await JumpList::LoadCurrentAsync();
        jumpList->SystemGroupKind = JumpListSystemGroupKind::None;
        jumpList->Items->Clear();

        for (NavCategory ^ option : calculatorOptions->Categories)
        {
            if (!option->IsEnabled)
            {
                continue;
            }
            ViewMode mode = option->Mode;
            auto item = JumpListItem::CreateWithArguments(((int)mode).ToString(), L"ms-resource:///Resources/" + NavCategory::GetNameResourceKey(mode));
            item->Description = L"ms-resource:///Resources/" + NavCategory::GetNameResourceKey(mode);
            item->Logo = ref new Uri("ms-appx:///Assets/" + mode.ToString() + ".png");

            jumpList->Items->Append(item);
        }

        co_await jumpList->SaveAsync();
    }
    catch (...)
    {
    }
};
#pragma optimize("", on)

Frame ^ App::CreateFrame()
{
    auto frame = ref new Frame();
    frame->FlowDirection = LocalizationService::GetInstance()->GetFlowDirection();

    return frame;
}

/// <summary>
/// Invoked when the application is launched normally by the end user. Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs ^ args)
{
    if (args->PrelaunchActivated)
    {
        // If the app got pre-launch activated, then save that state in a flag
        m_preLaunched = true;
    }
    NavCategory::InitializeCategoryManifest(args->User);
    OnAppLaunch(args, args->Arguments);
}

void App::OnAppLaunch(IActivatedEventArgs ^ args, String ^ argument)
{
    // Uncomment the following lines to display frame-rate and per-frame CPU usage info.
    //#if _DEBUG
    //    if (IsDebuggerPresent())
    //    {
    //        DebugSettings->EnableFrameRateCounter = true;
    //    }
    //#endif

    args->SplashScreen->Dismissed += ref new TypedEventHandler<SplashScreen ^, Object ^>(this, &App::DismissedEventHandler);

    auto rootFrame = dynamic_cast<Frame ^>(Window::Current->Content);
    WeakReference weak(this);

    float minWindowWidth = static_cast<float>(static_cast<double>(this->Resources->Lookup(ApplicationResourceKeys::AppMinWindowWidth)));
    float minWindowHeight = static_cast<float>(static_cast<double>(this->Resources->Lookup(ApplicationResourceKeys::AppMinWindowHeight)));
    Size minWindowSize = SizeHelper::FromDimensions(minWindowWidth, minWindowHeight);

    ApplicationView ^ appView = ApplicationView::GetForCurrentView();
    ApplicationDataContainer ^ localSettings = ApplicationData::Current->LocalSettings;
    // Set the size of the calc as size of the default standard mode
    appView->SetPreferredMinSize(minWindowSize);
    appView->TryResizeView(minWindowSize);

    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if (rootFrame == nullptr)
    {
        // Create a Frame to act as the navigation context
        rootFrame = App::CreateFrame();

        // When the navigation stack isn't restored navigate to the first page,
        // configuring the new page by passing required information as a navigation
        // parameter
        if (!rootFrame->Navigate(MainPage::typeid, argument))
        {
            // We couldn't navigate to the main page, kill the app so we have a good
            // stack to debug
            throw std::bad_exception();
        }

        SetMinWindowSizeAndActivate(rootFrame, minWindowSize);
        IsPhoneStyleDevice();
    }
    else
    {
        if (rootFrame->Content == nullptr)
        {
            // When the navigation stack isn't restored navigate to the first page,
            // configuring the new page by passing required information as a navigation
            // parameter
            if (!rootFrame->Navigate(MainPage::typeid, argument))
            {
                // We couldn't navigate to the main page,
                // kill the app so we have a good stack to debug
                throw std::bad_exception();
            }
        }
        if (ApplicationView::GetForCurrentView()->ViewMode != ApplicationViewMode::CompactOverlay)
        {
            // Ensure the current window is active
            Window::Current->Activate();
        }
    }

}

void App::SetMinWindowSizeAndActivate(Frame ^ rootFrame, Size minWindowSize)
{
    // SetPreferredMinSize should always be called before Window::Activate
    ApplicationView ^ appView = ApplicationView::GetForCurrentView();
    appView->SetPreferredMinSize(minWindowSize);

    // Place the frame in the current Window
    Window::Current->Content = rootFrame;
    Window::Current->Activate();
}

void App::RegisterDependencyProperties()
{
    NarratorNotifier::RegisterDependencyProperties();
}

void App::OnActivated(IActivatedEventArgs ^ args)
{
    if (args->Kind == ActivationKind::Protocol)
    {
        // We currently don't pass the uri as an argument,
        // and handle any protocol launch as a normal app launch.
        OnAppLaunch(args, nullptr);
    }
}

void CalculatorApp::App::OnSuspending(Object ^ sender, SuspendingEventArgs ^ args)
{
    TraceLogger::GetInstance()->LogButtonUsage();
}

void App::DismissedEventHandler(SplashScreen ^ sender, Object ^ e)
{
    SetupJumpList();
}



