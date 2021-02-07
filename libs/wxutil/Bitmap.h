#pragma once

#include "LocalBitmapArtProvider.h"
#include "wxutil/Bitmap.h"

namespace wxutil
{

/**
 * \brief
 * Get a wxBitmap from the local "darkradiant" art provider
 *
 * \param name
 * Image file name with no prefix, e.g. "something.png"
 */
inline wxBitmap GetLocalBitmap(const std::string& name)
{
    return wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + name);
}

/**
 * \brief
 * Get a wxBitmap from the local "darkradiant" art provider, passing the given
 * art client along.
 *
 * \param name
 * Image file name with no prefix, e.g. "something.png"
 */
inline wxBitmap GetLocalBitmap(const std::string& name, const wxArtClient& client)
{
    return wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + name, client);
}

}
