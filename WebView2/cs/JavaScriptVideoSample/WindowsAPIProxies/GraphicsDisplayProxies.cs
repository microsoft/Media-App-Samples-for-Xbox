// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Graphics.Display.Core;

// Some Windows APIs can't be projected into JavaScript unaltered because they
// can return on a background thread instead of the calling thread. This causes
// a crash when they run in a JavaScript context. For more information, see:
// https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/threading-model
// 
// This file proxies those APIs in a way that they can be safely called from
// JavaScript.
namespace WindowsAPIProxies
{
    public sealed class GraphicsDisplayProxies
    {
        /// <summary>
        /// Proxy that calls:
        /// HdmiDisplayInformation.GetForCurrentView().RequestSetCurrentDisplayModeAsync(...)
        /// 
        /// Note that because this public function is exposed in a Windows Runtime Component, it
        /// cannot return Task. Instead, we use an internal function which returns a Task and call
        /// AsAsyncAction() on it.
        /// </summary>
        public static IAsyncOperation<bool> RequestSetCurrentDisplayModeAsync(HdmiDisplayMode mode, HdmiDisplayHdrOption hdrOption)
        {
            return RequestSetCurrentDisplayModeInternalAsync(mode, hdrOption).AsAsyncOperation();
        }

        private static async Task<bool> RequestSetCurrentDisplayModeInternalAsync(HdmiDisplayMode mode, HdmiDisplayHdrOption hdrOption)
        {
            var hdmiInfo = HdmiDisplayInformation.GetForCurrentView();
            return await hdmiInfo.RequestSetCurrentDisplayModeAsync(mode, hdrOption);
        }
    }
}
