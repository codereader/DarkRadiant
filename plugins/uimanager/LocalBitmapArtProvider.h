#pragma once

#include <wx/artprov.h>
#include <wx/image.h>
#include <boost/algorithm/string/predicate.hpp>
#include "os/file.h"

namespace ui
{

/**
 * Implements wxWidget's ArtProvider interface to allow custom stock item IDs for
 * bitmaps used in toolbars and other controls. The schema for these custom ArtIDs
 * is "darkradiant:filename.png" where filename.png is a file in DR's bitmap folder.
 */
class LocalBitmapArtProvider :
	public wxArtProvider
{
public:
	wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size)
	{
		static std::string prefix = "darkradiant:";

		std::string filename(id.begin(), id.end()); // convert wxString to std::string

		if (boost::algorithm::starts_with(filename, prefix))
		{
			std::string filePath = GlobalRegistry().get(RKEY_BITMAPS_PATH) + filename.substr(prefix.length());

			if (os::fileOrDirExists(filePath))
			{
				wxImage img(filePath);
				return wxBitmap(img);
			}
		}

		return wxNullBitmap;
	 }
};

} // namespace
