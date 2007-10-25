#include "ShaderTemplate.h"
#include "MapExpression.h"

#include "os/path.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

/*	Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable m_nFlags
 * 
 * 	Note: input "token" has to be lowercase for the keywords to be recognized
 */
bool ShaderTemplate::parseShaderFlags(parser::DefTokeniser& tokeniser, const std::string& token) 
{	
	if (token == "qer_trans") {
		m_fTrans = boost::lexical_cast<float>(tokeniser.nextToken());
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
	else if (token == "description") {
		// greebo: Parse description token, this should be the next one
		description = tokeniser.nextToken();
	}
	else {
		// We haven't found anything of interest >> return false
		return false;
	}

	return true;
}

/* Searches for light-specific keywords and takes the appropriate actions
 */
bool ShaderTemplate::parseLightFlags(parser::DefTokeniser& tokeniser, const std::string& token) 
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
    	_lightFalloff = shaders::IMapExpression::createForToken(tokeniser);
    }
	else {
		// No light-specific keywords found, return false
		return false;	
	}
	return true;
}

// Parse any single-line stages (such as "diffusemap x/y/z")
bool ShaderTemplate::parseBlendShortcuts(parser::DefTokeniser& tokeniser, 
										 const std::string& token) 
{
	if (token == "qer_editorimage") {
		_texture = shaders::IMapExpression::createForToken(tokeniser);
	}
	else if (token == "diffusemap") {
		_diffuse = shaders::IMapExpression::createForToken(tokeniser);
	}
	else if (token == "specularmap") {
		_specular = shaders::IMapExpression::createForToken(tokeniser);
	}
	else if (token == "bumpmap") {
		_bump = shaders::IMapExpression::createForToken(tokeniser);
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
bool ShaderTemplate::parseBlendType(parser::DefTokeniser& tokeniser, const std::string& token) 
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
bool ShaderTemplate::parseClamp(parser::DefTokeniser& tokeniser, const std::string& token) 
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
bool ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, const std::string& token) 
{	
	if (token == "map") {
		m_currentLayer.mapExpr = shaders::IMapExpression::createForToken(tokeniser);		
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
			_diffuse = m_currentLayer.mapExpr;
			break;
		case LAYER_BUMPMAP:
			_bump = m_currentLayer.mapExpr;
			break;
		case LAYER_SPECULARMAP:
			_specular = m_currentLayer.mapExpr;
			break;
		default:
			if (m_currentLayer.mapExpr) {
				m_layers.push_back(m_currentLayer);
			}
	}
	
	// Clear the currentLayer structure for possible future layers
	m_currentLayer.m_type = LAYER_NONE;
	m_currentLayer.mapExpr = shaders::MapExpressionPtr();
	return true;
}

/* Parses a material definition for shader keywords and takes the according 
 * actions. 
 */
void ShaderTemplate::parseDoom3(parser::DefTokeniser& tokeniser)
{
	int curStage = 1;	// we always start at stage 1
		
	while (curStage>0 && tokeniser.hasMoreTokens()) {
		std::string token = tokeniser.nextToken();
		std::string token_lowercase = boost::algorithm::to_lower_copy(token);
		
		if (token=="}") {
			
			if (--curStage == 1) {
				saveLayer();
			}
			
			// If the texture is missing (i.e. no editorimage provided), 
			// substitute this with the diffusemap
			if (!_texture) {		
				_texture = _diffuse;
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
	
}
