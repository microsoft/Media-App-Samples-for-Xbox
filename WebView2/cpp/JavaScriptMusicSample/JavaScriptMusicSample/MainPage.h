// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainPage.g.h"

namespace winrt::JavaScriptMusicSample::implementation
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
        /// This is the URI that will be loaded when the app launches.
        /// For the purposes of this sample, we're using HTML/JavaScript embedded within the app itself, but
        /// you can just as easily point this to a web URL where your app code is hosted instead.
        /// </summary>
        const hstring initialUri = L"https://local.webcode/music-player.html";

        winrt::event_token navigationCompletedEventToken{};

        void OnUnloaded(IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&);
        void OnNavigationCompleted(Microsoft::UI::Xaml::Controls::WebView2 const&, Microsoft::Web::WebView2::Core::CoreWebView2NavigationCompletedEventArgs const&);
        fire_and_forget InitializeWebView();
    };
}

namespace winrt::JavaScriptMusicSample::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
