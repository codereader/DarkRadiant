#include "ShaderTemplate.h"

// clean a texture name to the qtexture_t name format we use internally
// NOTE: case sensitivity: the engine is case sensitive. we store the shader name with case information and save with case
// information as well. but we assume there won't be any case conflict and so when doing lookups based on shader name,
// we compare as case insensitive. That is Radiant is case insensitive, but knows that the engine is case sensitive.
//++timo FIXME: we need to put code somewhere to detect when two shaders that are case insensitive equal are present
template<typename StringType>
void parseTextureName(StringType& name, const char* token)
{
  StringOutputStream cleaned(256);
  cleaned << PathCleaned(token);
  name = CopiedString(StringRange(cleaned.c_str(), path_get_filename_base_end(cleaned.c_str()))).c_str(); // remove extension
}

/*	Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable m_nFlags
 * 
 * 	Note: input "token" has to be lowercase for the keywords to be recognized
 */
bool ShaderTemplate::parseShaderFlags(parser::DefTokeniser& tokeniser, std::string& token) 
{	
	if (token == "qer_trans") {
		m_fTrans = string_read_float(tokeniser.nextToken().c_str());
        m_nFlags |= QER_TRANS;
	}
	else if (token == "translucent") {
        m_fTrans = 1;
        m_nFlags |= QER_TRANS;
	} 
	else if (token == "decal_macro") {
		m_fTrans = 1;
		m_nFlags |= QER_TRANS;
	} 
	else if (token == "twosided") {
		m_Cull = IShader::eCullNone;
		m_nFlags |= QER_CULL;
	}
	else if (token == "nodraw") {
		m_nFlags |= QER_NODRAW;
	}
	else if (token == "nonsolid") {
		m_nFlags |= QER_NONSOLID;
	}
	else if (token == "liquid") {
		m_nFlags |= QER_WATER;
	}
	else if (token == "areaportal") {
		m_nFlags |= QER_AREAPORTAL;
	}
	else if (token == "playerclip" || token == "monsterclip" 
			|| token == "ikclip" || token == "moveableclip") {
		m_nFlags |= QER_CLIP;
	}
	else {
		// We haven't found anything of interest >> return false
		return false;
	}

	return true;
}

/* Searches the next token for a map (e.g. textures/name), 
 * also accepts Image Program Functions like heightmap(), scale() etc.
 * the implementation for some of these programs are still to be done.
 * 
 * Results are stored in m_currentLayer for further use within the calling function 
 */
bool ShaderTemplate::parseMap(parser::DefTokeniser& tokeniser)
{
	std::string token = boost::algorithm::to_lower_copy(tokeniser.nextToken());	
		
	if (token == "") {
		throw parser::ParseException("Missing map identifier");
	}
	// These keywords are ignored at the moment and their argument is taken as texture name
	else if (token == "makeintensity" || token == "makealpha" || token == "invertcolor"
			 || token == "invertalpha" || token == "smoothnormals" ) {
		tokeniser.assertNextToken("(");
		parseTextureName(m_currentLayer.m_texture, tokeniser.nextToken().c_str());
		tokeniser.assertNextToken(")");
	}
	// Currently the second argument for addnormals() is ignored and the first is returned
	else if (token == "addnormals") {
		tokeniser.assertNextToken("(");
		
		// Load the first argument into the member variable		
		parseTextureName(m_currentLayer.m_texture, tokeniser.nextToken().c_str());
		
		tokeniser.assertNextToken(",");
		
		std::string normal2;
		parseTextureName(normal2, tokeniser.nextToken().c_str());
								
		tokeniser.assertNextToken(")");
	} 
	// The second argument for heightmap (the height scale) is ignored at the moment
	else if (token == "heightmap") {
		tokeniser.assertNextToken("(");		
		parseTextureName(m_currentLayer.m_texture, tokeniser.nextToken().c_str());
		tokeniser.assertNextToken(",");		
		m_currentLayer.m_heightmapScale = tokeniser.nextToken().c_str();		
		tokeniser.assertNextToken(")");
	}
	// The second argument for add is ignored at the moment
	else if (token == "add") {
		tokeniser.assertNextToken("(");
		parseTextureName(m_currentLayer.m_texture, tokeniser.nextToken().c_str());
		tokeniser.assertNextToken(",");
		TextureExpression map2;
		parseTextureName(map2, tokeniser.nextToken().c_str());
		tokeniser.assertNextToken(")");
	}
	// The floating point values for the scale program are ignored and the texture name is returned
	else if (token == "scale") {
		tokeniser.assertNextToken("(");
		parseTextureName(m_currentLayer.m_texture, tokeniser.nextToken().c_str());
		tokeniser.assertNextToken(",");
		while (tokeniser.nextToken() != ")") {
			// do nothing
		}
	}
	else {
		parseTextureName(m_currentLayer.m_texture, token.c_str());
	}
	
	return true;
}

/* Searches for light-specific keywords and takes the appropriate actions
 */
bool ShaderTemplate::parseLightFlags(parser::DefTokeniser& tokeniser, std::string& token) 
{	
	if (token == "ambientlight") {
		ambientLight = true;
	} 
	else if (token == "blendlight") {
		blendLight = true;
	} 
	else if (token == "foglight") {
        fogLight = true;
    }
    else if (!fogLight && token == "lightfalloffimage") {
    	parseMap(tokeniser);
    	m_lightFalloffImage = m_currentLayer.m_texture;
    }
	else {
		// No light-specific keywords found, return false
		return false;	
	}
	return true;
}

bool ShaderTemplate::parseBlendShortcuts(parser::DefTokeniser& tokeniser, std::string& token) 
{
	if (token == "qer_editorimage") {
		parseMap(tokeniser);
		m_textureName = m_currentLayer.m_texture;		
	}
	else if (token == "diffusemap") {
		parseMap(tokeniser);
		m_diffuse = m_currentLayer.m_texture;		
	}
	else if (token == "specularmap") {
		parseMap(tokeniser);
		m_specular = m_currentLayer.m_texture;
	}
	else if (token == "bumpmap") {
		parseMap(tokeniser);
		m_bump = m_currentLayer.m_texture;
		m_heightmapScale = m_currentLayer.m_heightmapScale;
	}
	else {
		// No shortcuts found, return false
		return false;	
	}
	return true;
}

/* Parses for possible blend commands like "add", "diffusemap", "gl_one, gl_zero" etc.
 * Note: input "token" has to be lowercase
 * Output: true, if the blend keyword was found, false otherwise.
 */
bool ShaderTemplate::parseBlendType(parser::DefTokeniser& tokeniser, std::string& token) 
{
	if (token == "blend") {
		std::string blendType = boost::algorithm::to_lower_copy(tokeniser.nextToken());
		
		if (blendType == "diffusemap") {
			m_currentLayer.m_type = LAYER_DIFFUSEMAP;
		}
		else if (blendType == "bumpmap") {
			m_currentLayer.m_type = LAYER_BUMPMAP;
		}
		else if (blendType == "specularmap") {
			m_currentLayer.m_type = LAYER_SPECULARMAP;
		}
		else {
			m_currentLayer.m_blendFunc.first = blendType.c_str();
			
			if (blendType.substr(0,3) == "gl_") {
				// there is a second argument to parse
				tokeniser.assertNextToken(",");
				m_currentLayer.m_blendFunc.second = tokeniser.nextToken().c_str();
			} else {
				m_currentLayer.m_blendFunc.second = "";
			}			
		}		
	} 
	else {
		// Nothing found
		return false;	
	}
	
	return true;
}

/* Searches for clamp keywords in stage 2, expects token to be lowercase 
 */
bool ShaderTemplate::parseClamp(parser::DefTokeniser& tokeniser, std::string& token) 
{	
	if (token == "zeroclamp") {
		m_currentLayer.m_clampToBorder = true;
	}
	else {
		return false;
	}
	
	return true;
}

/* Searches for the map keyword in stage 2, expects token to be lowercase 
 */
bool ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, std::string& token) 
{	
	if (token == "map") {
		parseMap(tokeniser);		
	}
	else {
		return false;
	}
	
	return true;
}

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the m_layers vector.  
 */
bool ShaderTemplate::saveLayer()
{
	switch (m_currentLayer.m_type) {
		case LAYER_DIFFUSEMAP:
			m_diffuse = m_currentLayer.m_texture;
			break;
		case LAYER_BUMPMAP:
			m_bump = m_currentLayer.m_texture;
			break;
		case LAYER_SPECULARMAP:
			m_specular = m_currentLayer.m_texture;
			break;
		default:
			if (m_currentLayer.m_texture != "") {
				m_layers.push_back(MapLayerTemplate(
					m_currentLayer.m_texture.c_str(),
					m_currentLayer.m_blendFunc,
            		m_currentLayer.m_clampToBorder,
					m_currentLayer.m_alphaTest));
			}
	}
	
	// Clear the currentLayer structure for possible future layers
	m_currentLayer.m_type = LAYER_NONE;
	m_currentLayer.m_texture = "";
	return true;
}

/* Parses a material definition for shader keywords and takes the according actions.
 */
bool ShaderTemplate::parseDoom3(parser::DefTokeniser& tokeniser)
{
	int curStage = 1;	// we always start at stage 1
		
	while (curStage>0 && tokeniser.hasMoreTokens()) {
		std::string token = tokeniser.nextToken();
		std::string token_lowercase = boost::algorithm::to_lower_copy(token);
		
		if (token=="}") {
			--curStage;
			switch (curStage) {
				case 1:
					saveLayer();
					break;
			}
			
			// If the textureName is missing (i.e. no editorimage provided), substitute this with the diffusemap
			if (m_textureName == "" && m_diffuse != "") {				
				m_textureName = m_diffuse;
			}
		} 
		else if (token=="{") {
			++curStage;
		}
		else {
			switch (curStage) {
				case 1:
					parseShaderFlags(tokeniser, token_lowercase);
					parseLightFlags(tokeniser, token_lowercase);
					parseBlendShortcuts(tokeniser, token_lowercase);
					break;
				case 2:
					parseBlendType(tokeniser, token_lowercase);
					parseBlendMaps(tokeniser, token_lowercase);
					parseClamp(tokeniser, token_lowercase);
					break;
			}
		} 
	}
	
	return true;
}
