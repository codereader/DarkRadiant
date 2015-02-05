#pragma once

#include "ifilesystem.h"
#include "iradiant.h"
#include "ShaderTemplate.h"

#include "parser/DefTokeniser.h"

#include <string>

namespace shaders
{

class ShaderLibrary;

/**
 * VFS functor class which loads material (mtr) files.
 */
class ShaderFileLoader
{
private:
	// The base path for the shaders (e.g. "materials/")
	std::string _basePath;

    ShaderLibrary& _library;

	ILongRunningOperation* _currentOperation;

	std::vector<std::string> _files;

private:

	// Parse a shader file with the given contents and filename
	void parseShaderFile(std::istream& inStr, const std::string& filename);

public:
	// Constructor. Set the basepath to prepend onto shader filenames.
    ShaderFileLoader(const std::string& path, 
                     ShaderLibrary& library, 
                     ILongRunningOperation* currentOperation) : 
        _basePath(path),
        _library(library),
	    _currentOperation(currentOperation)
	{
		_files.reserve(200);
	}

	void addFile(const std::string& filename);

	void parseFiles();
};

}
