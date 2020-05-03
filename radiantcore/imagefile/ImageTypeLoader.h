#pragma once

#include "iimage.h"

namespace image
{

/// Loader implementation for a particular type of image (e.g TGA, DDS)
class ImageTypeLoader
{
public:
    typedef std::shared_ptr<ImageTypeLoader> Ptr;

	/* greebo: Loads an image from the given ArchiveFile class
	 *
	 * @returns: NULL, if the load failed, the pointer to the image otherwise
	 */
	virtual ImagePtr load(ArchiveFile& file) const = 0;

    typedef std::list<std::string> Extensions;

    /**
     * \brief
     * Return the list of extensions this ImageTypeLoader is capable of
     * loading.
     */
	virtual Extensions getExtensions() const = 0;

	/* greebo: Retrieves the directory prefix needed to construct the
	 * full path to the image (mainly needed for the "dds/" prefix for DDS textures).
	 *
	 * @returns: the lowercase prefix (e.g. "dds/").
	 */
	virtual std::string getPrefix() const { return ""; }
};

}
