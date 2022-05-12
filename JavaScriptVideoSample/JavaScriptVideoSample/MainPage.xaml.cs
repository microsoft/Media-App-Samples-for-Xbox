// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics;
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
        private WebView webView;

        /// <summary>
        /// This is the URI that will be loaded when the app launches.
        /// For the purposes of this sample, we're using HTML/JavaScript embedded within the app itself, but
        /// you can just as easily point this to a https web URL where your app code is hosted instead.
        /// </summary>
        private const string initialUri = "ms-appx-web:///WebCode/video-player.html";

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

            // Create the WebView and point it at the initial URL to begin loading the app's UI.
            // The WebView is not added to the XAML page until the NavigationCompleted event fires.
            webView = new WebView();
            webView.NavigationCompleted += OnNavigationCompleted;
            webView.ScriptNotify += OnScriptNotify;
            webView.Navigate(new Uri(initialUri));
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
        /// library from your javascript code and delete the OnScriptNotify handler here, and rely on
        /// Script debugging instead.
        /// </summary>
        /// <param name="sender">The WebView that called window.external.notify()</param>
        /// <param name="args">An object containing the URL of the calling page and the string that was
        /// passed to window.external.notify()</param>
        private void OnScriptNotify(object sender, NotifyEventArgs args)
        {
            Debug.WriteLine(args.Value);
        }
    }
}
