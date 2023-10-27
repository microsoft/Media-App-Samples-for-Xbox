// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "GraphicsDisplayProxies.g.h"

namespace winrt::WindowsAPIProxies::implementation
{
    struct GraphicsDisplayProxies : GraphicsDisplayProxiesT<GraphicsDisplayProxies>
    {
        GraphicsDisplayProxies() = default;

        static winrt::Windows::Foundation::IAsyncOperation<bool> RequestSetCurrentDisplayModeAsync(winrt::Windows::Graphics::Display::Core::HdmiDisplayMode mode, winrt::Windows::Graphics::Display::Core::HdmiDisplayHdrOption hdrOption);
    };
}
namespace winrt::WindowsAPIProxies::factory_implementation
{
    struct GraphicsDisplayProxies : GraphicsDisplayProxiesT<GraphicsDisplayProxies, implementation::GraphicsDisplayProxies>
    {
    };
}
