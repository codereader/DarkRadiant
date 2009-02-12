#include "ShaderFileLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "parser/DefTokeniser.h"
#include "parser/DefBlockTokeniser.h"
#include "ShaderDefinition.h"
#include "Doom3ShaderSystem.h"

#include <iostream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/* FORWARD DECLS */

namespace shaders {

/* Parses through the shader file and processes the tokens delivered by 
 * DefTokeniser. 
 */ 
void ShaderFileLoader::parseShaderFile(std::istream& inStr, 
									   const std::string& filename)
{
	// Parse the file with a blocktokeniser, the actual block contents
	// will be parsed separately.
	parser::BasicDefBlockTokeniser<std::istream> tokeniser(inStr);

	while (tokeniser.hasMoreBlocks()) {
		// Get the next block
		parser::BlockTokeniser::Block block = tokeniser.nextBlock();

		// Skip tables
		if (block.name.substr(0, 5) == "table") {
			continue;
		}

		boost::algorithm::replace_all(block.name, "\\", "/"); // use forward slashes

		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(block.name, block.contents));
		
		// Construct the ShaderDefinition wrapper class
		ShaderDefinition def(shaderTemplate, filename);

		// Insert into the definitions map, if not already present
		if (!GetShaderLibrary().addDefinition(block.name, def)) {
    		globalErrorStream() << "[shaders] " << filename 
				<< ": shader " << block.name << " already defined." << std::endl;
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
