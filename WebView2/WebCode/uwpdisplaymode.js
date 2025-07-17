// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This is a small helper script for calling some common functions for checking the
// attached display device's supported functionalities and switching its modes.
//
// This script expects the native code to expose these namespaces via the WinRTAdapter
// under a top-level object called "Windows":
// Windows.Media.Protection.ProtectionCapabilities
// Windows.Media.Protection.ProtectionCapabilityResult
// Windows.Graphics.Display.Core
//
// Additionally, the WindowsAPIProxies project wraps some WinRT APIs so they can be
// safely called from JavaScript. These objects are expected to be added by the native
// code under a top-level object called "WindowsProxies".
//
// You should customize this script based on the types of content your app displays.
var uwpDisplayMode = (function (Windows, WindowsProxies) {
    let public = {};

    // Common strings for device capability WinRT API calls
    const SUPPORT_SUFFIX = "\"";
    const SUPPORT_4K_FEATURES_AFFIX = "features=\"decode-res-x=3840,decode-res-y=2160,decode-bitrate=20000,decode-fps=30,decode-bpc=10,display-res-x=3840,display-res-y=2160,display-bpc=8";
    const SUPPORT_4K_MP4_PREFIX = "video/mp4;codecs=\"hvc1,mp4a\";" + SUPPORT_4K_FEATURES_AFFIX;
    const SUPPORT_4K_VP9_PREFIX = "video/mp4;codecs=\"vp09\";" + SUPPORT_4K_FEATURES_AFFIX;
    const SUPPORT_HDR_AFFIX = ",hdr=1";
    const SUPPORT_DV_AFFIX = ",ext-profile=dvhe.05";

    // Strings useful for checking types of HDCP support
    public.hdcpTypes = {
        hdcp1: "video/mp4;codecs=\"hvc1,mp4a\";features=\"hdcp=1\"",
        hdcp2: "video/mp4;codecs=\"hvc1,mp4a\";features=\"hdcp=2\""
    };

    // Strings useful for checking various kinds of video support
    public.videoTypes = {
        fullhd: "video/mp4;codecs=\"avc1,mp4a\";features=\"display-res-x=1920,display-res-y=1080,display-bpc=8\"",
        sdr4k: SUPPORT_4K_MP4_PREFIX + SUPPORT_SUFFIX,
        hdr4k: SUPPORT_4K_MP4_PREFIX + SUPPORT_HDR_AFFIX + SUPPORT_SUFFIX,
        dolbyVision: SUPPORT_4K_MP4_PREFIX + SUPPORT_HDR_AFFIX + SUPPORT_DV_AFFIX + SUPPORT_SUFFIX,
        vp9: SUPPORT_4K_VP9_PREFIX + SUPPORT_SUFFIX
    };

    // Strings useful for checking various kinds of audio support
    public.audioTypes = {
        mp4a: "video/mp4;codecs=\"avc1,mp4a\";",
        dolbyDigital: "video/mp4;codecs=\"hvc1,ac-3\";",
        dolbyDigitalPlus: "video/mp4;codecs=\"hvc1,ec-3\";"
    };

    // Sample function which switches the display to 4K Dolby Vision mode,
    // if supported. Returns false otherwise, and prints warnings to the console.
    public.switchTVModeTo4KDVAsync = async function () {
        // Validate display supports Dolby Vision and HDCP2
        if (!await this.isTypeSupportedAsync(this.videoTypes.dolbyVision)) {
            console.warn("Display does not support 4K Dolby Vision");
            return false;
        }
        if (!await this.isTypeSupportedAsync(this.hdcpTypes.hdcp2)) {
            console.warn("Display does not support HDCP2");
            return false;
        }

        let hdmiInfo = Windows.Graphics.Display.Core.HdmiDisplayInformation.getForCurrentView();
        let modes = hdmiInfo.getSupportedDisplayModes();

        // Find the Dolby Vision mode
        let desiredMode = modes.find(mode =>
            (mode.refreshRate + 0.5) >= 60 && // Some TVs report a refresh rate a few decimals lower than
            (mode.refreshRate + 0.5) < 120 && // 60 or 120. Add a small delta to bump it over the threshold.
            mode.resolutionWidthInRawPixels >= 3840 &&
            mode.isDolbyVisionLowLatencySupported);

        // Change to the desired mode, if able
        if (!desiredMode) {
            return false;
        } else {
            return await WindowsProxies.GraphicsDisplayProxies.requestSetCurrentDisplayModeAsync(
                desiredMode, Windows.Graphics.Display.Core.HdmiDisplayHdrOption.dolbyVisionLowLatency);
        }
    };

    // Sample function which switches the display to 4K HDR mode,
    // if supported. Returns false otherwise, and prints warnings to the console.
    public.switchTVModeTo4KHDRAsync = async function () {
        // Validate display supports 4K HDR and HDCP2
        if (!await this.isTypeSupportedAsync(this.videoTypes.hdr4k)) {
            console.warn("Display does not support 4K HDR");
            return false;
        }
        if (!await this.isTypeSupportedAsync(this.hdcpTypes.hdcp2)) {
            console.warn("Display does not support HDCP2");
            return false;
        }

        let hdmiInfo = Windows.Graphics.Display.Core.HdmiDisplayInformation.getForCurrentView();
        let modes = hdmiInfo.getSupportedDisplayModes();

        // Find the appropriate mode
        let desiredMode = modes.find(mode =>
            (mode.refreshRate + 0.5) >= 60 && // Some TVs report a refresh rate a few decimals lower than
            (mode.refreshRate + 0.5) < 120 && // 60 or 120. Add a small delta to bump it over the threshold.
            mode.resolutionWidthInRawPixels >= 3840 &&
            mode.isSmpte2084Supported);

        // Change to the desired mode, if able
        if (!desiredMode) {
            return false;
        } else {
            return await WindowsProxies.GraphicsDisplayProxies.requestSetCurrentDisplayModeAsync(
                desiredMode, Windows.Graphics.Display.Core.HdmiDisplayHdrOption.eotf2084);
        }
    };

    // Sample function which switches the display to 4K SDR mode,
    // if supported. Returns false otherwise, and prints warnings to the console.
    public.switchTVModeTo4KSDRAsync = async function () {
        // Validate display supports 4K SDR and HDCP
        if (!this.isTypeSupportedAsync(this.videoTypes.sdr4k)) {
            console.warn("Display does not support 4K Resolution");
            return false;
        }
        if (!await this.isTypeSupportedAsync(this.hdcpTypes.hdcp1)) {
            console.warn("Display does not support HDCP");
            return false;
        }

        let hdmiInfo = Windows.Graphics.Display.Core.HdmiDisplayInformation.getForCurrentView();
        let modes = hdmiInfo.getSupportedDisplayModes();

        // Find the appropriate mode
        let desiredMode = modes.find(mode =>
            (mode.refreshRate + 0.5) >= 60 && // Some TVs report a refresh rate a few decimals lower than
            (mode.refreshRate + 0.5) < 120 && // 60 or 120. Add a small delta to bump it over the threshold.
            mode.resolutionWidthInRawPixels >= 3840 &&
            mode.isSdrLuminanceSupported);

        // Change to the desired mode, if able
        if (!desiredMode) {
            return false;
        } else {
            return await WindowsProxies.GraphicsDisplayProxies.requestSetCurrentDisplayModeAsync(
                desiredMode, Windows.Graphics.Display.Core.HdmiDisplayHdrOption.eotfSdr);
        }
    };

    // Sample function which switches the display to a 50Hz display mode,
    // if supported. Returns false otherwise, and prints warnings to the console.
    // If your content is authored with a 25Hz or 50Hz refresh rate, setting the
    // display to a 50Hz display mode is important to avoid visual judder.
    public.switchTVModeTo50HzAsync = async function () {
        let hdmiInfo = Windows.Graphics.Display.Core.HdmiDisplayInformation.getForCurrentView();
        let modes = hdmiInfo.getSupportedDisplayModes();

        // Find the first 50Hz mode
        // If your content has other display needs (such as 4K, or HDR) you should modify this code to
        // check for those as well.
        let desiredMode = modes.find(mode =>
            (mode.refreshRate + 0.5) >= 50 && // Some TVs report a refresh rate a few decimals lower than
            (mode.refreshRate + 0.5) < 60);   // 50 or 60. Add a small delta to bump it over the threshold.

        // Change to the desired mode, if able
        if (!desiredMode) {
            return false;
        } else {
            return await WindowsProxies.GraphicsDisplayProxies.requestSetCurrentDisplayModeAsync(
                desiredMode, Windows.Graphics.Display.Core.HdmiDisplayHdrOption.none);
        }
    };

    // Calls Windows.Media.Protection.ProtectionCapabilities().IsTypeSupported() in a loop
    // until it gets a non-maybe result.
    // https://learn.microsoft.com/en-us/uwp/api/windows.media.protection.protectioncapabilities.istypesupported
    public.isTypeSupportedAsync = async function (type) {
        try {
            // This string checks for PlayReady SL3000 (hardware) support. Your app must
            // specify the hevcPlayback capability in its appxmanifest file to use SL3000.
            // If you only need SL2000, use "com.microsoft.playready.recommendation" instead.
            // For more information see:
            // https://learn.microsoft.com/en-us/playready/overview/key-system-strings
            let playReadyVersion = "com.microsoft.playready.recommendation.3000";
            let protCap = Windows.Media.Protection.ProtectionCapabilities();
            let result = protCap.isTypeSupported(type, playReadyVersion);

            // Continue checking until we get a non-maybe result. This API will not return
            // "maybe" for more than 10 seconds.
            while (result == Windows.Media.Protection.ProtectionCapabilityResult.maybe) {
                await new Promise(r => setTimeout(r, 100));
                result = protCap.isTypeSupported(type, playReadyVersion);
            }

            return (result != Windows.Media.Protection.ProtectionCapabilityResult.notSupported);
        } catch (error) {
            console.error(error);
        }

        return false;
    };

    return public;
}(Windows, WindowsProxies));