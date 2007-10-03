#ifndef DEFAULTCONSTRUCTOR_H_
#define DEFAULTCONSTRUCTOR_H_

#include "iradiant.h"
#include "iimage.h"
#include "ishaders.h"

#include "ImageLoaderManager.h"

namespace shaders {

/* greebo: This is the default imageconstructor that loads a given file 
 * from the disk and creates the according Image object (on demand).
 * 
 * It constructs paths relative to the doom3 mod folder (./textures/...) 
 * and searches for the file formats defined in doom3.game. 
 */
class DefaultConstructor : 
	public TextureConstructor
{
	// The ImageLoader references 
	ImageLoaderList _imageLoaders;
	
	// The filename of the image to load
	std::string _filename;

public:
	// Constructor
	DefaultConstructor(const std::string& filename);
	
	// The actual construct() command as needed by the GLTextureManager
	ImagePtr construct();

}; // class DefaultConstructor

} // namespace shaders

#endif /*DEFAULTCONSTRUCTOR_H_*/
