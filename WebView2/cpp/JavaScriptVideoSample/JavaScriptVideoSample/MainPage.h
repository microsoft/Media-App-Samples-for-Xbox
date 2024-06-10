// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "MainPage.g.h"

namespace winrt::JavaScriptVideoSample::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
    public:
        MainPage();

    private:
        /// <summary>
        /// The WebView in which we will be hosting our app's UI
        /// </summary>
        Microsoft::UI::Xaml::Controls::WebView2 webView = nullptr;

        /// <summary>
        /// Because we host the video inside the WebView, we need to update the SystemMediaTransportControls
        /// manually so that the system knows that we're playing a video and can adjust its behavior
        /// accordingly. For instance, it can prevent the screen from dimming when a video is playing, despite
        /// the user not pressing any buttons on the controller.
        /// 
        /// The SystemMediaTransportControls also enable interaction from physical media remotes.
        /// </summary>
        Windows::Media::SystemMediaTransportControls smtc = nullptr;

        /// <summary>
        /// This is the URI that will be loaded when the app launches.
        /// For the purposes of this sample, we're using HTML/JavaScript embedded within the app itself, but
        /// you can just as easily point this to a web URL where your app code is hosted instead.
        /// </summary>
        const hstring initialUri = L"https://local.webcode/video-player.html";

        /// <summary>
        /// Tracks whether or not the WebView has completed successful navigation to a page.
        /// </summary>
        bool isNavigatedToPage = false;

        fire_and_forget InitializeWebView();
        void OnNavigationStarting(Microsoft::UI::Xaml::Controls::WebView2 const&, Microsoft::Web::WebView2::Core::CoreWebView2NavigationStartingEventArgs const&);
        void OnNavigationCompleted(Microsoft::UI::Xaml::Controls::WebView2 const&, Microsoft::Web::WebView2::Core::CoreWebView2NavigationCompletedEventArgs const&);
        void OnWebMessageReceived(Microsoft::UI::Xaml::Controls::WebView2 const&, Microsoft::Web::WebView2::Core::CoreWebView2WebMessageReceivedEventArgs const&);
        void OnSMTCButtonPressed(Windows::Media::SystemMediaTransportControls const&, Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs const&);
        void OnDisplayModeChanged(Windows::Graphics::Display::Core::HdmiDisplayInformation const&, Windows::Foundation::IInspectable const&);

        void HandleJsonNotification(Windows::Data::Json::JsonObject const& json);
        void UpdatePlaybackProgress(double currentTime, double duration);
        void UpdateVideoMetadata(hstring const& title, hstring const& subtitle);
        fire_and_forget HandleSMTCButtonPressed(Windows::Media::SystemMediaTransportControlsButton const button);
        fire_and_forget UpdateDisplayMode();
    };
}

namespace winrt::JavaScriptVideoSample::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
