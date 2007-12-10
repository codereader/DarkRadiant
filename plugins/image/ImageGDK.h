#ifndef IMAGEGDK_H_
#define IMAGEGDK_H_

#include "ifilesystem.h"
#include "iimage.h"
#include "imagelib.h" // for RGBAImagePtr
#include <iostream>

ImagePtr LoadImageGDK(ArchiveFile& file);

/* greebo: A GDKLoader is capable of loading any image files that can be
 * handled by GDK.
 *  
 * Use load() to actually retrieve an Image* object with the loaded image.
 * 
 * Shouldn't be used to load textures directly, use the 
 * GlobalShaderSystem() module instead.  
 * 
 * Complies with the ImageLoader interface defined in "iimage.h" 
 */
class GDKLoader :
	public ImageLoader
{
public:
	/* greebo: This loads the file and returns the pointer to 
	 * the allocated Image object (or NULL, if the load failed). 
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadImageGDK(file);
	}
	
	/* greebo: Gets the file extension of the supported image file type.
	 * This returns an empty string as there are several extensions supported by GDK.  
	 */
	std::string getExtension() const {
		return "";
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderGDK");
		return _name;
	}
  	
  	virtual const StringSet& getDependencies() const {
  		static StringSet _dependencies; // no dependencies
  		return _dependencies;
  	}
  	
  	virtual void initialiseModule(const ApplicationContext& ctx) {
  		globalOutputStream() << "ImageLoaderGDK::initialiseModule called.\n";
  	}
};

#endif /*IMAGEGDK_H_*/
