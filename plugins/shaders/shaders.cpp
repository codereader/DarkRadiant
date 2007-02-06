/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Shaders Manager Plugin
//
// Leonardo Zide (leo@lokigames.com)
//

#include "shaders.h"
#include "MissingXMLNodeException.h"
#include "ShaderTemplate.h"
#include "ShaderFileLoader.h"

#include <map>

#include "ifilesystem.h"
#include "itextures.h"
#include "qerplugin.h"
#include "irender.h"
#include "iregistry.h"

#include "parser/ParseException.h"
#include "parser/DefTokeniser.h"
#include "debugging/debugging.h"
#include "generic/callback.h"
#include "generic/referencecounted.h"
#include "shaderlib.h"
#include "texturelib.h"
#include "moduleobservers.h"
#include "archivelib.h"
#include "imagelib.h"

#include "xmlutil/Node.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include "textures/DefaultConstructor.h"
#include "textures/FileLoader.h"
#include "textures/GLTextureManager.h"
#include "CShader.h"
#include "Doom3ShaderSystem.h"

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
void parseShaderDecl(parser::DefTokeniser& tokeniser, 
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
