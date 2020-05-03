#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/// ImageLoader implementation using wxImage to load an image from disk
class ImageLoaderWx : public ImageTypeLoader
{
public:

    /// Construct and register wxImage handlers
    ImageLoaderWx();

    // ImageLoader implementation
	ImagePtr load(ArchiveFile& file) const;
	Extensions getExtensions() const;
};

}
