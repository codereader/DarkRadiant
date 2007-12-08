#include "ShaderFileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "parser/DefTokeniser.h"
#include "ShaderDefinition.h"
#include "Doom3ShaderSystem.h"

#include <iostream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/* FORWARD DECLS */

namespace shaders {

/**
 * Parses the contents of a material definition. The shader name and opening
 * brace "{" will already have been removed when this function is called.
 * 
 * @param tokeniser
 * DefTokeniser to retrieve tokens from.
 * 
 * @param shaderTemplate
 * An empty ShaderTemplate which will parse the token stream and populate
 * itself.
 * 
 * @param filename
 * The name of the shader file we are parsing.
 */
void ShaderFileLoader::parseShaderDecl(parser::DefTokeniser& tokeniser, 
					  ShaderTemplatePtr shaderTemplate, 
					  const std::string& filename) 
{
	// Get the ShaderTemplate to populate itself by parsing tokens from the
	// DefTokeniser. This may throw exceptions.	
	shaderTemplate->parseDoom3(tokeniser);
	
	// Construct the ShaderDefinition wrapper class
	ShaderDefinition def(shaderTemplate, filename);
	
	// Get the parsed shader name
	std::string name = shaderTemplate->getName();
	
	// Insert into the definitions map, if not already present
    if (!GetShaderLibrary().addDefinition(name, def)) {
    	std::cout << "[shaders] " << filename << ": shader " << name
    			  << " already defined." << std::endl;
    }
}

/* Parses through the shader file and processes the tokens delivered by 
 * DefTokeniser. 
 */ 
void ShaderFileLoader::parseShaderFile(std::istream& inStr, 
									   const std::string& filename)
{
	// Create the tokeniser
	parser::BasicDefTokeniser<std::istream> tokeniser(inStr, 
													  " \t\n\v\r", 
													  "{}(),");
	
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

/* Normalises a given (raw) shadername (slash conversion, extension removal, ...) 
 * and inserts a new shader 
 */
ShaderTemplatePtr ShaderFileLoader::parseShaderName(std::string& rawName) 
{
	boost::algorithm::replace_all(rawName, "\\", "/"); // use forward slashes
	// greebo: Don't use lowercase as this might screw the shader lookup
	//boost::algorithm::to_lower(rawName); // use lowercase

	// Remove the extension if present
	size_t dotPos = rawName.rfind(".");
	if (dotPos != std::string::npos) {
		rawName = rawName.substr(0, dotPos);
	}

	// Construct and return the ShaderTemplate pointer	
	ShaderTemplatePtr shaderTemplate(new ShaderTemplate(rawName));
	return shaderTemplate;
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
void ShaderFileLoader::operator() (const std::string& fileName)
{
	// Construct the full VFS path
	std::string fullPath = _basePath + fileName;
	
	// Open the file
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fullPath);

	if (file != NULL) {
		std::istream is(&(file->getInputStream()));
		parseShaderFile(is, fullPath);
	} 
	else {
		throw std::runtime_error("Unable to read shaderfile: " + fullPath);  	
	}
}


} // namespace shaders
