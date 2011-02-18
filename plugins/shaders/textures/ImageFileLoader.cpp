#include "ImageFileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "archivelib.h"
#include "iregistry.h"
#include "igame.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace shaders
{

namespace
{
	// Registry key holding texture types
    const char* RKEY_IMAGE_TYPES = "/filetypes/texture//extension";
}

// Static accessor for the list of .game-defined ImageLoaders
const ImageFileLoader::ImageLoaderList& 
ImageFileLoader::getGameFileImageLoaders()
{
	static ImageLoaderList _imageLoaders;

	if (_imageLoaders.empty())
	{
		// Load the texture types from the .game file
		xml::NodeList texTypes = GlobalGameManager().currentGame()->getLocalXPath(RKEY_IMAGE_TYPES);

		for (xml::NodeList::const_iterator i = texTypes.begin();
			 i != texTypes.end();
			 ++i)
		{
			// Get the file extension
			std::string extension = i->getContent();
			boost::algorithm::to_upper(extension);

			// Attempt to obtain an ImageLoader for this extension
			ImageLoaderPtr loader = GlobalImageLoader(extension);
			if (loader) {
				_imageLoaders.push_back(loader);
			}
		}
	}

	return _imageLoaders;
}

// Load image from VFS
ImagePtr ImageFileLoader::imageFromVFS(const std::string& name)
{
	ImagePtr returnValue;

	const ImageLoaderList& loaders = getGameFileImageLoaders();
	for (ImageLoaderList::const_iterator i = loaders.begin();
		 i != loaders.end();
		 ++i)
	{
		const ImageLoaderPtr& ldr = *i;

		// Construct the full name of the image to load, including the
		// prefix (e.g. "dds/") and the file extension.
		std::string fullName = ldr->getPrefix() + name + "."
							   + ldr->getExtension();

		// Try to open the file (will fail if the extension does not fit)
		ArchiveFilePtr file = GlobalFileSystem().openFile(fullName);

		// Has the file been loaded?
		if (file != NULL) {
			// Try to invoke the imageloader with a reference to the
			// ArchiveFile
			returnValue = ldr->load(*file);
			break;
		}
	}

	return returnValue;
}

ImagePtr ImageFileLoader::imageFromFile(const std::string& filename,
                                        const std::string& modules)
{
    ImagePtr image;
    ImageLoaderList imageLoaders = getNamedLoaders(modules);

    for (ImageLoaderList::const_iterator i = imageLoaders.begin();
         i != imageLoaders.end();
         ++i)
    {
        // Construct a DirectoryArchiveFile out of the filename
        DirectoryArchiveFilePtr file(
            new DirectoryArchiveFile(filename, filename)
        );

        if (!file->failed())
        {
            // Try to invoke the imageloader with a reference to an ArchiveFile
            image = (*i)->load(*file);
            break;
        }
    }

    return image;
}

ImageFileLoader::ImageLoaderList 
ImageFileLoader::getNamedLoaders(const std::string& names) 
{
    ImageLoaderList list;

    // Split string into loader names
    std::list<std::string> parts;
    boost::algorithm::split(parts, names, boost::algorithm::is_any_of(" "));

    for (std::list<std::string>::const_iterator i = parts.begin();
         i != parts.end();
         ++i) 
    {
        std::string fileExt = boost::to_upper_copy(*i);

        // Acquire the module using the given fileExt
        ImageLoaderPtr loader = GlobalImageLoader(fileExt);
        if (loader) 
        {
            list.push_back(loader);
        }
    }

    return list;
}


} // namespace shaders
