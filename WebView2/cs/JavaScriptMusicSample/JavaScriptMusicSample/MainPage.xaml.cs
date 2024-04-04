// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using NativeMediaPlayer;
using System;
using System.Diagnostics;
using Windows.UI;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace JavaScriptMusicSample
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
        /// This is the URI that will be loaded when the app launches.
        /// For the purposes of this sample, we're using HTML/JavaScript embedded within the app itself, but
        /// you can just as easily point this to a web URL where your app code is hosted instead.
        /// </summary>
        private const string initialUri = "https://local.webcode/music-player.html";

        public MainPage()
        {
            this.InitializeComponent();
            this.Loaded += OnLoaded;

            // Never reuse the cached page because the model is designed to be unloaded and disposed
            this.NavigationCacheMode = NavigationCacheMode.Disabled;

            // By default, Xbox gives you a border around your content to help you keep it inside a "TV-safe"
            // area. This helps protect you from drawing too close to the edges of the screen where content may
            // not be visible due to physical variations in televisions.
            //
            // This line disables that behavior. If you want, you can restore the automatic TV-safe area by
            // commenting this line out. Otherwise, be careful not to draw vital content too close to the edge
            // of the screen. (A buffer of approximately 80 pixels is ideal).
            ApplicationView.GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode.UseCoreWindow);

            // By default, XAML apps are scaled up 2x on Xbox. This line disables that behavior, allowing the
            // app to use the actual resolution of the device (1920 x 1080 pixels).
            if (!ApplicationViewScaling.TrySetDisableLayoutScaling(true))
            {
                Debug.WriteLine("Error: Failed to disable layout scaling.");
            }

            // Handle page unload events
            Unloaded += OnUnloaded;
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

                // Inject the singleton instance of the MediaPlaybackController into the WebView so that
                // it can be accessed through JavaScript.
                var dispatchAdapter = new WinRTAdapter.DispatchAdapter();
                coreWV2.AddHostObjectToScript("mediaPlaybackControllerInstance", dispatchAdapter.WrapObject(MediaPlaybackController.Instance, dispatchAdapter));

                // Set some JavaScript code to run for each page. This sets some properties about how
                // the projected APIs will behave and creates a "mediaPlaybackController" object at the
                // top level to make it easier to access the projected MediaPlaybackController instance.
                await coreWV2.AddScriptToExecuteOnDocumentCreatedAsync(
                "(() => {" +
                    "if (chrome && chrome.webview) {" +
                        "console.log('Setting up WinRT projection options');" +
                        "chrome.webview.hostObjects.options.defaultSyncProxy = true;" +
                        "chrome.webview.hostObjects.options.forceAsyncMethodMatches = [/Async$/,/AsyncWithSpeller$/];" +
                        "chrome.webview.hostObjects.options.ignoreMemberNotFoundError = true;" +
                        "mediaPlaybackController = chrome.webview.hostObjects.sync.mediaPlaybackControllerInstance;" +
                    "}" +
                "})();");

                // Hook up the event handlers before setting the source so none of these events get missed.
                webView.NavigationCompleted += OnNavigationCompleted;

                // This will cause the WebView to navigate to our initial page
                webView.Source = new Uri(initialUri);
            }
            else
            {
                // TODO: Show an error state
                Debug.WriteLine($"Unable to retrieve CoreWebView2");
            }
        }

        /// <summary>
        /// Called when the page is no longer connected to the main object tree.
        /// In this sample (which only has one page to begin with) this generally happens when the app is
        /// entering the background and is dropping its UI to reduce its memory footprint.
        /// </summary>
        private void OnUnloaded(object sender, RoutedEventArgs e)
        {
            // Drop references to the WebView so that it can be garbage collected
            // This allows our app to reduce its memory footprint
            webView.NavigationCompleted -= OnNavigationCompleted;
            webView.Close();
            this.Content = null;
            webView = null;

            GC.Collect();
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
        }
    }
}
