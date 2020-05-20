#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/**
 * \brief
 * ImageTypeLoader implementation for PNG files
 */
class PNGLoader : public ImageTypeLoader
{
public:
    // ImageTypeLoader implementation
    ImagePtr load(ArchiveFile& file) const override;
    Extensions getExtensions() const override;
};

}
