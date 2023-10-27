// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "PlaylistDataFetcher.g.h"

namespace winrt::NativeMediaPlayer::implementation
{
    struct PlaylistDataFetcher : PlaylistDataFetcherT<PlaylistDataFetcher>
    {
    public:
        PlaylistDataFetcher() = delete;

        static winrt::Windows::Foundation::IAsyncOperation<hstring> GetPlaylistTracks(hstring playlistId);
        static hstring GetUriFromTrackId(hstring const& trackId);

    private:
        static winrt::Windows::Foundation::IAsyncOperation<hstring> FetchStringFromUri(winrt::Windows::Foundation::Uri uri);
    };
}
namespace winrt::NativeMediaPlayer::factory_implementation
{
    struct PlaylistDataFetcher : PlaylistDataFetcherT<PlaylistDataFetcher, implementation::PlaylistDataFetcher>
    {
    };
}
