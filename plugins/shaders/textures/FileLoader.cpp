#include "FileLoader.h"
#include "ImageLoaderManager.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "archivelib.h"

namespace shaders {

ImagePtr FileLoader::imageFromFile(const std::string& filename,
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
