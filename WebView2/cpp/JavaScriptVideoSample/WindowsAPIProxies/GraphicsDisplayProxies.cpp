// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "GraphicsDisplayProxies.h"
#include "GraphicsDisplayProxies.g.cpp"

namespace winrt::WindowsAPIProxies::implementation
{
    winrt::Windows::Foundation::IAsyncOperation<bool> GraphicsDisplayProxies::RequestSetCurrentDisplayModeAsync(winrt::Windows::Graphics::Display::Core::HdmiDisplayMode mode, winrt::Windows::Graphics::Display::Core::HdmiDisplayHdrOption hdrOption)
    {
        auto hdmiInfo = winrt::Windows::Graphics::Display::Core::HdmiDisplayInformation::GetForCurrentView();
        bool success = co_await hdmiInfo.RequestSetCurrentDisplayModeAsync(mode, hdrOption);
        co_return success;
    }
}
