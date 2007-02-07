#ifndef IMAGEGDK_H_
#define IMAGEGDK_H_

#include "ifilesystem.h"
#include "iimage.h"
#include "modulesystem/singletonmodule.h"

Image* LoadImageGDK(ArchiveFile& file);

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
	// Definitions to enable the lookup by the radiant modulesystem
	typedef ImageLoader Type;
	STRING_CONSTANT(Name, "GDK");

	// Return the instance pointer
	ImageLoader* getTable() {
		return this;
	}

public:
	// Constructor
	GDKLoader() {}
	
	/* greebo: This loads the file and returns the pointer to 
	 * the allocated Image object (or NULL, if the load failed). 
	 */
	Image* load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadImageGDK(file);
	}
	
	/* greebo: Gets the file extension of the supported image file type.
	 * This returns an empty string as there are several extensions supported by GDK.  
	 */
	std::string getExtension() const {
		return "";
	}

};

// The dependency class
class GDKLoaderDependencies :
	public GlobalFileSystemModuleRef
{};

// Define the GDK module with the GDKLoader class and its dependencies
typedef SingletonModule<GDKLoader, GDKLoaderDependencies> GDKLoaderModule;

#endif /*IMAGEGDK_H_*/
