#include "ShaderTemplate.h"
#include "MapExpression.h"

#include "itextstream.h"

#include "os/path.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

/*	Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable m_nFlags
 * 
 * 	Note: input "token" has to be lowercase for the keywords to be recognized
 */
void ShaderTemplate::parseShaderFlags(parser::DefTokeniser& tokeniser, const std::string& token) 
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
}

/* Searches for light-specific keywords and takes the appropriate actions
 */
void ShaderTemplate::parseLightFlags(parser::DefTokeniser& tokeniser, const std::string& token) 
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
}

// Parse any single-line stages (such as "diffusemap x/y/z")
void ShaderTemplate::parseBlendShortcuts(parser::DefTokeniser& tokeniser, 
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
}

/* Parses for possible blend commands like "add", "diffusemap", "gl_one, gl_zero" etc.
 * Note: input "token" has to be lowercase
 * Output: true, if the blend keyword was found, false otherwise.
 */
void ShaderTemplate::parseBlendType(parser::DefTokeniser& tokeniser, const std::string& token) 
{
	if (token == "blend") 
    {
		std::string blendType = boost::algorithm::to_lower_copy(tokeniser.nextToken());
		
		if (blendType == "diffusemap") {
			_currentLayer.m_type = LAYER_DIFFUSEMAP;
		}
		else if (blendType == "bumpmap") {
			_currentLayer.m_type = LAYER_BUMPMAP;
		}
		else if (blendType == "specularmap") {
			_currentLayer.m_type = LAYER_SPECULARMAP;
		}
		else 
        {
            // Special blend type, either predefined like "add" or "modulate",
            // or an explicit combination of GL blend modes
			_currentLayer.blendFunc.first = blendType;
			
			if (blendType.substr(0,3) == "gl_") 
            {
				// This is an explicit GL blend mode
				tokeniser.assertNextToken(",");
				_currentLayer.blendFunc.second = tokeniser.nextToken();
			} else {
				_currentLayer.blendFunc.second = "";
			}			
		}		
	} 
}

/* Searches for clamp keywords in stage 2, expects token to be lowercase 
 */
void ShaderTemplate::parseClamp(parser::DefTokeniser& tokeniser, const std::string& token) 
{	
	if (token == "zeroclamp") {
		_currentLayer.m_clampToBorder = true;
	}
}

/* Searches for the map keyword in stage 2, expects token to be lowercase 
 */
void ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, const std::string& token) 
{	
	if (token == "map") {
		_currentLayer.mapExpr = shaders::IMapExpression::createForToken(tokeniser);		
	}
}

// Search for colour modifications, e.g. red, green, blue, rgb or vertexColor
void ShaderTemplate::parseColourModulation(parser::DefTokeniser& tokeniser,
                                           const std::string& token)
{
    std::string tok_lc = boost::algorithm::to_lower_copy(token);
    if (tok_lc == "vertexcolor")
    {
        _currentLayer.vertexColourMode =
            ShaderLayer::VERTEX_COLOUR_MULTIPLY;
    }
    else if (tok_lc == "inversevertexcolor")
    {
        _currentLayer.vertexColourMode = 
            ShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY;
    }
    else if (tok_lc == "red" 
             || tok_lc == "green" 
             || tok_lc == "blue" 
             || tok_lc == "rgb")
    {
        // Get the colour value
        std::string valueString = tokeniser.nextToken();
        float value = strToFloat(valueString);

        // Set the appropriate component(s)
        if (tok_lc == "red")
        {
            _currentLayer.colour[0] = value;
        }
        else if (tok_lc == "green")
        {
            _currentLayer.colour[1] = value;
        }
        else if (tok_lc == "blue")
        {
            _currentLayer.colour[2] = value;
        }
        else
        {
            _currentLayer.colour = Vector3(value, value, value);
        }
    }
}

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the m_layers vector.  
 */
bool ShaderTemplate::saveLayer()
{
	switch (_currentLayer.m_type) {
		case LAYER_DIFFUSEMAP:
			_diffuse = _currentLayer.mapExpr;
			break;
		case LAYER_BUMPMAP:
			_bump = _currentLayer.mapExpr;
			break;
		case LAYER_SPECULARMAP:
			_specular = _currentLayer.mapExpr;
			break;
		default:
			if (_currentLayer.mapExpr) {
				m_layers.push_back(_currentLayer);
			}
	}
	
	// Clear the currentLayer structure for possible future layers
	_currentLayer.m_type = LAYER_NONE;
	_currentLayer.mapExpr = shaders::MapExpressionPtr();
	return true;
}

/* Parses a material definition for shader keywords and takes the according 
 * actions. 
 */
void ShaderTemplate::parseDefinition() 
{
	// Construct a local deftokeniser to parse the unparsed block
	parser::BasicDefTokeniser<std::string> tokeniser(
		_blockContents,
		" \t\n\v\r",	// delimiters (whitespace)
		"{}(),"			// add the comma character to the kept delimiters
	);

	_parsed = true; // we're parsed from now on

	try
	{
		int level = 1;	// we always start at top level
			
		while (level > 0 && tokeniser.hasMoreTokens()) 
        {
			std::string token = tokeniser.nextToken();
			std::string token_lowercase = boost::algorithm::to_lower_copy(token);
			
			if (token=="}") {
				
				if (--level == 1) {
					saveLayer();
				}
			} 
			else if (token=="{") {
				++level;
			}
			else {
				switch (level) {
					case 1: // global level
						parseShaderFlags(tokeniser, token_lowercase);
						parseLightFlags(tokeniser, token_lowercase);
						parseBlendShortcuts(tokeniser, token_lowercase);
						break;
					case 2: // stage level
						parseBlendType(tokeniser, token_lowercase);
						parseBlendMaps(tokeniser, token_lowercase);
						parseClamp(tokeniser, token_lowercase);
						break;
				}
			} 
		}

		// If the texture is missing (i.e. no editorimage provided), 
		// substitute this with the diffusemap
		if (!_texture) {		
			_texture = _diffuse;
		}
	}
	catch (parser::ParseException p) {
		globalErrorStream() << "Error while parsing shader " << _name << ": "
			<< p.what() << std::endl;
	}
}
