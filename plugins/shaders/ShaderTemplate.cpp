#include "ShaderTemplate.h"
#include "MapExpression.h"

#include "itextstream.h"

#include "os/path.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

namespace shaders
{

/*  Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable m_nFlags
 * 
 *  Note: input "token" has to be lowercase for the keywords to be recognized
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
        _texture = IMapExpression::createForToken(tokeniser);
    }
    else if (token == "diffusemap") {
        _diffuseLayer->setLayerType(ShaderLayer::DIFFUSE);
        _diffuseLayer->setMapExpression(
            IMapExpression::createForToken(tokeniser)
        );
        m_layers.push_back(_diffuseLayer);
    }
    else if (token == "specularmap") {
        _specularLayer->setLayerType(ShaderLayer::SPECULAR);
        _specularLayer->setMapExpression(
            IMapExpression::createForToken(tokeniser)
        );
        m_layers.push_back(_specularLayer);
    }
    else if (token == "bumpmap") {
        _bumpLayer->setLayerType(ShaderLayer::BUMP);
        _bumpLayer->setMapExpression(
            IMapExpression::createForToken(tokeniser)
        );
        m_layers.push_back(_bumpLayer);
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
            _currentLayer->setLayerType(ShaderLayer::DIFFUSE);
        }
        else if (blendType == "bumpmap") {
            _currentLayer->setLayerType(ShaderLayer::BUMP);
        }
        else if (blendType == "specularmap") {
            _currentLayer->setLayerType(ShaderLayer::SPECULAR);
        }
        else 
        {
            // Special blend type, either predefined like "add" or "modulate",
            // or an explicit combination of GL blend modes
            StringPair blendFuncStrings;
            blendFuncStrings.first = blendType;
            
            if (blendType.substr(0,3) == "gl_") 
            {
                // This is an explicit GL blend mode
                tokeniser.assertNextToken(",");
                blendFuncStrings.second = tokeniser.nextToken();
            } else {
                blendFuncStrings.second = "";
            }           
            _currentLayer->setBlendFuncStrings(blendFuncStrings);
        }       
    } 
}

/* Searches for the map keyword in stage 2, expects token to be lowercase 
 */
void ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, const std::string& token) 
{   
    if (token == "map") {
        _currentLayer->setMapExpression(IMapExpression::createForToken(tokeniser));     
    }
}

// Search for colour modifications, e.g. red, green, blue, rgb or vertexColor
void ShaderTemplate::parseColourModulation(parser::DefTokeniser& tokeniser,
                                           const std::string& token)
{
    if (token == "vertexcolor")
    {
        _currentLayer->setVertexColourMode(
            ShaderLayer::VERTEX_COLOUR_MULTIPLY
        );
    }
    else if (token == "inversevertexcolor")
    {
        _currentLayer->setVertexColourMode( 
            ShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY
        );
    }
    else if (token == "red" 
             || token == "green" 
             || token == "blue" 
             || token == "rgb")
    {
        // Get the colour value
        std::string valueString = tokeniser.nextToken();
        float value = strToFloat(valueString);

        // Set the appropriate component(s)
        Vector3 currentColour = _currentLayer->getColour();
        if (token == "red")
        {
            currentColour[0] = value;
        }
        else if (token == "green")
        {
            currentColour[1] = value;
        }
        else if (token == "blue")
        {
            currentColour[2] = value;
        }
        else
        {
            currentColour = Vector3(value, value, value);
        }
        _currentLayer->setColour(currentColour);
    }
}

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the m_layers vector.  
 */
bool ShaderTemplate::saveLayer()
{
    // If the current layer is a special layer, save it to the respective member
    // variable, otherwise add it to the layer list
    switch (_currentLayer->getType()) 
    {
        case ShaderLayer::DIFFUSE:
            _diffuseLayer = _currentLayer;
            break;
        case ShaderLayer::BUMP:
            _bumpLayer = _currentLayer;
            break;
        case ShaderLayer::SPECULAR:
            _specularLayer = _currentLayer;
            break;
        default:
            // Do nothing
            break;
    }

    // Append layer to list of all layers
    if (_currentLayer->getMapExpression()) {
        m_layers.push_back(_currentLayer);
    }
    
    // Clear the currentLayer structure for possible future layers
    _currentLayer = Doom3ShaderLayerPtr(new Doom3ShaderLayer);
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
        " \t\n\v\r",    // delimiters (whitespace)
        "{}(),"         // add the comma character to the kept delimiters
    );

    _parsed = true; // we're parsed from now on

    try
    {
        int level = 1;  // we always start at top level
            
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
                        parseColourModulation(tokeniser, token_lowercase);
                        break;
                }
            } 
        }

        // If the texture is missing (i.e. no editorimage provided), 
        // substitute this with the diffusemap
        if (!_texture) {        
            _texture = _diffuseLayer->getMapExpression();
        }
    }
    catch (parser::ParseException p) {
        globalErrorStream() << "Error while parsing shader " << _name << ": "
            << p.what() << std::endl;
    }
}

}

