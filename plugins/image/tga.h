#pragma once

#include "itextstream.h"
#include "ifilesystem.h"
#include "iimage.h"
#include <iostream>

ImagePtr LoadTGA(ArchiveFile& file);

/* greebo: A TGALoader is capable of loading TGA files.
 *
 * Use load() to actually retrieve an Image* object with the loaded image.
 *
 * Shouldn't be used to load textures directly, use the
 * GlobalMaterialManager() module instead.
 *
 * Complies with the ImageLoader interface defined in "iimage.h"
 */
class TGALoader :
	public ImageLoader
{
public:
	/* greebo: This loads the file and returns the pointer to
	 * the allocated Image object (or NULL, if the load failed).
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadTGA(file);
	}

	/* greebo: Gets the file extension of the supported image file type (e.g. "tga")
	 */
	std::string getExtension() const {
		return "tga";
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderTGA");
		return _name;
	}

  	virtual const StringSet& getDependencies() const {
  		static StringSet _dependencies; // no dependencies
  		return _dependencies;
  	}

  	virtual void initialiseModule(const ApplicationContext& ctx) {
  		rMessage() << "ImageLoaderTGA::initialiseModule called.\n";
  	}
};
