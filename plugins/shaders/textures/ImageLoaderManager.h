#ifndef IMAGELOADERMANAGER_H_
#define IMAGELOADERMANAGER_H_

#include <vector>
#include "iimage.h"

namespace shaders {

typedef std::vector<ImageLoaderPtr> ImageLoaderList;

class ImageLoaderManager
{
public:
	/**
	 * greebo: Use this convenience method to retrieve the ImageLoaderModules
	 *         specified in the <moduleNames> argument.
	 * 
	 * @moduleNames: A space-separated list of file extensions (e.g. "TGA DDS GDK")
	 */		
	static ImageLoaderList getLoaders(const std::string& moduleNames);
};

} // namespace shaders

#endif /*IMAGELOADERMANAGER_H_*/
