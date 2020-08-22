#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/**
 * \brief
 * ImageTypeLoader implementation for JPEG files
 */
class JPEGLoader : public ImageTypeLoader
{
public:
    // ImageTypeLoader implementation
    ImagePtr load(ArchiveFile& file) const override;
    Extensions getExtensions() const override;
};

}
