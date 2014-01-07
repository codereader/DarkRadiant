#pragma once

#include "ifilesystem.h"
#include "iimage.h"
#include "itextstream.h"
#include <iostream>

ImagePtr LoadPCX32(ArchiveFile& file);

/* greebo: A PCXLoader is capable of loading PCX files.
 *
 * Use load() to actually retrieve an Image* object with the loaded image.
 *
 * Shouldn't be used to load textures directly, use the
 * GlobalMaterialManager() module instead.
 *
 * Complies with the ImageLoader interface defined in "iimage.h"
 */
class PCXLoader :
	public ImageLoader
{
public:
	/* greebo: This loads the file and returns the pointer to
	 * the allocated Image object (or NULL, if the load failed).
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadPCX32(file);
	}

	/* greebo: Gets the file extension of the supported image file type (e.g. "PCX")
	 */
	std::string getExtension() const {
		return "pcx";
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderPCX");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
  		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		rMessage() << "ImageLoaderPCX::initialiseModule called.\n";
	}
};
