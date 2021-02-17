#include "pch.h"
#include "Program.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::System;
using namespace Windows::Security::Cryptography;

/// <summary>
/// This is our custom main method used to handle multi-instance activation.
/// This is achieved by including DISABLE_XAML_GENERATED_MAIN in the build properties,
/// which prevents the build system from generating the default Main method:
/// int __cdecl main(::Platform::Array<::Platform::String^>^ args)
///{
///    ::Windows::UI::Xaml::Application::Start(
///        ref new ::Windows::UI::Xaml::ApplicationInitializationCallback(
///        [](::Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
///        (void)p;
///        auto app = ref new ::App1::App();
///    }));
///}
/// </summary>
int __cdecl main(::Platform::Array<::Platform::String ^> ^ args)
{
    (void)args; // Unused parameter.

    //Check to see if we are on a version that supports the AppInstance APIs.
    if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.ApplicationModel.Activation.AppInstance"))
    {
        // In some scenarios, the platform might indicate a recommended instance.
        // If so, we can redirect this activation to that instance instead, if we wish.
        if (AppInstance::RecommendedInstance != nullptr)
        {
            AppInstance::RecommendedInstance->RedirectActivationTo();
        }
        else
        {
            // Try to open the mutex we use to signal that we're on a phone style device (or in tablet mode)
            HANDLE hInstance = OpenMutexW(SYNCHRONIZE, NULL, L"SingleInstanceCalculator");

            // Was the mutex created?
            if (hInstance != NULL)
            {
                // On non-Phone style device we will always launch a new instance
                // So we will go ahead and reuse the first one we find. :)

                // First release the mutex, we don't need it anymore
                ReleaseMutex(hInstance);

                // Double check to see that the instance is still around...
                if (AppInstance::GetInstances()->Size > 0)
                {
                    // Then go ahead and redirect to the first instance in the list
                    AppInstance::GetInstances()->GetAt(0)->RedirectActivationTo();
                }
                else
                {
                    // ("if(The thing that shouldn't happen actually happened)" ;) )
                    // The old instance exited before we got this far, so go ahead and createa a new instance after all.
                    AppInstance ^ instance = AppInstance::FindOrRegisterInstanceForKey(CryptographicBuffer::GenerateRandomNumber().ToString());

                    if (instance->IsCurrentInstance)
                    {
                        // If we successfully registered this instance, we can now just
                        // go ahead and do normal XAML initialization.
                        ::Windows::UI::Xaml::Application::Start(
                            ref new ::Windows::UI::Xaml::ApplicationInitializationCallback([](::Windows::UI::Xaml::ApplicationInitializationCallbackParams ^ p) {
                                (void)p; // Unused parameter.
                                auto app = ref new ::CalculatorApp::App();
                            }));
                    }
                }
            }
            else
            {
                // No mutex had been created by any other instance.
                // This either means we're the first, or that we're on a device where we should multi-instance.
                // So we go ahead and create a new instance either way.
                AppInstance ^ instance = AppInstance::FindOrRegisterInstanceForKey(CryptographicBuffer::GenerateRandomNumber().ToString());

                // If the instace we got for the random number isn't us, we'll do this again...
                while (!instance->IsCurrentInstance)
                {
                    // That random number was already in use. Find another.
                    instance = AppInstance::FindOrRegisterInstanceForKey(CryptographicBuffer::GenerateRandomNumber().ToString());
                }
                // If we successfully registered this instance, we can now just
                // go ahead and do normal XAML initialization.
                ::Windows::UI::Xaml::Application::Start(
                    ref new ::Windows::UI::Xaml::ApplicationInitializationCallback([](::Windows::UI::Xaml::ApplicationInitializationCallbackParams ^ p) {
                        (void)p; // Unused parameter.
                        auto app = ref new ::CalculatorApp::App();
                    }));
            }
        }
    }
    else
    {
        // This version of Windows does not support multi instancing,
        // just go ahead with the normal XAML initialization.
        ::Windows::UI::Xaml::Application::Start(
            ref new ::Windows::UI::Xaml::ApplicationInitializationCallback([](::Windows::UI::Xaml::ApplicationInitializationCallbackParams ^ p) {
                (void)p; // Unused parameter.
                auto app = ref new ::CalculatorApp::App();
            }));
    }
    return 0;
}
