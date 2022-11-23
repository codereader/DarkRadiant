#pragma once

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
 * This schema is also valid when specified in XRC files.
 */
class LocalBitmapArtProvider final :
	public wxArtProvider
{
private:
    std::string _searchPath;

public:
    // Use an absolute file path to the list of search paths this provider is covering
    LocalBitmapArtProvider(const std::string& searchPath) :
        _searchPath(searchPath)
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

        // We listen only to "darkradiant" art IDs
        if (string::starts_with(filename, prefix))
        {
            std::string filePath = _searchPath + filename.substr(prefix.length());

            if (os::fileOrDirExists(filePath)) {
                wxBitmap bm;
                if (bm.LoadFile(filePath)) {
                    return bm;
                }
                else {
                    rError() << "Failed to load bitmap [" << filePath << "]\n";
                }
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
