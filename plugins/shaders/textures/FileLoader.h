#ifndef FILELODER_H_
#define FILELODER_H_

#include "iradiant.h"
#include "iimage.h"
#include "ishaders.h"

#include "ImageLoaderManager.h"

namespace shaders {

/* greebo: This is another simple imageconstructor that loads a given file 
 * from the disk and creates the according Image object (on demand).
 * 
 * It uses ABSOLUTE paths and does not care about altering the given filename,
 * so use this to load images directly from a specified path.
 * 
 * Optional: pass the imageloader module names in the constructor (e.g. "bmp jpg")
 * to specify which loaders to use. It's using the GDK image loader by default, which
 * covers a wide range of file formats.
 */
class FileLoader : 
	public TextureConstructor
{
	// The list of ImageLoader modules 
	ImageLoaderList _imageLoaders;
	
	// The filename of the image to load
	std::string _filename;

public:
	// The default FileLoader constructor (uses the GDK image loader)
	FileLoader(const std::string& filename, const std::string moduleNames = "GDK");
	
	// The actual construct() method as needed by the GLTextureManager
	ImagePtr construct();
};

} // namespace shaders

#endif /*FILELODER_H_*/
