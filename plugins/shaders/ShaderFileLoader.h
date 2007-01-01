#ifndef SHADERFILELOADER_H_
#define SHADERFILELOADER_H_

namespace shaders
{

/**
 * VFS functor class which loads material (mtr) files.
 */
class ShaderFileLoader
{
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;
	
public:

	// Required functor typedef
	typedef const char* first_argument_type;
	
	// Constructor. Set the basepath to prepend onto shader filenames.
	ShaderFileLoader(const std::string& path)
	: _basePath(path)
	{}
	
	// Functor operator
	void operator() (const char* fileName) {
		
		// Construct the full VFS path
		std::string fullPath = _basePath + fileName;
		
		// Open the file
		ArchiveTextFile* file = GlobalFileSystem().openTextFile(fullPath);

		if (file != 0) {
			parseShaderFile(file->getInputStream().getAsString(), fullPath);           
			file->release();
		} 
		else {
			throw std::runtime_error("Unable to read shaderfile: " + fullPath);  	
		}
	}
};

}

#endif /*SHADERFILELOADER_H_*/
