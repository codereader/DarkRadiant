#ifndef SHADERDEFINITION_H_
#define SHADERDEFINITION_H_

#include <map>
#include <string>
#include "ShaderTemplate.h"
#include "ShaderNameCompareFunctor.h"

/**
 * Wrapper class that associates a ShaderTemplate with its filename.
 */
struct ShaderDefinition
{
	// The shader template
	ShaderTemplatePtr shaderTemplate;
	
	// Filename from which the shader was parsed
	std::string filename;

	/* Constructor
	 */
	ShaderDefinition(ShaderTemplatePtr templ, const std::string& fname) : 
		shaderTemplate(templ),
		filename(fname)
	{}

};

typedef std::map<std::string, ShaderDefinition, ShaderNameCompareFunctor> ShaderDefinitionMap;

#endif /*SHADERDEFINITION_H_*/
