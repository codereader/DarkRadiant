#ifndef SHADERFILELOADER_H_
#define SHADERFILELOADER_H_

#include "ShaderTemplate.h"
#include <string>

// FORWARD DECLS
namespace parser { class DefTokeniser; }

namespace shaders
{

/**
 * VFS functor class which loads material (mtr) files.
 */
class ShaderFileLoader
{
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;
	
private:

	/*Parses the contents of a material definition. The shader name and opening
 	 * brace "{" will already have been removed when this function is called. */
	void parseShaderDecl(parser::DefTokeniser& tokeniser, 
					  ShaderTemplatePtr shaderTemplate, 
					  const std::string& filename); 

	// Parse a shader file with the given contents and filename
	void parseShaderFile(const std::string& inStr, const std::string& filename);

	// Parse the name of a shader and return a ShaderTemplate ready to be
	// filled in with parsed data
	ShaderTemplatePtr parseShaderName(std::string& name); 

	// Parse a "table" definition in a shader file
	void parseShaderTable(parser::DefTokeniser& tokeniser);
	
public:

	// Required functor typedef
	typedef const char* first_argument_type;
	
	// Constructor. Set the basepath to prepend onto shader filenames.
	ShaderFileLoader(const std::string& path)
	: _basePath(path)
	{}
	
	// Functor operator
	void operator() (const char* fileName);
};

}

#endif /*SHADERFILELOADER_H_*/
