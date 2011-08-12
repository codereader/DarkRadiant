#include "ShaderTemplate.h"
#include "MapExpression.h"
#include "CameraCubeMapDecl.h"

#include "itextstream.h"

#include "os/path.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>

#include "ShaderExpression.h"

namespace shaders
{

NamedBindablePtr ShaderTemplate::getEditorTexture()
{
    if (!_parsed)
        parseDefinition();

    return _editorTex;
}

/*  Searches a token for known shaderflags (e.g. "translucent") and sets the flags
 *  in the member variable _materialFlags
 *
 *  Note: input "token" has to be lowercase for the keywords to be recognized
 */
bool ShaderTemplate::parseShaderFlags(parser::DefTokeniser& tokeniser,
                                      const std::string& token)
{
    if (token == "translucent")
	{
        _materialFlags |= Material::FLAG_TRANSLUCENT;
    }
    else if (token == "decal_macro")
	{
        _materialFlags |= Material::FLAG_TRANSLUCENT;
        _sortReq = Material::SORT_DECAL;
        _polygonOffset = 1.0f;
		_surfaceFlags |= Material::SURF_DISCRETE;
    }
    else if (token == "twosided")
	{
        _cullType = Material::CULL_NONE;
    }
	else if (token == "backsided")
	{
        _cullType = Material::CULL_FRONT;
    }
    else if (token == "description")
	{
        // greebo: Parse description token, this should be the next one
        description = tokeniser.nextToken();
    }
	else if (token == "polygonoffset")
	{
		_polygonOffset = strToFloat(tokeniser.nextToken(), 0);
	}
	else if (token == "clamp")
	{
		_clampType = CLAMP_NOREPEAT;
	}
	else if (token == "zeroclamp")
	{
		_clampType = CLAMP_ZEROCLAMP;
	}
	else if (token == "alphazeroclamp")
	{
		_clampType = CLAMP_ALPHAZEROCLAMP;
	}
	else if (token == "sort")
	{
		std::string sortVal = tokeniser.nextToken();

		if (sortVal == "opaque")
		{
			_sortReq = Material::SORT_OPAQUE;
		}
		else if (sortVal == "decal")
		{
			_sortReq = Material::SORT_DECAL;
		}
		else if (sortVal == "portalSky")
		{
			_sortReq = Material::SORT_PORTAL_SKY;
		}
		else if (sortVal == "subview")
		{
			_sortReq = Material::SORT_SUBVIEW;
		}
		else if (sortVal == "far")
		{
			_sortReq = Material::SORT_FAR;
		}
		else if (sortVal == "medium")
		{
			_sortReq = Material::SORT_MEDIUM;
		}
		else if (sortVal == "close")
		{
			_sortReq = Material::SORT_CLOSE;
		}
		else if (sortVal == "almostNearest")
		{
			_sortReq = Material::SORT_ALMOST_NEAREST;
		}
		else if (sortVal == "nearest")
		{
			_sortReq = Material::SORT_NEAREST;
		}
		else if (sortVal == "postProcess")
		{
			_sortReq = Material::SORT_POST_PROCESS;
		}
		else // no special sort keyword, try to parse the numeric value
		{
			//  Strip any quotes
			boost::algorithm::trim_if(sortVal, boost::algorithm::is_any_of("\""));

			_sortReq = strToInt(sortVal, SORT_UNDEFINED); // fall back to UNDEFINED in case of parsing failures
		}
	}
	else if (token == "noshadows")
	{
		_materialFlags |= Material::FLAG_NOSHADOWS;
	}
	else if (token == "noselfshadow")
	{
		_materialFlags |= Material::FLAG_NOSELFSHADOW;
	}
	else if (token == "forceshadows")
	{
		_materialFlags |= Material::FLAG_FORCESHADOWS;
	}
	else if (token == "nooverlays")
	{
		_materialFlags |= Material::FLAG_NOOVERLAYS;
	}
	else if (token == "forceoverlays")
	{
		_materialFlags |= Material::FLAG_FORCEOVERLAYS;
	}
	else if (token == "forceopaque")
	{
		_materialFlags |= Material::FLAG_FORCEOPAQUE;
	}
	else if (token == "nofog")
	{
		_materialFlags |= Material::FLAG_NOFOG;
	}
	else if (token == "unsmoothedtangents")
	{
		_materialFlags |= Material::FLAG_UNSMOOTHEDTANGENTS;
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true; // token recognised
}

/* Searches for light-specific keywords and takes the appropriate actions
 */
bool ShaderTemplate::parseLightKeywords(parser::DefTokeniser& tokeniser, const std::string& token)
{
    if (token == "ambientlight")
	{
        ambientLight = true;
    }
    else if (token == "blendlight")
	{
        blendLight = true;
    }
    else if (token == "foglight")
	{
        fogLight = true;
    }
    else if (!fogLight && token == "lightfalloffimage")
	{
        _lightFalloff = MapExpression::createForToken(tokeniser);
    }
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

// Parse any single-line stages (such as "diffusemap x/y/z")
bool ShaderTemplate::parseBlendShortcuts(parser::DefTokeniser& tokeniser,
                                         const std::string& token)
{
    if (token == "qer_editorimage")
    {
        _editorTex = MapExpression::createForToken(tokeniser);
    }
    else if (token == "diffusemap")
    {
        addLayer(ShaderLayer::DIFFUSE, MapExpression::createForToken(tokeniser));
    }
    else if (token == "specularmap")
    {
		addLayer(ShaderLayer::SPECULAR, MapExpression::createForToken(tokeniser));
    }
    else if (token == "bumpmap")
    {
		addLayer(ShaderLayer::BUMP, MapExpression::createForToken(tokeniser));
    }
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

/* Parses for possible blend commands like "add", "diffusemap", "gl_one, gl_zero" etc.
 * Note: input "token" has to be lowercase
 * Output: true, if the blend keyword was found, false otherwise.
 */
bool ShaderTemplate::parseBlendType(parser::DefTokeniser& tokeniser, const std::string& token)
{
    if (token == "blend")
    {
        std::string blendType = boost::algorithm::to_lower_copy(tokeniser.nextToken());

        if (blendType == "diffusemap")
		{
            _currentLayer->setLayerType(ShaderLayer::DIFFUSE);
        }
        else if (blendType == "bumpmap")
		{
            _currentLayer->setLayerType(ShaderLayer::BUMP);
        }
        else if (blendType == "specularmap")
		{
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
            }
			else
			{
                blendFuncStrings.second = "";
            }

            _currentLayer->setBlendFuncStrings(blendFuncStrings);
        }
    }
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

/* Searches for the map keyword in stage 2, expects token to be lowercase
 */
bool ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, const std::string& token)
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
	else if (token == "texgen")
	{
		std::string type = tokeniser.nextToken();

		if (type == "skybox")
		{
			_currentLayer->setTexGenType(ShaderLayer::TEXGEN_SKYBOX);
		}
		else if (type == "reflect")
		{
			_currentLayer->setTexGenType(ShaderLayer::TEXGEN_REFLECT);
		}
		else if (type == "normal")
		{
			_currentLayer->setTexGenType(ShaderLayer::TEXGEN_NORMAL);
		}
		else if (type == "wobblesky")
		{
			_currentLayer->setTexGenType(ShaderLayer::TEXGEN_WOBBLESKY);

			// Parse the 3 wobblesky parameters
			// greebo: The D3 SDK says that registers could be used here (to support expressions), 
			// but no D3 material uses an expression for the texgen parameters
			for (std::size_t i = 0; i < 3; ++i)
			{
				_currentLayer->setTexGenParam(i, strToFloat(tokeniser.nextToken()));
			}
		}
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

bool ShaderTemplate::parseStageModifiers(parser::DefTokeniser& tokeniser,
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
	else if (token == "red")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RED, expr);
	}
	else if (token == "green")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_GREEN, expr);
	}
	else if (token == "blue")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_BLUE, expr);
	}
	else if (token == "alpha")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_ALPHA, expr);
	}
	else if (token == "rgb")
	{
		// Get the colour value
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RGB, expr);
	}
	else if (token == "rgba")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RGBA, expr);
	}
    else if (token == "alphatest")
    {
		// Get the alphatest expression
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		       
        _currentLayer->setAlphaTest(expr);
    }
	else if (token == "scale")
	{
		IShaderExpressionPtr xScaleExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yScaleExpr = ShaderExpression::createFromTokens(tokeniser);

		_currentLayer->setScale(xScaleExpr, yScaleExpr);
	}
	else if (token == "translate" || token == "scroll")
	{
		IShaderExpressionPtr xTranslateExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yTranslateExpr = ShaderExpression::createFromTokens(tokeniser);

		_currentLayer->setTranslation(xTranslateExpr, yTranslateExpr);
	}
	else if (token == "rotate")
	{
		IShaderExpressionPtr rotExpr = ShaderExpression::createFromTokens(tokeniser);

		_currentLayer->setRotation(rotExpr);
	}
	else if (token == "colored")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_COLOURED);
	}
	else if (token == "clamp")
	{
		_currentLayer->setClampType(CLAMP_NOREPEAT);
	}
	else if (token == "zeroclamp")
	{
		_currentLayer->setClampType(CLAMP_ZEROCLAMP);
	}
	else if (token == "alphazeroclamp")
	{
		_currentLayer->setClampType(CLAMP_ALPHAZEROCLAMP);
	}
	else if (token == "noclamp")
	{
		_currentLayer->setClampType(CLAMP_REPEAT);
	}
	else if (token == "uncompressed" || token == "highquality")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_HIGHQUALITY);
	}
	else if (token == "forcehighquality")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_FORCE_HIGHQUALITY);
	}
	else if (token == "nopicmip")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_NO_PICMIP);
	}
	else if (token == "maskred")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_RED);
	}
	else if (token == "maskgreen")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_GREEN);
	}
	else if (token == "maskblue")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_BLUE);
	}
	else if (token == "maskalpha")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_ALPHA);
	}
	else if (token == "maskcolor")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_RED);
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_GREEN);
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_BLUE);
	}
	else if (token == "maskdepth")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_MASK_DEPTH);
	}
	else if (token == "nearest")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_FILTER_NEAREST);
	}
	else if (token == "linear")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_FILTER_LINEAR);
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

bool ShaderTemplate::parseSurfaceFlags(parser::DefTokeniser& tokeniser,
                                       const std::string& token)
{
    if (token == "solid")
    {
        _surfaceFlags |= Material::SURF_SOLID;
    }
    else if (token == "water")
    {
        _surfaceFlags |= Material::SURF_WATER;
    }
    else if (token == "playerclip")
    {
        _surfaceFlags |= Material::SURF_PLAYERCLIP;
    }
    else if (token == "monsterclip")
    {
        _surfaceFlags |= Material::SURF_MONSTERCLIP;
    }
	else if (token == "moveableclip")
    {
        _surfaceFlags |= Material::SURF_MOVEABLECLIP;
    }
	else if (token == "ikclip")
    {
        _surfaceFlags |= Material::SURF_IKCLIP;
    }
	else if (token == "blood")
    {
        _surfaceFlags |= Material::SURF_BLOOD;
    }
	else if (token == "trigger")
    {
        _surfaceFlags |= Material::SURF_TRIGGER;
    }
	else if (token == "aassolid")
    {
		_surfaceFlags |= Material::SURF_AASSOLID;
    }
	else if (token == "aasobstacle")
    {
        _surfaceFlags |= Material::SURF_AASOBSTACLE;
    }
	else if (token == "flashlight_trigger")
    {
        _surfaceFlags |= Material::SURF_FLASHLIGHT_TRIGGER;
    }
	else if (token == "nonsolid")
    {
        _surfaceFlags |= Material::SURF_NONSOLID;
    }
	else if (token == "nullnormal")
    {
        _surfaceFlags |= Material::SURF_NULLNORMAL;
    }
	else if (token == "areaportal")
    {
		_surfaceFlags |= Material::SURF_AREAPORTAL;
    }
	else if (token == "qer_nocarve")
    {
		_surfaceFlags |= Material::SURF_NOCARVE;
    }
	else if (token == "discrete")
    {
        _surfaceFlags |= Material::SURF_DISCRETE;
    }
	else if (token == "nofragment")
    {
        _surfaceFlags |= Material::SURF_NOFRAGMENT;
    }
	else if (token == "slick")
    {
        _surfaceFlags |= Material::SURF_SLICK;
    }
	else if (token == "collision")
    {
        _surfaceFlags |= Material::SURF_COLLISION;
    }
	else if (token == "noimpact")
    {
        _surfaceFlags |= Material::SURF_NOIMPACT;
    }
	else if (token == "nodamage")
    {
        _surfaceFlags |= Material::SURF_NODAMAGE;
    }
	else if (token == "ladder")
    {
        _surfaceFlags |= Material::SURF_LADDER;
    }
	else if (token == "nosteps")
    {
        _surfaceFlags |= Material::SURF_NOSTEPS;
    }
	else if (token == "metal")
    {
		_surfaceType = Material::SURFTYPE_METAL;
    }
	else if (token == "stone")
    {
        _surfaceType = Material::SURFTYPE_STONE;
    }
	else if (token == "flesh")
    {
        _surfaceType = Material::SURFTYPE_FLESH;
    }
	else if (token == "wood")
    {
        _surfaceType = Material::SURFTYPE_WOOD;
    }
	else if (token == "cardboard")
    {
        _surfaceType = Material::SURFTYPE_CARDBOARD;
    }
	else if (token == "liquid")
    {
        _surfaceType = Material::SURFTYPE_LIQUID;
    }
	else if (token == "glass")
    {
        _surfaceType = Material::SURFTYPE_GLASS;
    }
	else if (token == "plastic")
    {
        _surfaceType = Material::SURFTYPE_PLASTIC;
    }
	else if (token == "ricochet")
    {
        _surfaceType = Material::SURFTYPE_RICOCHET;
    }
	else if (token == "surftype10")
    {
        _surfaceType = Material::SURFTYPE_10;
    }
	else if (token == "surftype11")
    {
        _surfaceType = Material::SURFTYPE_11;
    }
	else if (token == "surftype12")
    {
        _surfaceType = Material::SURFTYPE_12;
    }
	else if (token == "surftype13")
    {
        _surfaceType = Material::SURFTYPE_13;
    }
	else if (token == "surftype14")
    {
        _surfaceType = Material::SURFTYPE_14;
    }
	else if (token == "surftype15")
    {
        _surfaceType = Material::SURFTYPE_15;
    }
	else if (token == "guisurf")
	{
		// Something like "guisurf blah.gui" or "guisurf entity2", skip the argument and proceed
		tokeniser.skipTokens(1);
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

bool ShaderTemplate::parseCondition(parser::DefTokeniser& tokeniser, const std::string& token)
{
	if (token == "if")
	{
		// Parse condition
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		
		_currentLayer->setCondition(expr);

		return true;
	}
	else
	{
		return false;
	}
}

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the m_layers vector.
 */
bool ShaderTemplate::saveLayer()
{
    // Append layer to list of all layers
    if (_currentLayer->getBindableTexture())
    {
		addLayer(_currentLayer);
    }

    // Clear the currentLayer structure for possible future layers
    _currentLayer = Doom3ShaderLayerPtr(new Doom3ShaderLayer(*this));

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
		parser::WHITESPACE, // delimiters (whitespace)
        "{}(),"  // add the comma character to the kept delimiters
    );

    _parsed = true; // we're parsed from now on

    try
    {
        int level = 1;  // we always start at top level

        while (level > 0 && tokeniser.hasMoreTokens())
        {
            std::string token = tokeniser.nextToken();
            
            if (token == "}")
			{
                if (--level == 1)
				{
                    saveLayer();
                }
            }
            else if (token == "{")
			{
                ++level;
            }
            else
			{
				boost::algorithm::to_lower(token);

                switch (level)
				{
                    case 1: // global level
                        if (parseShaderFlags(tokeniser, token)) continue;
                        if (parseLightKeywords(tokeniser, token)) continue;
                        if (parseBlendShortcuts(tokeniser, token)) continue;
						if (parseSurfaceFlags(tokeniser, token)) continue;

						globalWarningStream() << "Material keyword not recognised: " << token << std::endl;

                        break;
                    case 2: // stage level
						if (parseCondition(tokeniser, token)) continue;
                        if (parseBlendType(tokeniser, token)) continue;
                        if (parseBlendMaps(tokeniser, token)) continue;
                        if (parseStageModifiers(tokeniser, token)) continue;

						globalWarningStream() << "Stage keyword not recognised: " << token << std::endl;

                        break;
                }
            }
        }
    }
    catch (parser::ParseException& p)
	{
        globalErrorStream() << "Error while parsing shader " << _name << ": "
            << p.what() << std::endl;
    }

	// greebo: It appears that D3 is applying default sort values for material without
	// an explicitly defined sort value, depending on a couple of things I didn't really investigate
	// Some blend materials get SORT_MEDIUM applied by default, diffuses get OPAQUE assigned, but lights do not, etc.
	if (_sortReq == SORT_UNDEFINED)
	{
		_sortReq = Material::SORT_OPAQUE;
	}
}

void ShaderTemplate::addLayer(const Doom3ShaderLayerPtr& layer)
{
	// Add the layer
	m_layers.push_back(layer);

	// If there is no editor texture yet, use the bindable texture, but no Bump or speculars
	if (!_editorTex && layer->getBindableTexture() != NULL &&
		layer->getType() != ShaderLayer::BUMP && layer->getType() != ShaderLayer::SPECULAR)
	{
		_editorTex = layer->getBindableTexture();
	}
}

void ShaderTemplate::addLayer(ShaderLayer::Type type, const MapExpressionPtr& mapExpr)
{
	// Construct a layer out of this mapexpression and pass the call
	addLayer(Doom3ShaderLayerPtr(new Doom3ShaderLayer(*this, type, mapExpr)));
}

bool ShaderTemplate::hasDiffusemap()
{
	if (!_parsed) parseDefinition();

	for (Layers::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i)
    {
        if ((*i)->getType() == ShaderLayer::DIFFUSE)
        {
            return true;
        }
    }

	return false; // no diffuse found
}

} // namespace

