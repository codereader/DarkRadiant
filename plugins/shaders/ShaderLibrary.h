#ifndef SHADERLIBRARY_H_
#define SHADERLIBRARY_H_

#include <string>
#include <map>
#include "CShader.h"

class ShaderLibrary
{
	typedef std::map<std::string, ShaderPtr> ShaderMap;
	typedef ShaderMap::iterator iterator;
	
	ShaderMap _shaders;
	  
public:
	// Constructor
	ShaderLibrary();
	
	// Destructor
	~ShaderLibrary();
	
	iterator begin();
	iterator end();
	iterator find(const std::string& name);

}; // class ShaderLibrary

#endif /*SHADERLIBRARY_H_*/
