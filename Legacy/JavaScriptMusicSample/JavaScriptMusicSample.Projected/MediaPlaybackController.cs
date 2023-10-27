// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.Media.Core;
using Windows.Media.Playback;
using Windows.Storage.Streams;

namespace JavaScriptMusicSample.Projected
{
    /// <summary>
    /// This singleton object wraps a MediaPlayer in a way that it can be projected into
    /// JavaScript.
    /// 
    /// The [AllowForWeb] attribute below is what marks this class as being projectable, though
    /// also note that it must reside in a Windows Runtime Component separate from the main app
    /// (which in this sample is called JavaScriptMusicSample.Projected).
    /// 
    /// MainPage.Xaml.cs injects the singleton instance of this class into the WebView using
    /// AddWebAllowedObject whenever the NavigationStarting event is fired.
    /// </summary>
    [AllowForWeb]
    public sealed class MediaPlaybackController
    {
        // This class is a singleton
        private static MediaPlaybackController instance;
        public static MediaPlaybackController Instance => instance ?? (instance = new MediaPlaybackController());

        // This is the Windows object responsible for managing playback
        private readonly MediaPlayer player;

        // The list of MediaPlaybackItems that is currently being played by the player
        private MediaPlaybackList playbackList = null;

        // Because this class is a singleton, its constructor is marked as private
        private MediaPlaybackController()
        {
            player = new MediaPlayer();
            player.Volume = .1; // Set default volume low

            player.PlaybackSession.PositionChanged += (sender, args) => TimeUpdate?.Invoke(this, null);
            player.PlaybackSession.PlaybackStateChanged += (sender, args) => PlaybackUpdate?.Invoke(this, null);
            player.SourceChanged += (sender, args) => SourceUpdate?.Invoke(this, null);
        }

        // The list of songs in the current playlist
        public TrackMetadata[] CurrentPlaylist { get; private set; } = new TrackMetadata[0];

        // The JavaScript code can check this property to retrieve the metadata for the currently playing track
        public TrackMetadata CurrentTrack =>
            (CurrentPlaylist != null && playbackList != null && playbackList.CurrentItemIndex < CurrentPlaylist.Length) ?
                CurrentPlaylist[playbackList.CurrentItemIndex] :
                null;

        // The index of the currently playing track in the CurrentPlaylist, or -1 if no track is currently playing
        public int CurrentTrackIndex => (int?)playbackList?.CurrentItemIndex ?? -1;

        // Whether or not the media player is in the paused state
        public bool Paused => player.PlaybackSession.PlaybackState == MediaPlaybackState.Paused;

        // Whether or not the media player has played the current source all the way through to completion
        public bool Ended => player.PlaybackSession.Position.Equals(player.PlaybackSession.NaturalDuration);

        // Whether or not the media player is currently muted
        public bool Muted
        {
            get => player.IsMuted;
            set => player.IsMuted = value;
        }

        // The current volume of the media player, a number between 0 and 1
        public double Volume
        {
            get => player.Volume;
            set => player.Volume = value;
        }

        // The current playback position of the media player in its current track
        public double CurrentTime
        {
            get => player.PlaybackSession.Position.TotalSeconds;
            set => player.PlaybackSession.Position = TimeSpan.FromSeconds(value);
        }

        // The total duration of the track currently being played by the media player
        public double Duration => player.PlaybackSession.NaturalDuration.TotalSeconds;

        // Causes the media player to start playback
        public void Play() => player.Play();

        // Causes the media player to pause playback
        public void Pause() => player.Pause();

        // Switches to the previous track, if able
        public void SkipPrevious() => playbackList?.MovePrevious();

        // Switches to the next track, if able                                                                                                                                                           
        public void SkipNext() => playbackList?.MoveNext();

        // Allows the JavaScript code to construct a TrackMetadata object.
        // Note that it cannot call the TrackMetadata constructor directly, which is why it is exposed
        // as a helper function off of a class that the code already has access to.
        public TrackMetadata CreateTrack(string src, string title, string artist, string thumbnailSrc)
        {
            return new TrackMetadata(src, title, artist, thumbnailSrc);
        }

        /// <summary>
        /// Plays a particular track in a playlist.
        /// 
        /// Note that because this public function is exposed in a Windows Runtime Component, it
        /// cannot return Task. Instead, we use an internal function which returns a Task and call
        /// AsAsyncAction() on it.
        /// </summary>
        /// <param name="playlistId">The ID of the playlist that contains the track.</param>
        /// <param name="trackId">
        /// The ID of the track to start the playlist on.
        /// If left null, the first track in the playlist will be used.
        /// </param>
        public IAsyncAction PlayTrack(string playlistId, string trackId)
        {
            return PlayTrackAsync(playlistId, trackId).AsAsyncAction();
        }

        /// <summary>
        /// Plays a particular playlist from the beginning. See also PlayTrack().
        /// </summary>
        /// <param name="playlistId">The ID of the playlist to play.</param>
        public IAsyncAction PlayPlaylist(string playlistId)
        {
            return PlayTrack(playlistId, null);
        }

        /// <summary>
        /// Internal implementation of playing a particular track in a playlist.
        /// 
        /// Note that in the process, this constructs a new MediaPlaybackItemList containing
        /// *all* of the tracks in the associated playlist and sets it as the Source on the
        /// MediaPlayer. This allows the MediaPlayer to keep playing through the playlist after
        /// the selected track completes, and allows it to handle presses of the next/previous
        /// track buttons.
        /// </summary>
        private async Task PlayTrackAsync(string playlistId, string trackId)
        {
            // Remove event listeners from the old list
            if (playbackList != null)
            {
                playbackList.CurrentItemChanged -= OnCurrentPlaybackItemChanged;
            }

            // Fetch the JSON data describing the requested playlist
            string trackDataString = await PlaylistDataFetcher.GetPlaylistTracks(playlistId);
            JsonObject trackData = JsonObject.Parse(trackDataString);
            JsonArray trackListJson = trackData["Tracks"].GetArray();
            uint initialTrackIdx = 0;

            CurrentPlaylist = new TrackMetadata[trackListJson.Count];
            playbackList = new MediaPlaybackList();

            // Create a TrackMetadata and MediaPlaybackItem from each track
            for (int i = 0; i < trackListJson.Count; i++)
            {
                JsonObject trackJson = trackListJson[i].GetObject();

                if (trackId != null && trackJson["Id"].GetString().Equals(trackId))
                {
                    initialTrackIdx = (uint)i;
                }

                CurrentPlaylist[i] = CreateTrackMetadataFromJson(trackJson);
                playbackList.Items.Add(CreatePlaybackItemFromMetadata(CurrentPlaylist[i]));
            }

            // Register for event callbacks when the current item changes
            playbackList.CurrentItemChanged += OnCurrentPlaybackItemChanged;

            // Update the player's current source to draw from the new list
            player.Source = playbackList;

            // Move to the specified track's index, if any
            // This can only be called after the list is set as the MediaPlayer's Source
            playbackList.MoveTo(initialTrackIdx);
        }

        /// <summary>
        /// Constructs a TrackMetadata object from the provided JSON which can be passed to
        /// the JavaScript code so it can query information about the currently playing track.
        /// </summary>
        private TrackMetadata CreateTrackMetadataFromJson(JsonObject json)
        {
            return new TrackMetadata(
                src: PlaylistDataFetcher.GetUriFromTrackId(json["Id"].GetString()),
                title: json["Title"].GetString(),
                artist: json["Artist"].GetString(),
                thumbnailSrc: json["Image"].GetString()
                );
        }

        /// <summary>
        /// Constructs a MediaPlaybackItem from the provided metadata, representing a single
        /// track. The MediaPlaybackItem can then be provided to MediaPlayer.Source to cause
        /// it to play it as a single track, or added to a MediaPlaybackItemList if you want
        /// to play a list of tracks.
        /// </summary>
        private MediaPlaybackItem CreatePlaybackItemFromMetadata(TrackMetadata track)
        {
            MediaSource source = MediaSource.CreateFromUri(new Uri(track.Src));
            MediaPlaybackItem playbackItem = new MediaPlaybackItem(source);

            // This is where the display properties are set.
            // Other MusicProperties exist, if you want to provide them.
            MediaItemDisplayProperties props = playbackItem.GetDisplayProperties();
            props.Type = Windows.Media.MediaPlaybackType.Music;
            props.MusicProperties.Title = track.Title;
            props.MusicProperties.Artist = track.Artist;

            // Fetch the thumbnail as a RandomAccessStreamReference
            if (!string.IsNullOrEmpty(track.ThumbnailSrc))
            {
                props.Thumbnail = RandomAccessStreamReference.CreateFromUri(new Uri(track.ThumbnailSrc));
            }

            // Add the modified properties back to the playbackItem
            playbackItem.ApplyDisplayProperties(props);

            return playbackItem;
        }

        /// <summary>
        /// This is called whenever the MediaPlayer is playing a list of tracks, and playback
        /// switches to another track within that list.
        /// </summary>
        private void OnCurrentPlaybackItemChanged(MediaPlaybackList sender, CurrentMediaPlaybackItemChangedEventArgs args)
        {
            // For the purposes of this sample, the JavaScript code does not need to distinguish between
            // the MediaPlayer's Source list changing completely and an individual track changing in the
            // current playbackList.
            // If it did, we could introduce a new event to fire in this instance instead of reusing
            // SourceUpdate here.
            SourceUpdate?.Invoke(this, null);
        }

        // Callback to let the JavaScript code know when to update its progress bar
        public event EventHandler<object> TimeUpdate;

        // Callback to let the JavaScript code know that the play state has changed
        public event EventHandler<object> PlaybackUpdate;

        // Callback to let the JavaScript code know that the current track has changed
        public event EventHandler<object> SourceUpdate;
    }
}
