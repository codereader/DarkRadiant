#pragma once

#include "ImageTypeLoader.h"

namespace image
{

/// ImageTypeLoader implementation for DDS files
class DDSLoader : public ImageTypeLoader
{
public:

    // ImageTypeLoader implementation
	ImagePtr load(ArchiveFile& file) const;

	Extensions getExtensions() const;

	/* greebo: Returns the prefix that is necessary to construct the
	 * path to the dds files.
	 */
	std::string getPrefix() const {
		return "dds/";
	}
};

}
