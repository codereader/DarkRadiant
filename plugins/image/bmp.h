#pragma once

#include "ifilesystem.h"
#include "iimage.h"
#include "itextstream.h"
#include <iostream>

ImagePtr LoadBMP(ArchiveFile& file);

/* greebo: A BMPLoader is capable of loading Bitmap files.
 *
 * Use load() to actually retrieve an Image* object with the loaded image.
 *
 * Shouldn't be used to load textures directly, use the
 * GlobalMaterialManager() module instead.
 *
 * Complies with the ImageLoader interface defined in "iimage.h"
 */
class BMPLoader :
	public ImageLoader
{
public:
    virtual ~BMPLoader() {}

	/* greebo: This loads the file and returns the pointer to
	 * the allocated Image object (or an empty pointer, if the load failed).
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadBMP(file);
	}

	/* greebo: Gets the file extension of the supported image file type (e.g. "BMP")
	 */
	std::string getExtension() const {
		return "bmp";
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderBMP");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
  		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		rMessage() << "ImageLoaderBMP::initialiseModule called.\n";
	}
};
