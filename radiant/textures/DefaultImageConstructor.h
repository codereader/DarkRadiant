#ifndef DEFAULTIMAGECONSTRUCTOR_H_
#define DEFAULTIMAGECONSTRUCTOR_H_

#include "iimage.h"
#include "qerplugin.h"
#include "modulesystem/modulesmap.h"

/* greebo: This is a temporary implementation of an ImageConstructor
 * class to be used by the TexturesCache module.
 * 
 * This functionality should be moved into the Shadersystem module and this file
 * consequently removed.  
 */
class DefaultImageConstructor : 
	public ImageConstructor
{
	// The reference collection of ImageLoader modules 
	ImageLoaderModulesRef _imageLoaders;

public:
	DefaultImageConstructor() :
		_imageLoaders(GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes"))
	{}
	
	Image* construct() {
		std::cout << "DefaultImageConstructor::construct called\n";
		return NULL;
	}
};

#endif /*DEFAULTIMAGECONSTRUCTOR_H_*/
