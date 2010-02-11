#include "FontLoader.h"

namespace fonts 
{

void FontLoader::operator() (const std::string& fileName)
{
	// Construct the full VFS path
	std::string fullPath = _basePath + fileName;
	
	// Open the file
	/*ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fullPath);

	if (file != NULL) {
		std::istream is(&(file->getInputStream()));
		parseShaderFile(is, fullPath);
	} 
	else {
		throw std::runtime_error("Unable to read shaderfile: " + fullPath);  	
	}*/
}

} // namespace fonts 
