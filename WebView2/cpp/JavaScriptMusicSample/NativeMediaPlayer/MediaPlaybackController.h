// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "MediaPlaybackController.g.h"

namespace winrt::NativeMediaPlayer::implementation
{
    struct MediaPlaybackController : MediaPlaybackControllerT<MediaPlaybackController>
    {
    public:
        MediaPlaybackController();

        winrt::Windows::Foundation::Collections::IVector<winrt::NativeMediaPlayer::TrackMetadata> CurrentPlaylist();
        winrt::NativeMediaPlayer::TrackMetadata CurrentTrack();
        uint32_t CurrentTrackIndex();
        bool Paused();
        bool Ended();
        bool Muted();
        void Muted(bool value);
        double Volume();
        void Volume(double value);
        double CurrentTime();
        void CurrentTime(double value);
        double Duration();
        void Play();
        void Pause();
        void SkipPrevious();
        void SkipNext();
        winrt::Windows::Foundation::IAsyncAction PlayTrackAsync(hstring playlistId, hstring trackId);
        winrt::Windows::Foundation::IAsyncAction PlayPlaylistAsync(hstring playlistId);
        winrt::event_token TimeUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable> const& handler);
        void TimeUpdate(winrt::event_token const& token) noexcept;
        winrt::event_token PlaybackUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable> const& handler);
        void PlaybackUpdate(winrt::event_token const& token) noexcept;
        winrt::event_token SourceUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::NativeMediaPlayer::TrackMetadata> const& handler);
        void SourceUpdate(winrt::event_token const& token) noexcept;
    private:
        // The dispatcher for the thread the MediaPlaybackController was created on.
        // In this sample, it is expected to be the UI thread.
        // This allows us to marshal calls that will be directed into the WebView2 onto the
        // UI thread. For more information, see:
        // https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/threading-model
        winrt::Windows::UI::Core::CoreDispatcher dispatcher{ nullptr };

        // This is the Windows object responsible for managing playback
        winrt::Windows::Media::Playback::MediaPlayer player{ };

        // The list of MediaPlaybackItems that is currently being played by the player
        winrt::Windows::Media::Playback::MediaPlaybackList playbackList{ nullptr };

        winrt::event_token playbackListItemChangedToken{};
        winrt::Windows::Foundation::Collections::IVector<winrt::NativeMediaPlayer::TrackMetadata> currentPlaylist{ winrt::single_threaded_vector<winrt::NativeMediaPlayer::TrackMetadata>() };
        uint32_t currentTrackIndex{ 0 };
        winrt::event<Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable>> timeUpdateEvent;
        winrt::event<Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable>> playbackUpdateEvent;
        winrt::event<Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::NativeMediaPlayer::TrackMetadata>> sourceUpdateEvent;

        winrt::Windows::Foundation::IAsyncAction PlayTrackInternalAsync(winrt::hstring playlistId, winrt::hstring trackId);
        winrt::NativeMediaPlayer::TrackMetadata CreateTrackMetadataFromJson(winrt::Windows::Data::Json::JsonObject const& json);
        winrt::Windows::Media::Playback::MediaPlaybackItem CreatePlaybackItemFromMetadata(winrt::NativeMediaPlayer::TrackMetadata const& track);
        winrt::fire_and_forget OnPlayerPositionChanged(winrt::Windows::Media::Playback::MediaPlaybackSession sender, IInspectable args);
        winrt::fire_and_forget OnPlayerPlaybackStateChanged(winrt::Windows::Media::Playback::MediaPlaybackSession sender, IInspectable args);
        winrt::fire_and_forget OnPlayerSourceChanged(winrt::Windows::Media::Playback::MediaPlayer sender, IInspectable args);
        winrt::fire_and_forget OnCurrentPlaybackItemChanged(winrt::Windows::Media::Playback::MediaPlaybackList sender, winrt::Windows::Media::Playback::CurrentMediaPlaybackItemChangedEventArgs args);
    };
}
namespace winrt::NativeMediaPlayer::factory_implementation
{
    struct MediaPlaybackController : MediaPlaybackControllerT<MediaPlaybackController, implementation::MediaPlaybackController>
    {
    };
}
