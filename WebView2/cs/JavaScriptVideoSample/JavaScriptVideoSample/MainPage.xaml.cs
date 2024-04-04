// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Diagnostics;
using Windows.Data.Json;
using Windows.Media;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.System.Profile;

namespace JavaScriptVideoSample
{
    /// <summary>
    /// Sample page which loads all of its UI in a WebView
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// The WebView in which we will be hosting our app's UI
        /// </summary>
        private WebView2 webView;

        /// <summary>
        /// Because we host the video inside the WebView, we need to update the SystemMediaTransportControls
        /// manually so that the system knows that we're playing a video and can adjust its behavior
        /// accordingly. For instance, it can prevent the screen from dimming when a video is playing, despite
        /// the user not pressing any buttons on the controller.
        /// 
        /// The SystemMediaTransportControls also enable interaction from physical media remotes.
        /// </summary>
        private readonly SystemMediaTransportControls smtc;

        /// <summary>
        /// This is the URI that will be loaded when the app launches.
        /// For the purposes of this sample, we're using HTML/JavaScript embedded within the app itself, but
        /// you can just as easily point this to a web URL where your app code is hosted instead.
        /// </summary>
        private const string initialUri = "https://local.webcode/video-player.html";

        /// <summary>
        /// Tracks whether or not the WebView has completed successful navigation to a page.
        /// </summary>
        private bool isNavigatedToPage = false;

        public MainPage()
        {
            this.InitializeComponent();
            this.Loaded += OnLoaded;

            // By default, Xbox gives you a border around your content to help you keep it inside a "TV-safe"
            // area. This helps protect you from drawing too close to the edges of the screen where content may
            // not be visible due to physical variations in televisions.
            //
            // This line disables that behavior. If you want, you can restore the automatic TV-safe area by
            // commenting this line out. Otherwise, be careful not to draw vital content too close to the edge
            // of the screen. Details can be found here:
            // https://docs.microsoft.com/en-us/windows/apps/design/devices/designing-for-tv#tv-safe-area
            ApplicationView.GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode.UseCoreWindow);

            // By default, XAML apps are scaled up 2x on Xbox. This line disables that behavior, allowing the
            // app to use the actual resolution of the device (1920 x 1080 pixels).
            if (!ApplicationViewScaling.TrySetDisableLayoutScaling(true))
            {
                Debug.WriteLine("Error: Failed to disable layout scaling.");
            }

            // Set up the SystemMediaTransportControls. At least IsPlayEnabled and IsPauseEnabled must be set
            // to true for the system to know how to properly tailor the user experience.
            smtc = SystemMediaTransportControls.GetForCurrentView();
            smtc.IsPlayEnabled = true;
            smtc.IsPauseEnabled = true;
            smtc.IsStopEnabled = true;
            smtc.IsNextEnabled = true;
            smtc.IsPreviousEnabled = true;
            smtc.ButtonPressed += OnSMTCButtonPressed;

            // Hook up an event so that we can inform the JavaScript code when the HDMI display mode chages.
            var hdmiInfo = Windows.Graphics.Display.Core.HdmiDisplayInformation.GetForCurrentView();
            if (hdmiInfo != null)
            {
                hdmiInfo.DisplayModesChanged += OnDisplayModeChanged;
            }
        }

        /// <summary>
        /// Called when the XAML page has finished loading.
        /// We wait to initialize the WebView until this point because it can take a little time to load.
        /// </summary>
        /// <param name="sender">The page which completed loading.</param>
        /// <param name="e">Details about the load operation.</param>
        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            // Create the WebView and point it at the initial URL to begin loading the app's UI.
            // The WebView is not added to the XAML page until the NavigationCompleted event fires.
            webView = new WebView2();
            InitializeWebView();
        }

        /// <summary>
        /// Sets up the web view, hooking up all of the event handlers that the app cares about.
        /// </summary>
        private async void InitializeWebView()
        {
            // The WebView XAML background color can sometimes show while a page is loading. Set it to
            // something that matches the app's color scheme so it does not produce a jarring flash.
            webView.Background = new SolidColorBrush(Color.FromArgb(255, 16, 16, 16));

            await webView.EnsureCoreWebView2Async();
            var coreWV2 = webView.CoreWebView2;
            if (coreWV2 != null)
            {
                // Change some settings on the WebView. Check the documentation for more options:
                // https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2settings
                coreWV2.Settings.AreDefaultContextMenusEnabled = false;
                coreWV2.Settings.IsGeneralAutofillEnabled = false;
                coreWV2.Settings.IsPasswordAutosaveEnabled = false;
                coreWV2.Settings.IsStatusBarEnabled = false;
                coreWV2.Settings.HiddenPdfToolbarItems = CoreWebView2PdfToolbarItems.None;

                // This prevents the user from opening the DevTools with F12, it does not prevent you from
                // attaching the Edge Dev Tools yourself.
                coreWV2.Settings.AreDevToolsEnabled = false; 

                // This creates a virtual URL which can be used to navigate the WebView to a local folder
                // embedded within the app package. If your application uses entirely pages hosted on the
                // web, you should remove this line.
                coreWV2.SetVirtualHostNameToFolderMapping("local.webcode", "WebCode", CoreWebView2HostResourceAccessKind.Allow);

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
                var dispatchAdapter = new WinRTAdapter.DispatchAdapter();

                // This line adds the official APIs directly.
                coreWV2.AddHostObjectToScript("Windows", dispatchAdapter.WrapNamedObject("Windows", dispatchAdapter));
                // This line adds custom proxy objects for APIs which would crash if called directly
                // from JavaScript. See the WindowsAPIProxies project in this solution for details.
                coreWV2.AddHostObjectToScript("WindowsAPIProxies", dispatchAdapter.WrapNamedObject("WindowsAPIProxies", dispatchAdapter));

                // Set some JavaScript code to run for each page. This sets some properties about how
                // the projected APIs will behave and creates "Windows" and "WindowsProxies" objects
                // at the top level to make it easier to access these APIs.
                await coreWV2.AddScriptToExecuteOnDocumentCreatedAsync(
                "(() => {" +
                    "if (chrome && chrome.webview) {" +
                        "console.log('Setting up WinRT projection options');" +
                        "chrome.webview.hostObjects.options.defaultSyncProxy = true;" +
                        "chrome.webview.hostObjects.options.forceAsyncMethodMatches = [/Async$/,/AsyncWithSpeller$/];" +
                        "chrome.webview.hostObjects.options.ignoreMemberNotFoundError = true;" +
                        "window.Windows = chrome.webview.hostObjects.sync.Windows;" +
                        "window.WindowsProxies = chrome.webview.hostObjects.sync.WindowsAPIProxies;" +
                    "}" +
                "})();");

                // Hook up the event handlers before setting the source so none of these events get missed.
                webView.WebMessageReceived += OnWebMessageReceived;
                webView.NavigationStarting += OnNavigationStarting;
                webView.NavigationCompleted += OnNavigationCompleted;

                // Pass the device type as a query parameter. This data could be passed in other ways as
                // well, such as by setting the UserAgent string in the WebView's CoreWebView2Settings,
                // or as part of the call to AddScriptToExecuteOnDocumentCreatedAsync.
                // Possible values for the DeviceForm include:
                //   "Xbox One"
                //   "Xbox One S"
                //   "Xbox One X"
                //   "Xbox Series S"
                //   "Xbox Series X"
                string deviceType = AnalyticsInfo.DeviceForm;
                string queryString = $"?deviceType={deviceType}";

                // This will cause the WebView to navigate to our initial page.
                webView.Source = new Uri(initialUri + queryString);
            }
            else
            {
                // TODO: Show an error state
                Debug.WriteLine($"Unable to retrieve CoreWebView2");
            }
        }

        /// <summary>
        /// Called whenever the WebView begins navigating to a new page.
        /// </summary>
        /// <param name="sender">The WebView that is beginning navigation.</param>
        /// <param name="args">Details about the new page which is to be loaded.</param>
        private void OnNavigationStarting(WebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            isNavigatedToPage = false;
        }

        /// <summary>
        /// Called whenever a new page is fully loaded (or fails to load) in the WebView.
        /// </summary>
        /// <param name="sender">The WebView that just loaded a new page.</param>
        /// <param name="args">Details about the page which was loaded.</param>
        private void OnNavigationCompleted(WebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            // If we haven't done so yet, add the WebView to the page, making it visible to the user.
            // We create it dynamically and wait for the first navigation to complete before doing so.
            // This ensures that focus gets set on the WebView only after it is ready to receive it.
            // Failing to do this can occasionally result in Gamepad input failing to route to your
            // JavaScript code.
            if (webView.Parent == null)
            {
                if (args.IsSuccess)
                {
                    // Replace the current contents of this page with the WebView
                    this.Content = webView;
                    webView.Focus(FocusState.Programmatic);
                }
                else
                {
                    // WebView navigation failed.
                    // TODO: Show an error state
                    Debug.WriteLine($"Initial WebView navigation failed with error status: {args.WebErrorStatus}");
                }
            }

            // Track whether or not the WebView is successfully navigated to a page.
            if (args.IsSuccess)
            {
                isNavigatedToPage = true;
            }
        }

        /// <summary>
        /// Recieves any data that your web app passed to window.chrome.webview.postMessage().
        /// 
        /// The JavaScript code in this sample sends JSON data through
        /// this mechanism to inform the C# code when important events occur.
        /// </summary>
        /// <param name="sender">The WebView that called window.chrome.webview.postMessage()</param>
        /// <param name="args">An object containing the URL of the calling page and the data that was
        /// passed to window.chrome.webview.postMessage()</param>
        private void OnWebMessageReceived(WebView2 sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            // If the message contains valid JSON data, handle it as an event notification.
            // Otherwise, simply log the message as-is to the debug console.
            string jsonMessage = args.TryGetWebMessageAsString();
            if (JsonObject.TryParse(jsonMessage, out JsonObject argsJson))
            {
                HandleJsonNotification(argsJson);
            }
            else
            {
                Debug.WriteLine("Could not parse JSON message recieved:");
                Debug.WriteLine(jsonMessage);
            }
        }

        /// <summary>
        /// A helper function to handle JSON messages sent from the JavaScript code.
        /// </summary>
        /// <param name="json">The JSON object that was sent.</param>
        private void HandleJsonNotification(JsonObject json)
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

            string message = json.GetNamedString("Message");
            JsonObject args = json.GetNamedObject("Args");

            if (message == "PlaybackStarted")
            {
                // Inform the system that playback has started
                smtc.PlaybackStatus = MediaPlaybackStatus.Playing;
            }
            else if (message == "PlaybackPaused")
            {
                // Inform the system that playback has paused
                smtc.PlaybackStatus = MediaPlaybackStatus.Paused;
            }
            else if (message == "PlaybackEnded")
            {
                // Inform the system that playback stopped
                smtc.PlaybackStatus = MediaPlaybackStatus.Stopped;
            }
            else if (message == "TimeUpdate" && args != null)
            {
                // This message is sent at regular intervals when playback is occurring.
                // Ensure that the expected arguments are included.
                if (args.ContainsKey("CurrentTime") && args.ContainsKey("Duration"))
                {
                    double currentTime = args.GetNamedNumber("CurrentTime");
                    double duration = args.GetNamedNumber("Duration");

                    // Keep the system up to date on our current playback position
                    UpdatePlaybackProgress(currentTime, duration);
                }
                else
                {
                    Debug.WriteLine("Missing CurrentTime or Duration argument with TimeUpdate message.");
                }
            }
            else if (message == "VideoUpdate" && args != null)
            {
                // This message is passed when the selected video changes.
                string newTitle = "";
                string newSubtitle = "";

                // Treat arguments to this message as optional, but log if the title is missing.
                if (args.ContainsKey("Title"))
                {
                    newTitle = args.GetNamedString("Title");
                }
                else
                {
                    Debug.WriteLine("Missing Title with VideoUpdate message.");
                }
                if (args.ContainsKey("Subtitle"))
                {
                    newSubtitle = args.GetNamedString("Subtitle");
                }

                UpdateVideoMetadata(newTitle, newSubtitle);
            }
            else
            {
                Debug.WriteLine($"Unexpected JSON message: {message}");
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
        private void UpdatePlaybackProgress(double currentTime, double duration)
        {
            smtc.UpdateTimelineProperties(new SystemMediaTransportControlsTimelineProperties
            {
                StartTime = TimeSpan.Zero,
                MinSeekTime = TimeSpan.Zero,
                Position = TimeSpan.FromSeconds(currentTime),
                MaxSeekTime = TimeSpan.FromSeconds(duration),
                EndTime = TimeSpan.FromSeconds(duration)
            });
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
        private void UpdateVideoMetadata(string title, string subtitle)
        {
            SystemMediaTransportControlsDisplayUpdater updater = smtc.DisplayUpdater;
            updater.Type = MediaPlaybackType.Video;
            updater.VideoProperties.Title = title;
            updater.VideoProperties.Subtitle = subtitle;

            // There are a few other properties that can be set on the VideoProperties object in
            // addition to title and subtitle. You may plumb them if you wish.

            updater.Update();
        }

        /// <summary>
        /// Called whenever the user presses a button on a physical media remote.
        /// </summary>
        /// <param name="sender">The SystemMediaTransportControls which observed the button press.</param>
        /// <param name="args">Details about which button was pressed.</param>
        private async void OnSMTCButtonPressed(SystemMediaTransportControls sender, SystemMediaTransportControlsButtonPressedEventArgs args)
        {
            // This callback can occur on a background thread. We need to interact with the WebView,
            // so this call marshalls the handler back to the UI thread.
            await webView.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
            {
                // Only handle button presses if we're fully navigated to a page in the WebView.
                if (isNavigatedToPage)
                {
                    switch (args.Button)
                    {
                        // Call an appropriate function in the JavaScript code based on which button
                        // the user pressed.
                        case SystemMediaTransportControlsButton.Play:
                            await webView.ExecuteScriptAsync("play();");
                            break;
                        case SystemMediaTransportControlsButton.Pause:
                            await webView.ExecuteScriptAsync("pause();");
                            break;
                        case SystemMediaTransportControlsButton.Stop:
                            await webView.ExecuteScriptAsync("resetPlayback();");
                            break;
                        case SystemMediaTransportControlsButton.Next:
                            await webView.ExecuteScriptAsync("nextVideo();");
                            break;
                        case SystemMediaTransportControlsButton.Previous:
                            await webView.ExecuteScriptAsync("previousVideo();");
                            break;
                        default:
                            Debug.WriteLine($"Unsupported button pressed: {args.Button}");
                            break;
                    }
                }
            });
        }

        /// <summary>
        /// Called when the HDMI display mode changes. This can happen when the HDMI cable is unplugged or
        /// replugged, or when the mode changes for other reasons.
        /// </summary>
        /// <param name="sender">Information about the associated HDMI display.</param>
        /// <param name="args">Details about the display change operation.</param>
        private async void OnDisplayModeChanged(Windows.Graphics.Display.Core.HdmiDisplayInformation sender, object args)
        {
            await webView.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
            {
                // When the display changes (eg. the HDMI cable is plugged into a new device) ensure
                // the new device is in the correct mode for the current content.
                Debug.WriteLine("Display mode has changed.");
                await webView.ExecuteScriptAsync("updateDisplayModeAsync();");
            });
        }
    }
}
