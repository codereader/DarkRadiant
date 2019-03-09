#include "Doom3ImageLoader.h"
#include "ImageLoaderWx.h"
#include "TGALoader.h"
#include "dds.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "iregistry.h"
#include "igame.h"

#include "string/case_conv.h"

#include "os/path.h"
#include "DirectoryArchiveFile.h"
#include "modulesystem/StaticModule.h"

namespace image
{

namespace
{

// Registry key holding texture types
const char* RKEY_IMAGE_TYPES = "/filetypes/texture//extension";

ImageTypeLoader::Extensions getGameFileImageExtensions()
{
	static ImageTypeLoader::Extensions _extensions;

	if (_extensions.empty())
	{
		// Load the texture types from the .game file
		xml::NodeList texTypes = GlobalGameManager().currentGame()->getLocalXPath(RKEY_IMAGE_TYPES);

		for (xml::NodeList::const_iterator i = texTypes.begin();
			 i != texTypes.end();
			 ++i)
		{
			// Get the file extension
			std::string extension = i->getContent();
			string::to_lower(extension);
            _extensions.push_back(extension);
		}
	}

	return _extensions;
}

} // namespace

void Doom3ImageLoader::addLoaderToMap(ImageTypeLoader::Ptr loader)
{
    ImageTypeLoader::Extensions exts = loader->getExtensions();
    for (auto i = exts.begin(); i != exts.end(); ++i)
    {
        _loadersByExtension[string::to_lower_copy(*i)] = loader;
    }
}

Doom3ImageLoader::Doom3ImageLoader()
{
    // Wx loader (this handles regular image file types like BMP and PNG)
    addLoaderToMap(std::make_shared<ImageLoaderWx>());

    // RLE-supporting TGA loader
    addLoaderToMap(std::make_shared<TGALoader>());

    // DDS loader
    addLoaderToMap(std::make_shared<DDSLoader>());
}

// Load image from VFS
ImagePtr Doom3ImageLoader::imageFromVFS(const std::string& name) const
{
	const ImageTypeLoader::Extensions exts = getGameFileImageExtensions();
	for (auto i = exts.begin(); i != exts.end(); ++i)
	{
        // Find the loader for this extension
        auto loaderIter = _loadersByExtension.find(*i);
        if (loaderIter == _loadersByExtension.end())
        {
            rWarning() << "Doom3ImageLoader: failed to find loader"
                          " for extension '" << *i << "'" << std::endl;
            continue;
        }

        ImageTypeLoader& ldr = *loaderIter->second;

		// Construct the full name of the image to load, including the
		// prefix (e.g. "dds/") and the file extension.
		std::string fullName = ldr.getPrefix() + name + "." + *i;

		// Try to open the file (will fail if the extension does not fit)
		ArchiveFilePtr file = GlobalFileSystem().openFile(fullName);

		// Has the file been loaded?
		if (file != NULL)
        {
			// Try to invoke the imageloader with a reference to the
			// ArchiveFile
			return ldr.load(*file);
		}
	}

    // File not found
	return ImagePtr();
}

ImagePtr Doom3ImageLoader::imageFromFile(const std::string& filename) const
{
    ImagePtr image;

    // Construct a DirectoryArchiveFile out of the filename
    std::shared_ptr<archive::DirectoryArchiveFile> file = 
        std::make_shared<archive::DirectoryArchiveFile>(filename, filename);

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

const std::string& Doom3ImageLoader::getName() const
{
    static std::string _name(MODULE_IMAGELOADER);
    return _name;
}

const StringSet& Doom3ImageLoader::getDependencies() const
{
    static StringSet _dependencies; // no dependencies
    return _dependencies;
}

// Static module instance
module::StaticModule<Doom3ImageLoader> doom3ImageLoaderModule;

} // namespace shaders
