// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistDataFetcher.h"
#include "PlaylistDataFetcher.g.cpp"
#include <sstream>
#include <winrt/Windows.Storage.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Web::Http;

namespace winrt::NativeMediaPlayer::implementation
{
    winrt::Windows::Foundation::IAsyncOperation<hstring> PlaylistDataFetcher::GetPlaylistTracks(hstring playlistId)
    {
        std::wostringstream strStream{};
        strStream << L"ms-appx:///WebCode/playlistdata/" << playlistId.c_str() << L".json";
        Uri uri{ strStream.str() };
        hstring str{ co_await FetchStringFromUri(uri) };
        co_return str;
    }
    hstring PlaylistDataFetcher::GetUriFromTrackId(hstring const& trackId)
    {
        std::wostringstream strStream{};
        strStream << L"ms-appx:///WebCode/music/" << trackId.c_str() << L".mp3";
        return strStream.str().c_str();
    }

    /// <summary>
    /// Helper function to retrieve the contents of a text file either from local storage or the web.
    /// </summary>
    /// <param name="uri">URI to the file to retrieve.</param>
    /// <returns>A string containing the contents for the file.</returns>
    IAsyncOperation<hstring> PlaylistDataFetcher::FetchStringFromUri(Uri uri)
    {
        if (uri.SchemeName() == L"ms-appx")
        {
            StorageFile file{ co_await StorageFile::GetFileFromApplicationUriAsync(uri) };
            hstring text{ co_await FileIO::ReadTextAsync(file) };
            co_return text;
        }
        else
        {
            // It is best to avoid re-creating an HttpClient every time a new request is made, for performance.
            // Depending on your scenario, you may need to cusomize the HttpClient's settings further.
            // For more details, see:
            // https://learn.microsoft.com/en-us/dotnet/fundamentals/networking/http/httpclient-guidelines
            static const HttpClient httpClient{};

            HttpRequestMessage getRequest{ HttpMethod::Get(), uri };
            HttpResponseMessage response{ co_await httpClient.SendRequestAsync(getRequest) };
            if (response.IsSuccessStatusCode())
            {
                hstring text{ co_await response.Content().ReadAsStringAsync() };
                co_return text;
            }
            else
            {
                hstring errStr = L"Failed to retrieve string from URI: " + uri.ToString() + L" Failure Code: " + to_hstring(static_cast<int>(response.StatusCode()));
                OutputDebugString(errStr.c_str());
                co_return L"";
            }
        }
    }
}
