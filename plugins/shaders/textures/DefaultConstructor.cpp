#include "DefaultConstructor.h"

#include "ifilesystem.h"
#include "iarchive.h"

namespace shaders {

// The default case uses the file extensions specified in the .game file
DefaultConstructor::DefaultConstructor(const std::string& filename) :
	_imageLoaders(
		ImageLoaderManager::Instance().getLoaders(
			GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes")
		)
	),
	_filename(filename)
{}

// Load the actual image on demand
Image* DefaultConstructor::construct() {

	Image* returnValue = NULL;

	for (unsigned int i = 0; i < _imageLoaders.size(); i++) {
		ImageLoader& loader = *_imageLoaders[i];
		
		// Construct the full name of the image to load, including the 
		// prefix (e.g. "dds/") and the file extension.
		std::string fullName = loader.getPrefix() + _filename + "." 
							   + loader.getExtension();
		
		// Try to open the file (will fail if the extension does not fit)
		ArchiveFile* file = GlobalFileSystem().openFile(fullName.c_str());
		
		// Has the file been loaded?
		if (file != NULL) {
			// Try to invoke the imageloader with a reference to the 
			// ArchiveFile
			returnValue = loader.load(*file);
			
			// Release the loaded file
			file->release();
			
			break;
		}
	}
	
	return returnValue;
}

} // namespace shaders
