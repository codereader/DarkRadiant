#ifndef FILELODER_H_
#define FILELODER_H_

#include "iimage.h"
#include "ImageLoaderManager.h"

namespace shaders
{

/**
 * \brief
 * Helper class which provides methods to load an image from either an absolute
 * pathname, or a VFS path.
 */
class ImageFileLoader
{
private:

	// Get the list of ImageLoaders associated with the .game file formats
	static const ImageLoaderList& getImageLoaders();

public:

    /**
     * \brief
     * Load an image from a VFS path.
     */
    static ImagePtr imageFromVFS(const std::string& vfsPath);

	/**
     * \brief
     * Load an image from a filesystem path.
     */
	static ImagePtr imageFromFile(const std::string& filename,
                                  const std::string& modules = "GDK");
};

} // namespace shaders

#endif /*FILELODER_H_*/
