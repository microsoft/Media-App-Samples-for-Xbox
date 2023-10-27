// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "App.xaml.g.h"

namespace winrt::JavaScriptMusicSample::implementation
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    struct App : AppT<App>
    {
        Windows::UI::Xaml::Controls::Frame rootFrame = nullptr;

        /// <summary>
        /// Set this value to true to cause it to show pop-up messages when the app's background
        /// status changes. This can be useful for debugging memory issues, especially because
        /// memory limits are not enforced when the app has a debugger attached.
        /// </summary>
        const bool showToasts = false;

        App();
        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, Windows::ApplicationModel::SuspendingEventArgs const&);
        void OnResuming(IInspectable const&, IInspectable const&);
        void OnNavigationFailed(IInspectable const&, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
        void OnEnteredBackground(IInspectable const&, Windows::ApplicationModel::EnteredBackgroundEventArgs const&);
        void OnLeavingBackground(IInspectable const&, Windows::ApplicationModel::LeavingBackgroundEventArgs const&);
        void CreateRootFrame(Windows::ApplicationModel::Activation::ApplicationExecutionState const& previousExecutionState, hstring const& arguments);
        hstring GetMemoryUsageText();
        void ShowToast(hstring const& msg, hstring const& subMsg = L"");
    };
}
