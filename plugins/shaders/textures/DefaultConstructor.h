#ifndef DEFAULTCONSTRUCTOR_H_
#define DEFAULTCONSTRUCTOR_H_

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
	// The filename of the image to load
	std::string _filename;

private:
	
	// Get the list of ImageLoaders associated with the .game file formats. This
	// will be the same for all DefaultConstructors so we cache it statically.
	static const ImageLoaderList& getImageLoaders();
	
public:
	// Constructor
	DefaultConstructor(const std::string& filename);
	
	// The actual construct() command as needed by the GLTextureManager
	ImagePtr construct();

}; // class DefaultConstructor

} // namespace shaders

#endif /*DEFAULTCONSTRUCTOR_H_*/
