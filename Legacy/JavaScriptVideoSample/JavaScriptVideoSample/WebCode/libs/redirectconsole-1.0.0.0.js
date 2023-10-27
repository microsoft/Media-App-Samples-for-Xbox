// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This library redirects JavaScript console function calls to also pass their string
// payload along to window.external.notify, so that a UWP application can catch and log
// them to the Visual Studio console. Note that line numbers are lost in this process.
//
// This is useful for Visual Studio 2019 and later, as they lack built-in debugging
// tools for the original WebView control. If you are using Visual Studio 2017, you are
// better off using the built-in Script debugging tools instead.
(function () {
    "use strict";

    if (typeof (window.external) !== "undefined" && ("notify" in window.external))
    {
        let oldLog = window.console.log;
        window.console.log = function (text) {
            window.external.notify("  [JS Console Log] " + text);
            oldLog.apply(window.console, arguments);
        };
    
        let oldInfo = window.console.info;
        window.console.info = function (text) {
            window.external.notify(" [JS Console Info] " + text);
            oldInfo.apply(window.console, arguments);
        };
    
        let oldWarn = window.console.warn;
        window.console.warn = function (text) {
            window.external.notify(" [JS Console Warn] " + text);
            oldWarn.apply(window.console, arguments);
        };
    
        let oldError = window.console.error;
        window.console.error = function (text) {
            window.external.notify("[JS Console Error] " + text);
            oldError.apply(window.console, arguments);
        }

        window.onerror = function (text) {
            window.external.notify("[JS Console Error] " + text);
        }
    }
})();