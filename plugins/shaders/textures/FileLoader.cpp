#include "FileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "archivelib.h"

namespace shaders {

// The default constructor uses the GDK file loader (which covers a wide range of formats)
FileLoader::FileLoader(const std::string& filename, const std::string moduleNames) :
	_imageLoaders(ImageLoaderManager::getLoaders(moduleNames)),
	_filename(filename)
{}

ImagePtr FileLoader::construct() {
	ImagePtr image;

	for (unsigned int i = 0; i < _imageLoaders.size(); i++) {
		// Construct a DirectoryArchiveFile out of the filename		
		DirectoryArchiveFilePtr file(new DirectoryArchiveFile(_filename, _filename));

		if (!file->failed()) {
			// Try to invoke the imageloader with a reference to an ArchiveFile
			image = _imageLoaders[i]->load(*file);
			break;
		}
	}
	
	return image;
}

} // namespace shaders
