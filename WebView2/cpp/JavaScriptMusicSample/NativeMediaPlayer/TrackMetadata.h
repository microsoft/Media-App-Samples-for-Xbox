// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "TrackMetadata.g.h"

namespace winrt::NativeMediaPlayer::implementation
{
    struct TrackMetadata : TrackMetadataT<TrackMetadata>
    {
        TrackMetadata() = default;

        TrackMetadata(hstring const& src, hstring const& title, hstring const& artist, hstring const& thumbnailSrc);
        hstring Src();
        void Src(hstring const& value);
        hstring Title();
        void Title(hstring const& value);
        hstring Artist();
        void Artist(hstring const& value);
        hstring ThumbnailSrc();
        void ThumbnailSrc(hstring const& value);

    private:
        hstring src{ L"" };
        hstring title{ L"" };
        hstring artist{ L"" };
        hstring thumbnailSrc{ L"" };
    };
}
namespace winrt::NativeMediaPlayer::factory_implementation
{
    struct TrackMetadata : TrackMetadataT<TrackMetadata, implementation::TrackMetadata>
    {
    };
}
