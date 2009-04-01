#ifndef FILELODER_H_
#define FILELODER_H_

#include "iimage.h"

namespace shaders 
{

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
class FileLoader
{
public:
	
	// Load an image from the given file
	static ImagePtr imageFromFile(const std::string& filename,
                                  const std::string& modules = "GDK");
};

} // namespace shaders

#endif /*FILELODER_H_*/
