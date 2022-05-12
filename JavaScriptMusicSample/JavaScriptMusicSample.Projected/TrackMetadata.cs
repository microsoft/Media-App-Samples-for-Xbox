// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using Windows.Foundation.Metadata;
using Windows.Media.Core;
using Windows.Media.Playback;
using Windows.Storage.Streams;

namespace JavaScriptMusicSample.Projected
{
    /// <summary>
    /// This is a helper class to wrap the metadata for a single audio track in a way that it
    /// can be easily exposed to the JavaScript code.
    /// </summary>
    [AllowForWeb]
    public sealed class TrackMetadata
    {
        /// <summary>
        /// Absolute URL to the audio file to play
        /// </summary>
        public string Src
        {
            get;
            set;
        } = string.Empty;

        /// <summary>
        /// The title of the track
        /// </summary>
        public string Title
        {
            get;
            set;
        } = string.Empty;

        /// <summary>
        /// The name of the artist for the track
        /// </summary>
        public string Artist
        {
            get;
            set;
        } = string.Empty;

        /// <summary>
        /// URL to an image that can be used as a thumbnail for the track (such as album art)
        /// </summary>
        public string ThumbnailSrc
        {
            get;
            set;
        } = string.Empty;

        // Note that the constructor of this class cannot be reached by JavaScript. Instead,
        // MediaPlaybackController exposes a helper function to allow the JavaScript code to
        // construct TrackMetadata objects.
        public TrackMetadata(string src, string title, string artist, string thumbnailSrc)
        {
            Src = src;
            Title = title;
            Artist = artist;
            ThumbnailSrc = thumbnailSrc;
        }
    }
}
