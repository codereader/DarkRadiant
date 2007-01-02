#include "ShaderFileLoader.h"
#include "ShaderTemplate.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "parser/DefTokeniser.h"

#include <iostream>

/* FORWARD DECLS */

// Funtions from shaders.cpp - TODO: refactor these into members of this class
ShaderTemplatePtr parseShaderName(std::string& rawName);
void parseShaderDecl(parser::DefTokeniser&, 
					 ShaderTemplatePtr, 
					 const std::string&);

namespace shaders {
	
/* Parses through the shader file and processes the tokens delivered by 
 * DefTokeniser. 
 */ 
void ShaderFileLoader::parseShaderFile(const std::string& inStr, 
									   const std::string& filename)
{
	// Create the tokeniser
	parser::DefTokeniser tokeniser(inStr, " \t\n\v\r", "{}(),");
	
	while (tokeniser.hasMoreTokens()) {
		// Load the first token, it should be a name
		std::string token = tokeniser.nextToken();		

		if (token == "table") {
			parseShaderTable(tokeniser);		 		
		} 
		else if (token[0] == '{' || token[0] == '}') {
			// This is not supposed to happen, as the shaderName is still 
			// undefined
			std::cerr << "[shaders] Missing shadername in " 
					  << filename << std::endl;
			return; 
		} 
		else if (token == "material") {
			// Discard optional "material" keyword
			token = tokeniser.nextToken();
		}
		else {			
			// We are still outside of any braces, so this must be a shader name			
			ShaderTemplatePtr shaderTemplate = parseShaderName(token);

			// Try to parse the rest of the decl, catching and printing any
			// exception
			try {
				tokeniser.assertNextToken("{");
				parseShaderDecl(tokeniser, shaderTemplate, filename);
			}
			catch (parser::ParseException e) {
				std::cerr << "[shaders] " << filename << ": Failed to parse \"" 
						  << shaderTemplate->getName() << "\" (" << e.what() 
						  << ")" << std::endl;
				return;
			}
		}
	}
}

/* Parses through a table definition within a material file.
 * It just skips over the whole content 
 */
void ShaderFileLoader::parseShaderTable(parser::DefTokeniser& tokeniser) 
{
	// This is the name of the table
	tokeniser.nextToken();
	
	tokeniser.assertNextToken("{");
	unsigned short int openBraces = 1; 
	
	// Continue parsing till the table is over, i.e. the according closing brace is found
	while (openBraces>0 && tokeniser.hasMoreTokens()) {
		const std::string token = tokeniser.nextToken();
		
		if (token == "{") {
			openBraces++;
		}
		
		if (token == "}") {
			openBraces--;
		}
	}	
}

// Functor operator
void ShaderFileLoader::operator() (const char* fileName)
{
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


} // namespace shaders
