#include "ImageLoader.h"
#include "TGALoader.h"
#include "JPEGLoader.h"
#include "BMPLoader.h"
#include "PNGLoader.h"
#include "dds.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "iregistry.h"
#include "igame.h"

#include "string/case_conv.h"

#include "os/path.h"
#include "DirectoryArchiveFile.h"
#include "module/StaticModule.h"

namespace image
{

namespace
{
    // Registry key holding texture types
    const char* const GKEY_IMAGE_TYPES = "/filetypes/texture//extension";
}

void ImageLoader::addLoaderToMap(const ImageTypeLoader::Ptr& loader)
{
    auto extensions = loader->getExtensions();

    for (const auto& extension : extensions)
    {
        _loadersByExtension.emplace(string::to_lower_copy(extension), loader);
    }
}

ImageLoader::ImageLoader()
{
    // PNG Loader
    addLoaderToMap(std::make_shared<PNGLoader>());

    // BMP handler
    addLoaderToMap(std::make_shared<BMPLoader>());

    // JPEG handler
    addLoaderToMap(std::make_shared<JPEGLoader>());

    // RLE-supporting TGA loader
    addLoaderToMap(std::make_shared<TGALoader>());

    // DDS loader
    addLoaderToMap(std::make_shared<DDSLoader>());
}

// Load image from VFS
ImagePtr ImageLoader::imageFromVFS(const std::string& rawName) const
{
    // Replace backslashes with forward slashes and strip of
    // the file extension of the provided token, and store
    // the result in the provided string.
    auto name  = os::standardPath(rawName).substr(0, rawName.rfind("."));

	for (const auto& extension : _extensions)
	{
        // Find the loader for this extension
        auto loaderIter = _loadersByExtension.find(extension);

        if (loaderIter == _loadersByExtension.end())
        {
            rWarning() << "Doom3ImageLoader: failed to find loader"
                          " for extension '" << extension << "'" << std::endl;
            continue;
        }

        ImageTypeLoader& ldr = *loaderIter->second;

		// Construct the full name of the image to load, including the
		// prefix (e.g. "dds/") and the file extension.
		std::string fullName = ldr.getPrefix() + name + "." + extension;

		// Try to open the file (will fail if the extension does not fit)
		auto file = GlobalFileSystem().openFile(fullName);

		// Has the file been loaded?
		if (file)
        {
			// Try to invoke the imageloader with a reference to the
			// ArchiveFile
			return ldr.load(*file);
		}
	}

    // File not found
	return ImagePtr();
}

ImagePtr ImageLoader::imageFromFile(const std::string& filename) const
{
    ImagePtr image;

    // Construct a DirectoryArchiveFile out of the filename
    auto file = std::make_shared<archive::DirectoryArchiveFile>(filename, filename);

    if (!file->failed())
    {
        const std::string ext = string::to_lower_copy(
            os::getExtension(filename)
        );

        auto loaderIter = _loadersByExtension.find(ext);
        if (loaderIter != _loadersByExtension.end())
        {
            image = loaderIter->second->load(*file);
        }
        else
        {
            rWarning() << "Doom3ImageLoader: no loader found for image "
                       << filename << std::endl;
        }
    }

    return image;
}

const std::string& ImageLoader::getName() const
{
    static std::string _name(MODULE_IMAGELOADER);
    return _name;
}

const StringSet& ImageLoader::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_GAMEMANAGER);
    }

    return _dependencies;
}

void ImageLoader::initialiseModule(const IApplicationContext&)
{
    // Load the texture types from the .game file
    auto texTypes = GlobalGameManager().currentGame()->getLocalXPath(GKEY_IMAGE_TYPES);

    for (const auto& node : texTypes)
    {
        // Get the file extension, store it as lowercase
        std::string extension = node.getContent();
        _extensions.emplace_back(string::to_lower_copy(extension));
    }
}

// Static module instance
module::StaticModuleRegistration<ImageLoader> imageLoaderModule;

} // namespace shaders
