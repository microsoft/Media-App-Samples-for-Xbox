// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MediaPlaybackController.h"
#include "MediaPlaybackController.g.cpp"
#include "TrackMetadata.h"
#include "TrackMetadata.g.h"
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::Media::Core;
using namespace winrt::Windows::Media::Playback;
using namespace winrt::Windows::Storage::Streams;

namespace winrt::NativeMediaPlayer::implementation
{
    MediaPlaybackController::MediaPlaybackController()
    {
        dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();

        player.Volume(.1); // Set default volume low

        player.PlaybackSession().PositionChanged({ this, &MediaPlaybackController::OnPlayerPositionChanged });
        player.PlaybackSession().PlaybackStateChanged({ this, &MediaPlaybackController::OnPlayerPlaybackStateChanged });
        player.SourceChanged({ this, &MediaPlaybackController::OnPlayerSourceChanged });
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::NativeMediaPlayer::TrackMetadata> MediaPlaybackController::CurrentPlaylist()
    {
        return currentPlaylist;
    }
    winrt::NativeMediaPlayer::TrackMetadata MediaPlaybackController::CurrentTrack()
    {
        if (currentTrackIndex >= 0 && currentTrackIndex < currentPlaylist.Size())
        {
            return currentPlaylist.GetAt(currentTrackIndex);
        }
        
        return nullptr;
    }
    uint32_t MediaPlaybackController::CurrentTrackIndex()
    {
        return currentTrackIndex;
    }
    bool MediaPlaybackController::Paused()
    {
        return player.PlaybackSession().PlaybackState() == MediaPlaybackState::Paused;
    }
    bool MediaPlaybackController::Ended()
    {
        return player.PlaybackSession().Position() == player.PlaybackSession().NaturalDuration();
    }
    bool MediaPlaybackController::Muted()
    {
        return player.IsMuted();
    }
    void MediaPlaybackController::Muted(bool value)
    {
        player.IsMuted(value);
    }
    double MediaPlaybackController::Volume()
    {
        return player.Volume();
    }
    void MediaPlaybackController::Volume(double value)
    {
        player.Volume(value);
    }
    double MediaPlaybackController::CurrentTime()
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(player.PlaybackSession().Position()).count();
    }
    void MediaPlaybackController::CurrentTime(double value)
    {
        player.PlaybackSession().Position(std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>(value)));
    }
    double MediaPlaybackController::Duration()
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(player.PlaybackSession().NaturalDuration()).count();
    }
    void MediaPlaybackController::Play()
    {
        player.Play();
    }
    void MediaPlaybackController::Pause()
    {
        player.Pause();
    }
    void MediaPlaybackController::SkipPrevious()
    {
        if (playbackList)
        {
            playbackList.MovePrevious();
        }
    }
    void MediaPlaybackController::SkipNext()
    {
        if (playbackList)
        {
            playbackList.MoveNext();
        }
    }
    winrt::Windows::Foundation::IAsyncAction MediaPlaybackController::PlayTrackAsync(hstring playlistId, hstring trackId)
    {
        return PlayTrackInternalAsync(playlistId, trackId);
    }
    winrt::Windows::Foundation::IAsyncAction MediaPlaybackController::PlayPlaylistAsync(hstring playlistId)
    {
        return PlayTrackAsync(playlistId, L"");
    }
    winrt::event_token MediaPlaybackController::TimeUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable> const& handler)
    {
        return timeUpdateEvent.add(handler);
    }
    void MediaPlaybackController::TimeUpdate(winrt::event_token const& token) noexcept
    {
        timeUpdateEvent.remove(token);
    }
    winrt::event_token MediaPlaybackController::PlaybackUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::Windows::Foundation::IInspectable> const& handler)
    {
        return playbackUpdateEvent.add(handler);
    }
    void MediaPlaybackController::PlaybackUpdate(winrt::event_token const& token) noexcept
    {
        playbackUpdateEvent.remove(token);
    }
    winrt::event_token MediaPlaybackController::SourceUpdate(winrt::Windows::Foundation::TypedEventHandler<winrt::NativeMediaPlayer::MediaPlaybackController, winrt::NativeMediaPlayer::TrackMetadata> const& handler)
    {
        return sourceUpdateEvent.add(handler);
    }
    void MediaPlaybackController::SourceUpdate(winrt::event_token const& token) noexcept
    {
        sourceUpdateEvent.remove(token);
    }

    IAsyncAction MediaPlaybackController::PlayTrackInternalAsync(hstring playlistId, hstring trackId)
    {
        // Remove event listeners from the old list
        if (playbackList)
        {
            playbackList.CurrentItemChanged(playbackListItemChangedToken);
        }

        // Fetch the JSON data describing the requested playlist
        hstring trackDataString{ co_await PlaylistDataFetcher::GetPlaylistTracks(playlistId) };
        JsonObject trackData{ JsonObject::Parse(trackDataString) };
        JsonArray trackListJson{ trackData.GetNamedArray(L"Tracks") };
        uint32_t initialTrackIdx{ 0 };

        currentPlaylist.Clear();
        playbackList = MediaPlaybackList();

        // Create a TrackMetadata and MediaPlaybackItem from each track
        for (uint32_t i = 0; i < trackListJson.Size(); i++)
        {
            JsonObject trackJson{ trackListJson.GetObjectAt(i) };

            if (!trackId.empty() && trackJson.GetNamedString(L"Id", L"") == trackId)
            {
                initialTrackIdx = i;
            }

            NativeMediaPlayer::TrackMetadata metadata{ CreateTrackMetadataFromJson(trackJson) };
            currentPlaylist.Append(metadata);
            playbackList.Items().Append(CreatePlaybackItemFromMetadata(metadata));
        }

        // Register for event callbacks when the current item changes
        playbackList.CurrentItemChanged({ this, &MediaPlaybackController::OnCurrentPlaybackItemChanged });

        // Keep track of the current track. Note that playbackList.CurrentItemIndex
        // updates asynchronously--it may not be set by the time this function returns--
        // so we keep track of the intended index locally so we can provide a consistent
        // experience for the JavaScript code.
        currentTrackIndex = initialTrackIdx;

        // Update the player's current source to draw from the new list
        player.Source(playbackList);

        // Move to the specified track's index, if any
        // This can only be called after the list is set as the MediaPlayer's Source
        playbackList.MoveTo(initialTrackIdx);
    }

    NativeMediaPlayer::TrackMetadata MediaPlaybackController::CreateTrackMetadataFromJson(JsonObject const& json)
    {
        return NativeMediaPlayer::TrackMetadata(
            PlaylistDataFetcher::GetUriFromTrackId(json.GetNamedString(L"Id", L"")),
            json.GetNamedString(L"Title", L""),
            json.GetNamedString(L"Artist", L""),
            json.GetNamedString(L"Image", L"")
        );
    }

    MediaPlaybackItem MediaPlaybackController::CreatePlaybackItemFromMetadata(NativeMediaPlayer::TrackMetadata const& track)
    {
        MediaSource source{ MediaSource::CreateFromUri(Uri(track.Src())) };
        MediaPlaybackItem playbackItem{ source };

        // This is where the display properties are set.
        // Other MusicProperties exist, if you want to provide them.
        MediaItemDisplayProperties props{ playbackItem.GetDisplayProperties() };
        props.Type(Windows::Media::MediaPlaybackType::Music);
        props.MusicProperties().Title(track.Title());
        props.MusicProperties().Artist(track.Artist());

        // Fetch the thumbnail as a RandomAccessStreamReference
        hstring thumbnailSrc = track.ThumbnailSrc();
        if (!thumbnailSrc.empty())
        {
            props.Thumbnail(RandomAccessStreamReference::CreateFromUri(Uri(thumbnailSrc)));
        }

        // Add the modified properties back to the playbackItem
        playbackItem.ApplyDisplayProperties(props);

        return playbackItem;
    }

    fire_and_forget MediaPlaybackController::OnPlayerPositionChanged(MediaPlaybackSession sender, IInspectable args)
    {
        // Marshal back to the UI thread before firing an event the JavaScript may be
        // listening to
        co_await dispatcher;
        timeUpdateEvent(*this, nullptr);
    }

    fire_and_forget MediaPlaybackController::OnPlayerPlaybackStateChanged(MediaPlaybackSession sender, IInspectable args)
    {
        // Marshal back to the UI thread before firing an event the JavaScript may be
        // listening to
        co_await dispatcher;
        playbackUpdateEvent(*this, nullptr);
    }

    fire_and_forget MediaPlaybackController::OnPlayerSourceChanged(MediaPlayer sender, IInspectable args)
    {
        // Marshal back to the UI thread before firing an event the JavaScript may be
        // listening to
        co_await dispatcher;
        sourceUpdateEvent(*this, nullptr);
    }

    fire_and_forget MediaPlaybackController::OnCurrentPlaybackItemChanged(MediaPlaybackList sender, CurrentMediaPlaybackItemChangedEventArgs args)
    {
        // Marshal back to the UI thread before firing an event the JavaScript may be
        // listening to
        co_await dispatcher;
        currentTrackIndex = sender.CurrentItemIndex();

        // For the purposes of this sample, the JavaScript code does not need to distinguish between
        // the MediaPlayer's Source list changing completely and an individual track changing in the
        // current playbackList.
        // If it did, we could introduce a new event to fire in this instance instead of reusing
        // SourceUpdate here.
        sourceUpdateEvent(*this, CurrentTrack());
    }
}
