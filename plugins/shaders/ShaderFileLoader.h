#pragma once

#include "ifilesystem.h"
#include "iradiant.h"
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

	ILongRunningOperation* _currentOperation;

	std::vector<std::string> _files;

private:

	// Parse a shader file with the given contents and filename
	void parseShaderFile(std::istream& inStr, const std::string& filename);

public:
	// Constructor. Set the basepath to prepend onto shader filenames.
	ShaderFileLoader(const std::string& path, ILongRunningOperation* currentOperation)
	: _basePath(path),
	_currentOperation(currentOperation)
	{
		_files.reserve(200);
	}

	// FileVisitor implementation
	void visit(const std::string& filename);

	void parseFiles();
};

}
