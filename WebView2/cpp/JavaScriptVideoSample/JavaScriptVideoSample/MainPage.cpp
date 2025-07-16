// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "winrt/WinRTAdapter.h"
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.System.Profile.h>
#include <sstream>

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::Web::WebView2::Core;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::System;
using namespace winrt::Windows::System::Profile;
using namespace winrt;

namespace winrt::JavaScriptVideoSample::implementation
{
    MainPage::MainPage()
    {
        // By default, Xbox gives you a border around your content to help you keep it inside a "TV-safe"
        // area. This helps protect you from drawing too close to the edges of the screen where content may
        // not be visible due to physical variations in televisions.
        //
        // This line disables that behavior. If you want, you can restore the automatic TV-safe area by
        // commenting this line out. Otherwise, be careful not to draw vital content too close to the edge
        // of the screen. Details can be found here:
        // https://docs.microsoft.com/en-us/windows/apps/design/devices/designing-for-tv#tv-safe-area
        ApplicationView::GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);

        // Set up the SystemMediaTransportControls. At least IsPlayEnabled and IsPauseEnabled must be set
        // to true for the system to know how to properly tailor the user experience.
        smtc = SystemMediaTransportControls::GetForCurrentView();
        smtc.IsPlayEnabled(true);
        smtc.IsPauseEnabled(true);
        smtc.IsStopEnabled(true);
        smtc.IsNextEnabled(true);
        smtc.IsPreviousEnabled(true);
        smtc.ButtonPressed({ this, &MainPage::OnSMTCButtonPressed });

        // Hook up an event so that we can inform the JavaScript code when the HDMI display mode chages.
        auto hdmiInfo{ Windows::Graphics::Display::Core::HdmiDisplayInformation::GetForCurrentView() };
        if (hdmiInfo)
        {
            hdmiInfo.DisplayModesChanged({ this, &MainPage::OnDisplayModeChanged });
        }

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

            // Inject some Windows APIs into the WebView so that they can be called from JavaScript.
            // The WinRTAdapter project is responsible for converting classes into a format that
            // can be projected into JavaScript. In this sample, it is set up to adapt these APIs:
            // Windows.Media.Protection.ProtectionCapabilities
            // Windows.Media.Protection.ProtectionCapabilityResult
            // Windows.Graphics.Display.Core
            //
            // Additionally, it adapts all classes in the WindowsAPIProxies namespace, found in this
            // solution. To add additional namespaces, right-click on the
            // WinRTAdapter project > Properties > Common Properties > WebView2, and edit the
            // "Include Filters" property. For additional information, see:
            // https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/winrt-from-js
            auto dispatchAdapter{ WinRTAdapter::DispatchAdapter() };

            // This line adds the official APIs directly.
            coreWV2.AddHostObjectToScript(L"Windows", dispatchAdapter.WrapNamedObject(L"Windows", dispatchAdapter));
            // This line adds custom proxy objects for APIs which would crash if called directly
            // from JavaScript. See the WindowsAPIProxies project in this solution for details.
            coreWV2.AddHostObjectToScript(L"WindowsAPIProxies", dispatchAdapter.WrapNamedObject(L"WindowsAPIProxies", dispatchAdapter));

            // Set some JavaScript code to run for each page. This sets some properties about how
            // the projected APIs will behave and creates "Windows" and "WindowsProxies" objects
            // at the top level to make it easier to access these APIs.
            co_await coreWV2.AddScriptToExecuteOnDocumentCreatedAsync(
            L"(() => {"
                L"if (chrome && chrome.webview) {"
                    L"console.log('Setting up WinRT projection options');"
                    L"chrome.webview.hostObjects.options.defaultSyncProxy = true;"
                    L"chrome.webview.hostObjects.options.forceAsyncMethodMatches = [/Async$/,/AsyncWithSpeller$/];"
                    L"chrome.webview.hostObjects.options.ignoreMemberNotFoundError = true;"
                    L"window.Windows = chrome.webview.hostObjects.sync.Windows;"
                    L"window.WindowsProxies = chrome.webview.hostObjects.sync.WindowsAPIProxies;"
                L"}"
            L"})();");

            // Hook up the event handlers before setting the source so none of these events get missed.
            webView.WebMessageReceived({ this, &MainPage::OnWebMessageReceived });
            webView.NavigationStarting({ this, &MainPage::OnNavigationStarting });
            webView.NavigationCompleted({ this, &MainPage::OnNavigationCompleted });
            coreWV2.ProcessFailed({ this, &MainPage::OnWebViewProcessFailed });
            coreWV2.LaunchingExternalUriScheme({ this, &MainPage::OnLaunchingExternalUriScheme });

            // Pass the device type as a query parameter. This data could be passed in other ways as
            // well, such as by setting the UserAgent string in the WebView's CoreWebView2Settings,
            // or as part of the call to AddScriptToExecuteOnDocumentCreatedAsync.
            // Possible values for the DeviceForm include:
            //   "Xbox One"
            //   "Xbox One S"
            //   "Xbox One X"
            //   "Xbox Series S"
            //   "Xbox Series X"
            hstring deviceType = AnalyticsInfo::DeviceForm();
            hstring queryString = L"?deviceType=" + deviceType;

            // This will cause the WebView to navigate to our initial page.
            webView.Source(Uri{ initialUri + queryString });
        }
        else
        {
            // TODO: Show an error state
            OutputDebugString(L"Unable to retrieve CoreWebView2\n");
        }
    }

    /// <summary>
    /// Called whenever the WebView begins navigating to a new page.
    /// </summary>
    void MainPage::OnNavigationStarting(WebView2 const&, CoreWebView2NavigationStartingEventArgs const&)
    {
        isNavigatedToPage = false;
    }

    /// <summary>
    /// Called whenever a new page is fully loaded (or fails to load) in the WebView.
    /// </summary>
    /// <param name="args">Details about the page which was loaded.</param>
    void MainPage::OnNavigationCompleted(WebView2 const&, CoreWebView2NavigationCompletedEventArgs const& args)
    {
        // Track whether or not the WebView is successfully navigated to a page.
        if (args.IsSuccess())
        {
            isNavigatedToPage = true;
        }
        else
        {
            // WebView navigation failed.
            // TODO: Show an error state
            std::wostringstream strStream{};
            strStream << L"Initial WebView navigation failed with error status: " << static_cast<int>(args.WebErrorStatus()) << std::endl;
            OutputDebugString(strStream.str().c_str());
        }
    }
    
    /// <summary>
    /// Recieves any data that your web app passed to window.chrome.webview.postMessage().
    /// 
    /// The JavaScript code in this sample sends JSON data through
    /// this mechanism to inform the C# code when important events occur.
    /// </summary>
    /// <param name="args">An object containing the URL of the calling page and the data that was
    /// passed to window.chrome.webview.postMessage()</param>
    void MainPage::OnWebMessageReceived(WebView2 const&, CoreWebView2WebMessageReceivedEventArgs const& args)
    {
        // If the message contains valid JSON data, handle it as an event notification.
        // Otherwise, simply log the message as-is to the debug console.
        hstring jsonMessage{ args.TryGetWebMessageAsString() };
        JsonObject argsJson{};
        if (JsonObject::TryParse(jsonMessage, argsJson))
        {
            HandleJsonNotification(argsJson);
        }
        else
        {
            OutputDebugString(L"Could not parse JSON message recieved:");
            OutputDebugString(jsonMessage.c_str());
        }
    }

    /// <summary>
    /// Called whenever the user presses a button on a physical media remote.
    /// </summary>
    /// <param name="args">Details about which button was pressed.</param>
    void MainPage::OnSMTCButtonPressed(SystemMediaTransportControls const&, SystemMediaTransportControlsButtonPressedEventArgs const& args)
    {
        HandleSMTCButtonPressed(args.Button());
    }

    /// <summary>
    /// Called when the HDMI display mode changes. This can happen when the HDMI cable is unplugged or
    /// replugged, or when the mode changes for other reasons.
    /// </summary>
    void MainPage::OnDisplayModeChanged(HdmiDisplayInformation const&, IInspectable const&)
    {
        UpdateDisplayMode();
    }

    /// <summary>
    /// A helper function to handle JSON messages sent from the JavaScript code.
    /// </summary>
    /// <param name="json">The JSON object that was sent.</param>
    void MainPage::HandleJsonNotification(JsonObject const& json)
    {
        // The JavaScript code is capable of sending any arbitrary JSON message that the native code is
        // prepared to handle. For the purposes of this sample we expect it to have the form:
        //
        // {
        //     "Message": "<a well-known string that tells the native code what event occurred>",
        //     "Args": {
        //         "<argument name 1>": <argument value>,
        //         ...
        //     }
        // }

        hstring message{ json.GetNamedString(L"Message") };
        JsonObject args{ json.GetNamedObject(L"Args") };

        if (message == L"PlaybackStarted")
        {
            // Inform the system that playback has started
            smtc.PlaybackStatus(MediaPlaybackStatus::Playing);
        }
        else if (message == L"PlaybackPaused")
        {
            // Inform the system that playback has paused
            smtc.PlaybackStatus(MediaPlaybackStatus::Paused);
        }
        else if (message == L"PlaybackEnded")
        {
            // Inform the system that playback stopped
            smtc.PlaybackStatus(MediaPlaybackStatus::Stopped);
        }
        else if (message == L"TimeUpdate" && args)
        {
            // This message is sent at regular intervals when playback is occurring.
            // Ensure that the expected arguments are included.
            if (args.HasKey(L"CurrentTime") && args.HasKey(L"Duration"))
            {
                double currentTime = args.GetNamedNumber(L"CurrentTime");
                double duration = args.GetNamedNumber(L"Duration");

                // Keep the system up to date on our current playback position
                UpdatePlaybackProgress(currentTime, duration);
            }
            else
            {
                OutputDebugString(L"Missing CurrentTime or Duration argument with TimeUpdate message.\n");
            }
        }
        else if (message == L"VideoUpdate" && args)
        {
            // This message is passed when the selected video changes.
            hstring newTitle{ L"" };
            hstring newSubtitle{ L"" };

            // Treat arguments to this message as optional, but log if the title is missing.
            if (args.HasKey(L"Title"))
            {
                newTitle = args.GetNamedString(L"Title");
            }
            else
            {
                OutputDebugString(L"Missing Title with VideoUpdate message.\n");
            }
            if (args.HasKey(L"Subtitle"))
            {
                newSubtitle = args.GetNamedString(L"Subtitle");
            }

            UpdateVideoMetadata(newTitle, newSubtitle);
        }
        else
        {
            std::wostringstream strStream{};
            strStream << L"Unexpected JSON message:\n" << message.c_str() << std::endl;
            OutputDebugString(strStream.str().c_str());
        }
    }

    /// <summary>
    /// Keep the system in-sync with the current playback status as it changes. It is recommended
    /// to call this function at least every 5 seconds to ensure that the system and the
    /// application don't get out of sync.
    /// 
    /// In this sample, it is called more often because it is hooked up to the ontimeupdate event
    /// on the video element.
    /// </summary>
    /// <param name="currentTime">The current spot the playhead is at in the video.</param>
    /// <param name="duration">The total length of the video.</param>
    void MainPage::UpdatePlaybackProgress(double currentTime, double duration)
    {
        TimeSpan currentTimeSpan{ std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>(currentTime)) };
        TimeSpan durationSpan{ std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>(duration)) };

        SystemMediaTransportControlsTimelineProperties timelineProps{};
        timelineProps.StartTime(TimeSpan::zero());
        timelineProps.MinSeekTime(TimeSpan::zero());
        timelineProps.Position(currentTimeSpan);
        timelineProps.MaxSeekTime(durationSpan);
        timelineProps.EndTime(durationSpan);

        smtc.UpdateTimelineProperties(timelineProps);
    }

    /// <summary>
    /// Keep the system informed of the currently-playing content. The most important part of
    /// this function is that it informs the system that the currently playing content is a 
    /// video. Additionally it passes along some metadata. As of 01/2023 this metadata is not
    /// presently shown for videos anywhere in the Xbox UI, but it is plumbed here in case that
    /// changes in the future.
    /// </summary>
    /// <param name="title">The title of the video being played.</param>
    /// <param name="subtitle">The subtitle of the video being played.</param>
    void MainPage::UpdateVideoMetadata(hstring const& title, hstring const& subtitle)
    {
        SystemMediaTransportControlsDisplayUpdater updater{ smtc.DisplayUpdater() };
        updater.Type(MediaPlaybackType::Video);
        updater.VideoProperties().Title(title);
        updater.VideoProperties().Subtitle(subtitle);

        // There are a few other properties that can be set on the VideoProperties object in
        // addition to title and subtitle. You may plumb them if you wish.

        updater.Update();
    }

    /// <summary>
    /// Calls the appropriate JavaScript function to perform the action that matches the button the
    /// user pressed on the media remote.
    /// </summary>
    /// <param name="button">The button the user pressed.</param>
    fire_and_forget MainPage::HandleSMTCButtonPressed(SystemMediaTransportControlsButton const button)
    {
        // This callback can occur on a background thread. We need to interact with the WebView,
        // so this call marshalls the handler back to the UI thread.
        co_await webView.Dispatcher();

        // Only handle button presses if we're fully navigated to a page in the WebView.
        if (isNavigatedToPage)
        {
            switch (button)
            {
                // Call an appropriate function in the JavaScript code based on which button
                // the user pressed.
            case SystemMediaTransportControlsButton::Play:
                co_await webView.ExecuteScriptAsync(L"play();");
                break;
            case SystemMediaTransportControlsButton::Pause:
                co_await webView.ExecuteScriptAsync(L"pause();");
                break;
            case SystemMediaTransportControlsButton::Stop:
                co_await webView.ExecuteScriptAsync(L"resetPlayback();");
                break;
            case SystemMediaTransportControlsButton::Next:
                co_await webView.ExecuteScriptAsync(L"nextVideo();");
				break;
            case SystemMediaTransportControlsButton::Previous:
				co_await webView.ExecuteScriptAsync(L"previousVideo();");
				break;
            default:
                std::wostringstream strStream{};
                strStream << L"Unsupported button pressed: " << static_cast<int>(button) << std::endl;
                OutputDebugString(strStream.str().c_str());
                break;
            }
        }
    }

    /// <summary>
    /// Informs the JavaScript code that it needs to update the current display mode of the TV.
    /// </summary>
    fire_and_forget MainPage::UpdateDisplayMode()
    {
        // Marshal back to the UI thread so we can interact with the WebView
        co_await webView.Dispatcher();

        // When the display changes (eg. the HDMI cable is plugged into a new device) ensure
        // the new device is in the correct mode for the current content.
        OutputDebugString(L"Display mode has changed.\n");
        co_await webView.ExecuteScriptAsync(L"updateDisplayModeAsync();");
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
