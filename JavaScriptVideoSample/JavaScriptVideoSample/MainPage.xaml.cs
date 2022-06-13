// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics;
using Windows.Data.Json;
using Windows.Media;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

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
        private readonly WebView webView;

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
        /// you can just as easily point this to a https web URL where your app code is hosted instead.
        /// </summary>
        private const string initialUri = "ms-appx-web:///WebCode/video-player.html";

        /// <summary>
        /// Tracks whether or not the WebView has completed successful navigation to a page.
        /// </summary>
        private bool isNavigatedToPage = false;

        public MainPage()
        {
            this.InitializeComponent();

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
            smtc.ButtonPressed += OnSMTCButtonPressed;

            // This app only plays a single video. In real-world scenarios that play more than one video, the
            // app would need to use the DisplayUpdater to inform the system every time the video changes.
            SystemMediaTransportControlsDisplayUpdater updater = smtc.DisplayUpdater;
            updater.Type = MediaPlaybackType.Video;
            updater.VideoProperties.Title = "Sintel Trailer";
            updater.Update();

            // Create the WebView and point it at the initial URL to begin loading the app's UI.
            // The WebView is not added to the XAML page until the NavigationCompleted event fires.
            webView = new WebView();
            webView.NavigationStarting += OnNavigationStarting;
            webView.NavigationCompleted += OnNavigationCompleted;
            webView.ScriptNotify += OnScriptNotify;
            webView.Navigate(new Uri(initialUri));
        }

        /// <summary>
        /// Called whenever the WebView begins navigating to a new page.
        /// </summary>
        /// <param name="sender">The WebView that is beginning navigation.</param>
        /// <param name="args">Details about the new page which is to be loaded.</param>
        private void OnNavigationStarting(WebView sender, WebViewNavigationStartingEventArgs args)
        {
            isNavigatedToPage = false;
        }

        /// <summary>
        /// Called whenever a new page is fully loaded (or fails to load) in the WebView.
        /// </summary>
        /// <param name="sender">The WebView that just loaded a new page.</param>
        /// <param name="args">Details about the page which was loaded.</param>
        private void OnNavigationCompleted(WebView sender, WebViewNavigationCompletedEventArgs args)
        {
            // If we haven't done so yet, add the WebView to the page, making it visible to the user.
            // We create it dynamically and wait for the first navigation to complete before doing so.
            // This ensures that focus gets set on the WebView only after it is ready to receive it.
            // Failing to do this can occasionally result in Gamepad input failing to route to your
            // JavaScript code.
            // 
            // You can instead run this code upon receiving the WebView.DOMContentLoaded event, if
            // desired. It will show your page to the user sooner, but the page may not be fully
            // styled or ready to react to user input.
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
        /// Recieves any data that your web app passed to window.external.notify().
        /// 
        /// NOTE: In order to use ScriptNotify, your website's URL must be added to the
        /// ApplicationContentUriRules in your appxmanifest file. You can do this in Visual Studio
        /// by double-clicking the appxmanifest file and adding it under the Content URIs tab.
        /// 
        /// In this sample, we are using redirectconsole-1.0.0.0.js to redirect JavaScript console
        /// output to window.external.notify() so it can be caught and logged here. This is helpful
        /// if you are using Visual Studio 2019 or later, which does not have built-in Script debugging
        /// tools for WebView. If you are using Visual Studio 2017, you can omit the redirectconsole
        /// library from your JavaScript code and rely on Script debugging instead.
        /// 
        /// In addition to log messages, the JavaScript code in this sample sends JSON data through
        /// this mechanism to inform the C# code when important events occur.
        /// </summary>
        /// <param name="sender">The WebView that called window.external.notify()</param>
        /// <param name="args">An object containing the URL of the calling page and the string that was
        /// passed to window.external.notify()</param>
        private void OnScriptNotify(object sender, NotifyEventArgs args)
        {
            // If the message contains valid JSON data, handle it as an event notification.
            // Otherwise, simply log the message as-is to the debug console.
            if (JsonObject.TryParse(args.Value, out JsonObject argsJson))
            {
                HandleJsonNotification(argsJson);
            }
            else
            {
                Debug.WriteLine(args.Value);
            }
        }

        /// <summary>
        /// A helper function to handle JSON messages sent from the JavaScript code.
        /// </summary>
        /// <param name="json">The JSON object that was sent.</param>
        private void HandleJsonNotification(JsonObject json)
        {
            // The JavaScript code is capable of sending any arbitrary JSON message that the C# code is
            // prepared to handle. For the purposes of this sample we expect it to have the form:
            //
            // {
            //     "Message": "<a well-known string that tells the C# code what event occurred>",
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
                StartTime = TimeSpan.FromSeconds(0),
                MinSeekTime = TimeSpan.FromSeconds(0),
                Position = TimeSpan.FromSeconds(currentTime),
                MaxSeekTime = TimeSpan.FromSeconds(duration),
                EndTime = TimeSpan.FromSeconds(duration)
            });
        }

        /// <summary>
        /// Called whenever the user presses a button on a physical media remote.
        /// </summary>
        /// <param name="sender">The SystemMediaTransportControls which observed the button press.</param>
        /// <param name="args">Details about which button was pressed.</param>
        private async void OnSMTCButtonPressed(SystemMediaTransportControls sender, SystemMediaTransportControlsButtonPressedEventArgs args)
        {
            try
            {
                // This callback can occur on a background thread. We need to interact with the WebView,
                // so this call marshalls the handler back to the UI thread.
                await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
                {
                    // Only handle button presses if we're fully navigated to a page in the WebView.
                    if (isNavigatedToPage)
                    {
                        switch (args.Button)
                        {
                            // Call an appropriate function in the JavaScript code based on which button
                            // the user pressed.
                            case SystemMediaTransportControlsButton.Play:
                                await webView.InvokeScriptAsync("play", arguments: null);
                                break;
                            case SystemMediaTransportControlsButton.Pause:
                                await webView.InvokeScriptAsync("pause", arguments: null);
                                break;
                            case SystemMediaTransportControlsButton.Stop:
                                await webView.InvokeScriptAsync("resetPlayback", arguments: null);
                                break;
                            default:
                                Debug.WriteLine($"Unsupported button pressed: {args.Button}");
                                break;
                        }
                    }
                });
            }
            catch (Exception e)
            {
                Debug.WriteLine($"Error: Failed to handle SMTC button press. {e.Message}");
            }
        }
    }
}
