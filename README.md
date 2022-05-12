--- 
description: "Sample showing how to build music and video apps using primarily web technologies for Xbox."
languages: 
  - csharp
  - javascript
  - html
name: "Media App Sample for Xbox"
page_type: sample
products: 
  - xbox
urlFragment: media-app-samples-for-xbox
---

# Media App Samples for Xbox

This project contains two samples, a video player and a music player. Both apps are meant to be run on Xbox, and both are built primarily in JavaScript and HTML, running in a thin C# wrapper containing a full-screen WebView. Both apps utilize a small JavaScript library which may be independently useful, redirectconsole-1.0.0.0.js, which aids in debugging JavaScript code running in a WebView in versions of Visual Studio greater than 2017.

> Note - These samples are targeted and tested for Windows 11 (10.0; Build 22000), and Visual Studio 2017. If you prefer, you can use project properties to retarget the project(s) to Windows 10, version 2104 (10.0; Build 20348).

These samples run on the Universal Windows Platform (UWP). 

![Video app screenshot](/Images/VideoAppScreenshot.png)

## Features

These samples highlight the following topics:

* [UWP on Xbox One](https://docs.microsoft.com/windows/uwp/xbox-apps/)
* [Media Playback](https://docs.microsoft.com/windows/uwp/audio-video-camera/media-playback)
* [WebView](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.WebView) ([Windows.UI.Xaml.Controls](https://docs.microsoft.com/uwp/api/windows.ui.xaml.controls))

## Universal Windows Platform development

### Prerequisites

- Windows 10 or 11. Minimum: Windows 10, version 1809 (10.0; Build 17763), also known as the Windows 10 October 2018 Update.
- [Windows SDK](https://developer.microsoft.com/windows/downloads/windows-sdk/). Minimum: Windows SDK version 10.0.17763.0 (Windows 10, version 1809).
- [Visual Studio 2017](https://visualstudio.microsoft.com/vs/older-downloads/) You can use the free Visual Studio Community Edition to build and run Windows Universal Platform (UWP) apps. Visual Studio 2019 or 2022 can be used as well, but lack script debugging tools for WebView, so they are not recommended.

To get the latest updates to Windows and the development tools, and to help shape their development, join 
the [Windows Insider Program](https://insider.windows.com).

## Running the samples

You can simply hit Start Debugging (F5) to try these samples on your local machine.

To test the app on an Xbox, you will need to enable [Developer Mode on your Xbox](https://docs.microsoft.com/windows/uwp/xbox-apps/devkit-activation). Then you can configure Visual Studio to deploy to it as a Remote Machine:
1. Right-click the main project in the Solution Explorer, and then select Properties.
2. Under Start options, change Target device to Remote Machine.
3. In Remote machine, enter the system IP address or hostname of the Xbox One console. For information about obtaining the IP address or hostname, see [Introduction to Xbox One tools](https://docs.microsoft.com/windows/uwp/xbox-apps/introduction-to-xbox-tools).
4. In the Authentication Mode drop-down list, select Universal (Unencrypted Protocol).

When you next hit Start Debugging (F5) it may ask you for a pairing PIN. This can be found in the [Dev Home app](https://docs.microsoft.com/windows/uwp/xbox-apps/dev-home) on your Xbox.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
