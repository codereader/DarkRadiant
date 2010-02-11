#include "ifonts.h"

namespace fonts
{

class FontFinder
{
private:
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;

public:
	// Constructor. Set the base path of the search.
	FontFinder(const std::string& path) : 
		_basePath(path)
	{}

	// Required functor typedef
	typedef const std::string& first_argument_type;

	void operator() (const std::string& fileName)
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
};

} // namespace fonts
