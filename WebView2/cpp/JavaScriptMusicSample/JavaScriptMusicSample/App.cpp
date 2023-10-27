// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "App.h"
#include "MainPage.h"
#include <sstream>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Notifications.h>
using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::UI::Notifications;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::System;
using namespace JavaScriptMusicSample;
using namespace JavaScriptMusicSample::implementation;

/// <summary>
/// Creates the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    Suspending({ this, &App::OnSuspending });
    Resuming({ this, &App::OnResuming });

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif

    // Subscribe to key lifecyle events to know when the app transitions to and from
    // foreground and background. Leaving the background is an important transition
    // because the app may need to restore UI.
    EnteredBackground({ this, &App::OnEnteredBackground });
    LeavingBackground({ this, &App::OnLeavingBackground });

    // On Xbox, this turns off the virtual cursor so your app can be driven by the gamepad
    RequiresPointerMode(ApplicationRequiresPointerMode::WhenRequested);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs const& e)
{
    CreateRootFrame(e.PreviousExecutionState(), e.Arguments());

    if (e.PrelaunchActivated() == false)
    {
        // Ensure the current window is active
        Window::Current().Activate();
    }
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(IInspectable const&, SuspendingEventArgs const& e)
{
    auto deferral{ e.SuspendingOperation().GetDeferral()};
    //TODO: Save application state and stop any background activity
    ShowToast(L"Suspending");
    deferral.Complete();
}

/// <summary>
/// Invoked when application execution is resumed.
/// </summary>
void App::OnResuming(IInspectable const&, IInspectable const&)
{
    ShowToast(L"Resuming");
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(IInspectable const&, NavigationFailedEventArgs const& e)
{
    throw hresult_error(E_FAIL, hstring(L"Failed to load Page ") + e.SourcePageType().Name);
}

/// <summary>
/// Invoked when application begins running in the background. There are stricter
/// limitations on memory usage in the background versus the foreground, so the
/// application should take this opportunity to drop any memory it does not need
/// (such as memory being used for user interfaces).
/// </summary>
/// <param name="e">Details on entering the background.</param>
void App::OnEnteredBackground(IInspectable const&, [[maybe_unused]] EnteredBackgroundEventArgs const& e)
{
    ShowToast(L"Entering background");

    // If the app has caches or other memory it can free, now is the time.
    // << App can release memory here >>

    // Additionally, if the application is currently
    // in background mode and still has a view with content
    // then the view can be released to save memory and 
    // can be recreated again later when leaving the background.
    if (Window::Current().Content())
    {
        ShowToast(L"Unloading view");

        // Clear the view content. Note that views should rely on
        // events like Page.Unloaded to further release resources. Be careful
        // to also release event handlers in views since references can
        // prevent objects from being collected. C++ developers should take
        // special care to use weak references for event handlers where appropriate.
        rootFrame = nullptr;
        Window::Current().Content(nullptr);
    }

    ShowToast(L"Finished reducing memory usage");
}

/// <summary>
/// Invoked when application leaves the background and enters the foreground.
/// The application may need to recreate its user interface when this happens.
/// </summary>
/// <param name="e">Details on leaving the background.</param>
void App::OnLeavingBackground(IInspectable const&, [[maybe_unused]] LeavingBackgroundEventArgs const& e)
{
    ShowToast(L"Leaving background");
    // Restore view content if it was previously unloaded.
    if (!Window::Current().Content())
    {
        ShowToast(L"Loading view");
        CreateRootFrame(ApplicationExecutionState::Running, L"");
    }
}

/// <summary>
/// Creates the frame containing the view
/// </summary>
/// <remarks>
/// This is the same code that was in OnLaunched() initially.
/// It is moved to a separate method so the view can be restored
/// when leaving the background if it was unloaded when the app 
/// entered the background.
/// </remarks>
void App::CreateRootFrame(ApplicationExecutionState const& previousExecutionState, hstring const& arguments)
{
    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if (!rootFrame)
    {
        // Create a Frame to act as the navigation context and associate it with
        // a SuspensionManager key
        rootFrame = Frame();

        rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });

        if (previousExecutionState == ApplicationExecutionState::Terminated)
        {
            // Restore the saved session state only when appropriate, scheduling the
            // final launch steps after the restore is complete
        }

        // Place the frame in the current Window
        Window::Current().Content(rootFrame);
    }

    if (!rootFrame.Content())
    {
        // When the navigation stack isn't restored navigate to the first page,
        // configuring the new page by passing required information as a navigation
        // parameter
        rootFrame.Navigate(xaml_typename<JavaScriptMusicSample::MainPage>(), box_value(arguments));
    }
}

/// <summary>
/// Gets a string describing current memory usage
/// </summary>
/// <returns>String describing current memory usage</returns>
hstring App::GetMemoryUsageText()
{
    std::wostringstream strStream{};
    strStream
        << L"[Memory: "
        << L"Level = " << static_cast<int>(MemoryManager::AppMemoryUsageLevel()) << L", "
        << L"Usage=" << (MemoryManager::AppMemoryUsage() / 1024) << L"K, "
        << L"Target=" << (MemoryManager::AppMemoryUsageLimit() / 1024) << L"K"
        << L"]" << std::endl;
    return strStream.str().c_str();
}

/// <summary>
/// Sends a toast notification
/// </summary>
/// <param name="msg">Message to send</param>
/// <param name="subMsg">Sub message</param>
void App::ShowToast(hstring const& msg, hstring const& subMsg)
{
    std::wstring secondLine{ !subMsg.empty() ? subMsg : GetMemoryUsageText() };
    std::wostringstream strStream{};
    strStream << msg.c_str() << std::endl << secondLine;

    OutputDebugString(strStream.str().c_str());

    if (!showToasts)
        return;

    auto toastXml{ ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02) };

    auto toastTextElements{ toastXml.GetElementsByTagName(L"text") };
    toastTextElements.Item(0).AppendChild(toastXml.CreateTextNode(msg));
    toastTextElements.Item(1).AppendChild(toastXml.CreateTextNode(secondLine));

    auto toast{ ToastNotification(toastXml) };
    ToastNotificationManager::CreateToastNotifier().Show(toast);
}