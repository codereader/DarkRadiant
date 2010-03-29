#ifndef SHADERFILELOADER_H_
#define SHADERFILELOADER_H_

#include "ifilesystem.h"
#include "ShaderTemplate.h"

#include "parser/DefTokeniser.h"

#include <string>

namespace shaders
{

/**
 * VFS functor class which loads material (mtr) files.
 */
class ShaderFileLoader :
	public VirtualFileSystem::Visitor
{
private:
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;
	
private:

	// Parse a shader file with the given contents and filename
	void parseShaderFile(std::istream& inStr, const std::string& filename);

	// Parse a "table" definition in a shader file
	void parseShaderTable(parser::DefTokeniser& tokeniser);
	
public:
	// Constructor. Set the basepath to prepend onto shader filenames.
	ShaderFileLoader(const std::string& path)
	: _basePath(path)
	{}
	
	// FileVisitor implementation
	void visit(const std::string& filename);
};

}

#endif /*SHADERFILELOADER_H_*/
