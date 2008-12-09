#include "DefaultConstructor.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "iregistry.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace shaders {

namespace {

	// Registry key holding texture types
	const char* RKEY_IMAGE_TYPES = "game/filetypes/texture//extension";

}

// The default case uses the file extensions specified in the .game file
DefaultConstructor::DefaultConstructor(const std::string& filename) :
	_filename(filename)
{ }

// Static accessor for the list of .game-defined ImageLoaders
const ImageLoaderList& DefaultConstructor::getImageLoaders() {

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

// Load the actual image on demand
ImagePtr DefaultConstructor::construct() {

	ImagePtr returnValue;

	const ImageLoaderList& loaders = getImageLoaders();
	for (ImageLoaderList::const_iterator i = loaders.begin();
		 i != loaders.end();
		 ++i) 
	{
		const ImageLoaderPtr& ldr = *i;
		
		// Construct the full name of the image to load, including the 
		// prefix (e.g. "dds/") and the file extension.
		std::string fullName = ldr->getPrefix() + _filename + "." 
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

} // namespace shaders
