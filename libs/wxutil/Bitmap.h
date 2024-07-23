#pragma once

#include <wx/artprov.h>
#include <wx/bmpbuttn.h>

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
    return wxArtProvider::GetBitmap("darkradiant:" + name);
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
    return wxArtProvider::GetBitmap("darkradiant:" + name, client);
}

/// Construct and return a wxBitmapButton displaying the given local icon
inline wxBitmapButton* IconButton(wxWindow* parent, const std::string& iconFile)
{
    return new wxBitmapButton(parent, wxID_ANY, GetLocalBitmap(iconFile));
}

}
