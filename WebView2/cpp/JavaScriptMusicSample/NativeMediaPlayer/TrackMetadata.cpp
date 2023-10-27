// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "TrackMetadata.h"
#include "TrackMetadata.g.cpp"

namespace winrt::NativeMediaPlayer::implementation
{
    TrackMetadata::TrackMetadata(
        hstring const& src,
        hstring const& title,
        hstring const& artist,
        hstring const& thumbnailSrc) :
        src {src},
        title {title},
        artist {artist},
        thumbnailSrc {thumbnailSrc}
    { }
    hstring TrackMetadata::Src()
    {
        return src;
    }
    void TrackMetadata::Src(hstring const& value)
    {
        src = value;
    }
    hstring TrackMetadata::Title()
    {
        return title;
    }
    void TrackMetadata::Title(hstring const& value)
    {
        title = value;
    }
    hstring TrackMetadata::Artist()
    {
        return artist;
    }
    void TrackMetadata::Artist(hstring const& value)
    {
        artist = value;
    }
    hstring TrackMetadata::ThumbnailSrc()
    {
        return thumbnailSrc;
    }
    void TrackMetadata::ThumbnailSrc(hstring const& value)
    {
        thumbnailSrc = value;
    }
}
