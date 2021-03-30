#pragma once

#include <wx/artprov.h>
#include "iimage.h"

namespace ui
{

/**
 * Implements wxWidget's ArtProvider interface to allow requesting image files 
 * from the VFS. The schema for these custom ArtIDs is "vfs:path/to/the/image.tga".
 */
class VfsImageArtProvider final :
    public wxArtProvider
{
public:
    VfsImageArtProvider()
    {
        wxArtProvider::Push(this);
    }

    ~VfsImageArtProvider()
    {
        wxArtProvider::Remove(this);
    }

    wxBitmap CreateBitmap(const wxArtID& artId, const wxArtClient& client, const wxSize& size)
    {
        auto id = artId.ToStdString();
        const auto& prefix = ArtIdPrefix();

        // We listen only to "vfs" art IDs
        if (string::starts_with(id, prefix))
        {
            return wxBitmap(CreateImageFromVfsPath(id.substr(prefix.length())));
        }

        return wxNullBitmap;
    }

    static wxImage CreateImageFromVfsPath(const std::string& vfsPath)
    {
        auto image = GlobalImageLoader().imageFromVFS(vfsPath);

        if (!image) return wxNullImage;

        // wxWidgets is expecting separate buffers for RGB and Alpha, split the RGBA data
        auto width = image->getWidth();
        auto height = image->getHeight();
        auto numPixels = width * height;

        // wxImage can take ownership of data allocated with malloc() - maybe use that one
        std::shared_ptr<uint8_t[]> rgb(new uint8_t[numPixels * 3]);
        std::shared_ptr<uint8_t[]> alpha(new uint8_t[numPixels]);

        auto rgba = image->getPixels();

        for (std::size_t h = 0; h < height; ++h)
        {
            for (std::size_t w = 0; w < width; ++w)
            {
                auto pixel = h * width + w;

                rgb[pixel * 3 + 0] = rgba[pixel * 4 + 0];
                rgb[pixel * 3 + 1] = rgba[pixel * 4 + 1];
                rgb[pixel * 3 + 2] = rgba[pixel * 4 + 2];
                alpha[pixel] = rgba[pixel * 4 + 3];
            }
        }

        return wxImage(width, height, rgb.get(), alpha.get(), true).Copy();
    }

    static const std::string& ArtIdPrefix()
    {
        static std::string _artIdPrefix = "vfs:";
        return _artIdPrefix;
    }
};

}
