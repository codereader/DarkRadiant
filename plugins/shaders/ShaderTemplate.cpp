#include "ShaderTemplate.h"
#include "MapExpression.h"
#include "CameraCubeMapDecl.h"

#include "itextstream.h"

#include "os/path.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

namespace shaders
{

NamedBindablePtr ShaderTemplate::getEditorTexture()
{
    if (!_parsed) 
        parseDefinition();

    return _editorTex;
}

/*  Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable m_nFlags
 * 
 *  Note: input "token" has to be lowercase for the keywords to be recognized
 */
void ShaderTemplate::parseShaderFlags(parser::DefTokeniser& tokeniser,
                                      const std::string& token) 
{   
    if (token == "qer_trans") {
        m_nFlags |= QER_TRANS;
    }
    else if (token == "translucent") {
        m_nFlags |= QER_TRANS;
    } 
    else if (token == "decal_macro") {
        m_nFlags |= QER_TRANS;
        _sortReq = Material::SORT_DECAL;
    } 
    else if (token == "twosided") {
        m_Cull = Material::eCullNone;
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
        _lightFalloff = shaders::MapExpression::createForToken(tokeniser);
    }
}

// Parse any single-line stages (such as "diffusemap x/y/z")
void ShaderTemplate::parseBlendShortcuts(parser::DefTokeniser& tokeniser, 
                                         const std::string& token) 
{
    if (token == "qer_editorimage") 
    {
        _editorTex = MapExpression::createForToken(tokeniser);
    }
    else if (token == "diffusemap") 
    {
        // Parse the map expression
        MapExpressionPtr difMapExp = MapExpression::createForToken(tokeniser);

        // Add the diffuse layer
        Doom3ShaderLayerPtr layer(
            new Doom3ShaderLayer(ShaderLayer::DIFFUSE, difMapExp)
        );
        m_layers.push_back(layer);

        // If there is no editor texture set, use the diffusemap texture instead
        if (!_editorTex)
        {
            _editorTex = difMapExp;
        }
    }
    else if (token == "specularmap") 
    {
        Doom3ShaderLayerPtr layer(
            new Doom3ShaderLayer(
                ShaderLayer::SPECULAR, 
                MapExpression::createForToken(tokeniser)
            )
        );
        m_layers.push_back(layer);
    }
    else if (token == "bumpmap") 
    {
        Doom3ShaderLayerPtr layer(
            new Doom3ShaderLayer(
                ShaderLayer::BUMP,
                MapExpression::createForToken(tokeniser)
            )
        );
        m_layers.push_back(layer);
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
    if (token == "map") 
    {
        _currentLayer->setBindableTexture(
            MapExpression::createForToken(tokeniser)
        );     
    }
    else if (token == "cameracubemap")
    {
        std::string cubeMapPrefix = tokeniser.nextToken();
        _currentLayer->setBindableTexture(
            CameraCubeMapDecl::createForPrefix(cubeMapPrefix)
        );
        _currentLayer->setCubeMapMode(ShaderLayer::CUBE_MAP_CAMERA);
    }
}

// Search for colour modifications, e.g. red, green, blue, rgb or vertexColor
void ShaderTemplate::parseStageModifiers(parser::DefTokeniser& tokeniser,
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
    else if (token == "alphatest")
    {
        // Get the alphatest value
        std::string valueStr = tokeniser.nextToken();
        _currentLayer->setAlphaTest(strToFloat(valueStr));
    }
}

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the m_layers vector.  
 */
bool ShaderTemplate::saveLayer()
{
    // Append layer to list of all layers
    if (_currentLayer->getBindableTexture()) 
    {
        m_layers.push_back(_currentLayer);
    }

    // If the layer we just saved was a diffusemap layer, and there is no
    // editorimage, use the diffusemap as editor image
    if (_currentLayer->getType() == ShaderLayer::DIFFUSE
        && !_editorTex)
    {
        _editorTex = _currentLayer->getBindableTexture();
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
                        parseStageModifiers(tokeniser, token_lowercase);
                        break;
                }
            } 
        }
    }
    catch (parser::ParseException p) {
        globalErrorStream() << "Error while parsing shader " << _name << ": "
            << p.what() << std::endl;
    }
}

}

