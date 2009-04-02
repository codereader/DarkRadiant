#include "ImageFileLoader.h"
#include "ImageLoaderManager.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "archivelib.h"
#include "iregistry.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace shaders 
{

namespace
{
	// Registry key holding texture types
    const char* RKEY_IMAGE_TYPES = "game/filetypes/texture//extension";
}

// Static accessor for the list of .game-defined ImageLoaders
const ImageLoaderList& ImageFileLoader::getImageLoaders() 
{
	static ImageLoaderList _imageLoaders;
	
	if (_imageLoaders.empty()) 
	{
		// Load the texture types from the .game file
		xml::NodeList texTypes = GlobalRegistry().findXPath(RKEY_IMAGE_TYPES);
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

	const ImageLoaderList& loaders = getImageLoaders();
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

    ImageLoaderList imageLoaders(ImageLoaderManager::getLoaders(modules));
	for (unsigned int i = 0; i < imageLoaders.size(); i++) 
    {
		// Construct a DirectoryArchiveFile out of the filename		
		DirectoryArchiveFilePtr file(
            new DirectoryArchiveFile(filename, filename)
        );

		if (!file->failed()) 
        {
			// Try to invoke the imageloader with a reference to an ArchiveFile
			image = imageLoaders[i]->load(*file);
			break;
		}
	}
	
	return image;
}

} // namespace shaders
