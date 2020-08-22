#pragma once

#include "iimage.h"
#include "ImageTypeLoader.h"

#include <map>

namespace image
{

/// ImageLoader implementing module
class ImageLoader: 
    public IImageLoader
{
private:
    // Map of image extension to loader class. Multiple image extensions may
    // map to the same loader class.
    typedef std::map<std::string, ImageTypeLoader::Ptr> LoadersByExtension;
    LoadersByExtension _loadersByExtension;

    ImageTypeLoader::Extensions _extensions;

private:
    void addLoaderToMap(const ImageTypeLoader::Ptr& loader);

public:

    // Construct and initialise loaders
    ImageLoader();

    // ImageLoader implementation
    ImagePtr imageFromVFS(const std::string& vfsPath) const override;
	ImagePtr imageFromFile(const std::string& filename) const override;

    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext&) override;
};

}
