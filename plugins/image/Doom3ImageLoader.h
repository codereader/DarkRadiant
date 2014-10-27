#pragma once

#include "iimage.h"
#include "ImageTypeLoader.h"

#include <map>

namespace image
{

/// ImageLoader implementing module
class Doom3ImageLoader: public ImageLoader
{
    // Map of image extension to loader class. Multiple image extensions may
    // map to the same loader class.
    typedef std::map<std::string, ImageTypeLoader::Ptr> LoadersByExtension;
    LoadersByExtension _loadersByExtension;

private:
    void addLoaderToMap(ImageTypeLoader::Ptr loader);

public:

    // Construct and initialise loaders
    Doom3ImageLoader();

    // ImageLoader implementation
    ImagePtr imageFromVFS(const std::string& vfsPath) const;
	ImagePtr imageFromFile(const std::string& filename) const;

    // RegisterableModule implementation
    const std::string& getName() const;
    const StringSet& getDependencies() const;
    void initialiseModule(const ApplicationContext&) { }
};

}
