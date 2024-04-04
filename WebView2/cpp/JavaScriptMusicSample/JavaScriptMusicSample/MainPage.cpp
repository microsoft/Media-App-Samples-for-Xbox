// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "winrt/NativeMediaPlayer.h"
#include "winrt/WinRTAdapter.h"
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.Xaml.Media.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::Web::WebView2::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Navigation;

namespace winrt::JavaScriptMusicSample::implementation
{
	MainPage::MainPage()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        Loaded({ this, &MainPage::OnLoaded });

        // Never reuse the cached page because the model is designed to be unloaded and disposed
        NavigationCacheMode(NavigationCacheMode::Disabled);

        // By default, Xbox gives you a border around your content to help you keep it inside a "TV-safe"
        // area. This helps protect you from drawing too close to the edges of the screen where content may
        // not be visible due to physical variations in televisions.
        //
        // This line disables that behavior. If you want, you can restore the automatic TV-safe area by
        // commenting this line out. Otherwise, be careful not to draw vital content too close to the edge
        // of the screen. (A buffer of approximately 80 pixels is ideal).
        ApplicationView::GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);

        // By default, XAML apps are scaled up 2x on Xbox. This line disables that behavior, allowing the
        // app to use the actual resolution of the device (1920 x 1080 pixels).
        if (!ApplicationViewScaling::TrySetDisableLayoutScaling(true))
        {
            OutputDebugString(L"Error: Failed to disable layout scaling.");
        }

        // Handle page unload events
        Unloaded({ this, &MainPage::OnUnloaded });
    }

    /// <summary>
    /// Called when the XAML page has finished loading.
    /// We wait to initialize the WebView until this point because it can take a little time to load.
    /// </summary>
    /// <param name="e">Details about the load operation.</param>
    void MainPage::OnLoaded(IInspectable const&, [[maybe_unused]] RoutedEventArgs const& e)
    {
        // Create the WebView and point it at the initial URL to begin loading the app's UI.
        // The WebView is not added to the XAML page until the NavigationCompleted event fires.
        webView = WebView2();
        InitializeWebView();
    }

    /// <summary>
    /// Sets up the web view, hooking up all of the event handlers that the app cares about.
    /// </summary>
    fire_and_forget MainPage::InitializeWebView()
    {
        // The WebView XAML background color can sometimes show while a page is loading. Set it to
        // something that matches the app's color scheme so it does not produce a jarring flash.
        webView.Background(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 16, 16, 16)));

        co_await webView.EnsureCoreWebView2Async();
        auto coreWV2{ webView.CoreWebView2() };
        if (coreWV2)
        {
            // Change some settings on the WebView. Check the documentation for more options:
            // https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2settings
            auto settings = coreWV2.Settings();
            settings.AreDefaultContextMenusEnabled(false);
            settings.IsGeneralAutofillEnabled(false);
            settings.IsPasswordAutosaveEnabled(false);
            settings.IsStatusBarEnabled(false);
            settings.HiddenPdfToolbarItems(CoreWebView2PdfToolbarItems::None);

            // This prevents the user from opening the DevTools with F12, it does not prevent you from
            // attaching the Edge Dev Tools yourself.
            settings.AreDevToolsEnabled(false);

            // This creates a virtual URL which can be used to navigate the WebView to a local folder
            // embedded within the app package. If your application uses entirely pages hosted on the
            // web, you should remove this line.
            coreWV2.SetVirtualHostNameToFolderMapping(L"local.webcode", L"WebCode", CoreWebView2HostResourceAccessKind::Allow);

            // Inject a static instance of the MediaPlaybackController into the WebView so that it can
            // be accessed through JavaScript. Because it is static, it will continue to survive even
            // when this UI is destroyed, and it will not get re-created when the UI is reconstructed.
            static NativeMediaPlayer::MediaPlaybackController mediaPlaybackController{};
            WinRTAdapter::DispatchAdapter dispatchAdapter{ };
            coreWV2.AddHostObjectToScript(L"mediaPlaybackControllerInstance", dispatchAdapter.WrapObject(mediaPlaybackController, dispatchAdapter));

            // Set some JavaScript code to run for each page. This sets some properties about how
            // the projected APIs will behave and creates a "mediaPlaybackController" object at the
            // top level to make it easier to access the projected MediaPlaybackController instance.
            co_await coreWV2.AddScriptToExecuteOnDocumentCreatedAsync(
                L"(() => {"
                L"if (chrome && chrome.webview) {"
                L"console.log('Setting up WinRT projection options');"
                L"chrome.webview.hostObjects.options.defaultSyncProxy = true;"
                L"chrome.webview.hostObjects.options.forceAsyncMethodMatches = [/Async$/,/AsyncWithSpeller$/];"
                L"chrome.webview.hostObjects.options.ignoreMemberNotFoundError = true;"
                L"mediaPlaybackController = chrome.webview.hostObjects.sync.mediaPlaybackControllerInstance;"
                L"}"
                L"})();");

            // Hook up the event handlers before setting the source so none of these events get missed.
            navigationCompletedEventToken = webView.NavigationCompleted({ this, &MainPage::OnNavigationCompleted });

            // This will cause the WebView to navigate to our initial page
            webView.Source(Uri{ initialUri });
        }
        else
        {
            // TODO: Show an error state
            OutputDebugString(L"Unable to retrieve CoreWebView2");
        }
    }

    /// <summary>
    /// Called when the page is no longer connected to the main object tree.
    /// In this sample (which only has one page to begin with) this generally happens when the app is
    /// entering the background and is dropping its UI to reduce its memory footprint.
    /// </summary>
    void MainPage::OnUnloaded(IInspectable const&, RoutedEventArgs const&)
    {
        // Drop references to the WebView so that it can be destructed
        // This allows our app to reduce its memory footprint
        webView.NavigationCompleted(navigationCompletedEventToken);
        webView.Close();
        Content(nullptr);
        webView = nullptr;
    }

    /// <summary>
    /// Called whenever a new page is fully loaded (or fails to load) in the WebView.
    /// </summary>
    /// <param name="args">Details about the page which was loaded.</param>
    void MainPage::OnNavigationCompleted(WebView2 const&, CoreWebView2NavigationCompletedEventArgs const& args)
    {
        // If we haven't done so yet, add the WebView to the page, making it visible to the user.
        // We create it dynamically and wait for the first navigation to complete before doing so.
        // This ensures that focus gets set on the WebView only after it is ready to receive it.
        // Failing to do this can occasionally result in Gamepad input failing to route to your
        // JavaScript code.
        if (webView.Parent() == nullptr)
        {
            if (args.IsSuccess())
            {
                // Replace the current contents of this page with the WebView
                Content(webView);
                webView.Focus(FocusState::Programmatic);
            }
            else
            {
                // WebView navigation failed.
                // TODO: Show an error state
                OutputDebugString(L"Initial WebView navigation failed with error status: " + static_cast<int>(args.WebErrorStatus()));
            }
        }
    }
}
