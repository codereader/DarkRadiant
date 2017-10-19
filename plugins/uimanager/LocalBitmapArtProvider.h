#pragma once

#include "iregistry.h"
#include <wx/artprov.h>
#include <wx/image.h>
#include "string/predicate.h"
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
		std::string filename(id.begin(), id.end()); // convert wxString to std::string

		const std::string& prefix = ArtIdPrefix();

		if (string::starts_with(filename, prefix))
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

	static const std::string& ArtIdPrefix()
	{
		static std::string _artIdPrefix = "darkradiant:";
		return _artIdPrefix;
	}
};

} // namespace
