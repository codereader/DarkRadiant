#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/**
 * \brief
 * ImageTypeLoader implementation for TGA files
 *
 * We can't use the wxWidgets image loader because it is buggy and cannot
 * handle RLE-compressed TGA files.
 */
class TGALoader : public ImageTypeLoader
{
public:

    // ImageTypeLoader implementation
	ImagePtr load(ArchiveFile& file) const;
	Extensions getExtensions() const;
};

}
