#ifndef IMAGELOADERMANAGER_H_
#define IMAGELOADERMANAGER_H_

#include <vector>
#include "iimage.h"
#include "modulesystem/modulesmap.h"

namespace shaders {

typedef std::vector<ImageLoader*> ImageLoaderList;

class ImageLoaderManager
{
	ImageLoaderModulesRef _imageLoaders;
public:
	// Constructor, this loads all the available image loaders
	ImageLoaderManager();
	
	// Accessor method containing the static instance of this class
	static ImageLoaderManager& Instance();
	
	ImageLoaderList getLoaders(const std::string& moduleNames);
};

} // namespace shaders

#endif /*IMAGELOADERMANAGER_H_*/
