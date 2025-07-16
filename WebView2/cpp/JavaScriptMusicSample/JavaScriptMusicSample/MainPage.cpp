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
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <sstream>

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::Web::WebView2::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt;

namespace winrt::JavaScriptMusicSample::implementation
{
	MainPage::MainPage()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

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

        // Handle page unload events
        Unloaded({ this, &MainPage::OnUnloaded });

        InitializeWebView();
    }

    /// <summary>
    /// Sets up the web view, hooking up all of the event handlers that the app cares about.
    /// </summary>
    fire_and_forget MainPage::InitializeWebView()
    {
        webView = WebView2();

        // The WebView XAML background color can sometimes show while a page is loading. Set it to
        // something that matches the app's color scheme so it does not produce a jarring flash.
        webView.Background(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 16, 16, 16)));

        co_await webView.EnsureCoreWebView2Async();

        // Add the WebView to the page, making it visible to the user. This must be done after
        // EnsureCoreWebView2Async() has completed, because before that it is not capable of
        // receiving focus.
        Content(webView);
        webView.Focus(FocusState::Programmatic);

        if (auto coreWV2{ webView.CoreWebView2() })
        {
            // Change some settings on the WebView. Check the documentation for more options:
            // https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2settings
            auto settings = coreWV2.Settings();
            settings.AreDefaultContextMenusEnabled(false);
            settings.IsGeneralAutofillEnabled(false);
            settings.IsPasswordAutosaveEnabled(false);
            settings.IsStatusBarEnabled(false);

            // This turns off SmartScreen, which can have some performance impact. This is ONLY safe
            // to do if you are certain that your app will only ever visit trusted pages that you
            // control. If there is a chance your app could end up browsing the open web, you should
            // delete this line.
            settings.IsReputationCheckingRequired(false);

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
            coreWV2.ProcessFailed({ this, &MainPage::OnWebViewProcessFailed });
			coreWV2.LaunchingExternalUriScheme({ this, &MainPage::OnLaunchingExternalUriScheme });

            // This will cause the WebView to navigate to our initial page
            webView.Source(Uri{ initialUri });
        }
        else
        {
            // TODO: Show an error state
            OutputDebugString(L"Unable to retrieve CoreWebView2\n");
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
        if (webView != nullptr)
        {
            webView.NavigationCompleted(navigationCompletedEventToken);
            webView.Close();
        }
        Content(nullptr);
        webView = nullptr;
    }

    /// <summary>
    /// Called whenever a new page is fully loaded (or fails to load) in the WebView.
    /// </summary>
    /// <param name="args">Details about the page which was loaded.</param>
    void MainPage::OnNavigationCompleted(WebView2 const&, CoreWebView2NavigationCompletedEventArgs const& args)
    {
        if (!args.IsSuccess())
        {
            // WebView navigation failed.
            // TODO: Show an error state
            std::wostringstream strStream{};
            strStream << L"Initial WebView navigation failed with error status: " << static_cast<int>(args.WebErrorStatus()) << std::endl;
            OutputDebugString(strStream.str().c_str());
        }
    }

    /// <summary>
    /// Called whenever the WebView attempts to launch another app through a URI scheme.
    /// The confirmation dialog cannot be navigated by the Xbox controller, so we reroute it to
    /// use the native API instead.
    /// </summary>
    /// <param name="args">The details of the protocol launch.</param>
    fire_and_forget MainPage::OnLaunchingExternalUriScheme(CoreWebView2 const&, CoreWebView2LaunchingExternalUriSchemeEventArgs const& args)
    {
		// Cancel the default behavior of the WebView because we do not want it to show a dialog
        args.Cancel(true);

        // Launch the URI using the native API instead
        co_await Launcher::LaunchUriAsync(Uri(args.Uri()));
    }

    /// <summary>
    /// Called when one of the WebView processes fails. This implementation merely prints out the
    /// details of the process failure to the console. If you have a telemetry system, you could
    /// also capture this information for further analysis.
    /// </summary>
    /// <param name="args">Details about the failure.</param>
    void MainPage::OnWebViewProcessFailed(CoreWebView2 const&, CoreWebView2ProcessFailedEventArgs const& args)
    {
        std::wostringstream strStream{};
        strStream << L"WebView Process failed:\n";
        strStream << L"* Exit Code: " << args.ExitCode() << std::endl;
        strStream << L"* Process Description: " << args.ProcessDescription().c_str() << std::endl;

        // Convert the process failed kind to a string
        strStream << L"* Process Failed Kind: ";
        CoreWebView2ProcessFailedKind kind = args.ProcessFailedKind();
        switch (kind)
        {
        case CoreWebView2ProcessFailedKind::BrowserProcessExited:
            strStream << L"Browser Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::RenderProcessExited:
            strStream << L"Render Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::RenderProcessUnresponsive:
            strStream << L"Render Process Unresponsive";
            break;
        case CoreWebView2ProcessFailedKind::FrameRenderProcessExited:
            strStream << L"Frame Render Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::UtilityProcessExited:
            strStream << L"Utility Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::SandboxHelperProcessExited:
            strStream << L"Sandbox Helper Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::GpuProcessExited:
            strStream << L"GPU Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::PpapiPluginProcessExited:
            strStream << L"PPAPI Plugin Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::PpapiBrokerProcessExited:
            strStream << L"PPAPI Broker Process Exited";
            break;
        case CoreWebView2ProcessFailedKind::UnknownProcessExited:
            strStream << L"Unknown Process Exited";
            break;
        default:
            strStream << L"Unknown Process Failed Kind: " << static_cast<int>(kind);
            break;
        }
        strStream << std::endl;

        // Convert the failure reason to a string
        strStream << L"Process Failed Reason: ";
        CoreWebView2ProcessFailedReason reason = args.Reason();
        switch (reason)
        {
        case CoreWebView2ProcessFailedReason::Unexpected:
            strStream << L"Unexpected";
            break;
        case CoreWebView2ProcessFailedReason::Unresponsive:
            strStream << L"Unresponsive";
            break;
        case CoreWebView2ProcessFailedReason::Terminated:
            strStream << L"Terminated";
            break;
        case CoreWebView2ProcessFailedReason::Crashed:
            strStream << L"Crashed";
            break;
        case CoreWebView2ProcessFailedReason::LaunchFailed:
            strStream << L"Launch Failed";
            break;
        case CoreWebView2ProcessFailedReason::OutOfMemory:
            strStream << L"Out of Memory";
            break;
        case CoreWebView2ProcessFailedReason::ProfileDeleted:
            strStream << L"Profile Deleted"; // This reason is deprecated
            break;
        default:
            strStream << L"Unknown reason: " << static_cast<int>(reason);
            break;
        }
        strStream << std::endl;

        // Note that there is additional frame information that can be found under
        // FrameInfosForFailedProcess(), if relevant for your use case.

        OutputDebugString(strStream.str().c_str());
    }
}
