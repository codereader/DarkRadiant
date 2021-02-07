#pragma once

#include "imodule.h"
#include <wx/artprov.h>
#include <wx/image.h>
#include "string/predicate.h"
#include "os/file.h"

namespace wxutil
{

/**
 * Implements wxWidget's ArtProvider interface to allow custom stock item IDs for
 * bitmaps used in toolbars and other controls. The schema for these custom ArtIDs
 * is "darkradiant:filename.png" where filename.png is a file in DR's bitmap folder.
 */
class LocalBitmapArtProvider final :
	public wxArtProvider
{
public:
    LocalBitmapArtProvider()
    {
        wxArtProvider::Push(this);
    }

    ~LocalBitmapArtProvider()
    {
        wxArtProvider::Remove(this);
    }

	wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size)
	{
        auto filename = id.ToStdString();
		const auto& prefix = ArtIdPrefix();

		if (string::starts_with(filename, prefix))
		{
			const auto& ctx = module::GlobalModuleRegistry().getApplicationContext();
			std::string filePath = ctx.getBitmapsPath() + filename.substr(prefix.length());

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
