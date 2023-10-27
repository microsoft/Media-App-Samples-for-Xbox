// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Net;
using System.Threading.Tasks;
using Windows.Storage;

namespace NativeMediaPlayer
{
    /// <summary>
    /// This class is responsible for retrieving playlist JSON data for use by the MediaPlaybackController.
    /// 
    /// In this sample, playlists are stored in static json files inside the app package, but they could
    /// easily be pointed at web URLs which allow a server to construct them dynamically instead.
    /// </summary>
    static class PlaylistDataFetcher
    {
        /// <summary>
        /// Returns JSON data describing all tracks in a particular playlist. See the playlistdata folder
        /// in the main project for an example.
        /// For this sample, the Id is simply its filename.
        /// </summary>
        /// <param name="playlistId">A unique identifier for the playlist to describe.</param>
        public static async Task<string> GetPlaylistTracks(string playlistId)
        {
            Uri uri = new Uri($"ms-appx:///WebCode/playlistdata/{playlistId}.json");
            return await FetchStringFromUri(uri);
        }

        /// <summary>
        /// Constructs a URI to a particular track, given its Id.
        /// For this sample, the Id is simply its filename.
        /// </summary>
        /// <param name="trackId">A unique identifier for the track.</param>
        /// <returns>A fully-qualified URI that can be used to access the track.</returns>
        public static string GetUriFromTrackId(string trackId)
        {
            return $"ms-appx:///WebCode/music/{trackId}.mp3";
        }

        /// <summary>
        /// Helper function to retrieve the contents of a text file either from local storage or the web.
        /// </summary>
        /// <param name="uri">URI to the file to retrieve.</param>
        /// <returns>A string containing the contents for the file.</returns>
        private static async Task<string> FetchStringFromUri(Uri uri)
        {
            if (uri.Scheme.Equals("ms-appx"))
            {
                StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(uri);
                return await FileIO.ReadTextAsync(file);
            }
            else
            {
                using (WebClient wc = new WebClient())
                {
                    return await wc.DownloadStringTaskAsync(uri);
                }
            }
        }
    }
}
