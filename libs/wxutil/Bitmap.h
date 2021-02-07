#pragma once

#include "iuimanager.h"

#include <wx/artprov.h>

namespace wxutil
{

/**
 * \brief
 * Get a wxBitmap from the art provider
 *
 * \param name
 * Image file name with no prefix, e.g. "something.png"
 */
inline wxBitmap GetBitmap(const std::string& name)
{
    return wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + name);
}

}
