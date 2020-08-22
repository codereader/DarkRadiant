#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/**
 * \brief
 * ImageTypeLoader implementation for Windows Bitmap (BMP) files
 */
class BMPLoader : public ImageTypeLoader
{
public:
    // ImageTypeLoader implementation
    ImagePtr load(ArchiveFile& file) const override;
    Extensions getExtensions() const override;
};

}
