#include "ShaderTemplate.h"
#include "MapExpression.h"
#include "CameraCubeMapDecl.h"

#include "itextstream.h"

#include "os/path.h"
#include "string/convert.h"
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

IShaderExpressionPtr ShaderTemplate::parseSingleExpressionTerm(parser::DefTokeniser& tokeniser)
{
	std::string token = tokeniser.nextToken();

	if (token == "(")
	{
		std::string expr = token;
		std::size_t level = 1;

		while (level > 0)
		{
			token = tokeniser.nextToken();
			expr += token;

			if (token == ")")
			{
				--level;
			}
			else if (token == "(")
			{
				++level;
			}
		}

		return ShaderExpression::createFromString(expr);
	}
	else
	{
		// No parenthesis, parse this token alone
		return ShaderExpression::createFromString(token);
	}
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
        _materialFlags |= Material::FLAG_TRANSLUCENT | Material::FLAG_NOSHADOWS;
		_coverage = Material::MC_TRANSLUCENT;
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
		_polygonOffset = string::convert<float>(tokeniser.nextToken(), 0);
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

			_sortReq = string::convert<int>(sortVal, SORT_UNDEFINED); // fall back to UNDEFINED in case of parsing failures
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
		_coverage = Material::MC_OPAQUE;
	}
	else if (token == "nofog")
	{
		_materialFlags |= Material::FLAG_NOFOG;
	}
	else if (token == "noportalfog")
	{
		_materialFlags |= Material::FLAG_NOPORTALFOG;
	}
	else if (token == "unsmoothedtangents")
	{
		_materialFlags |= Material::FLAG_UNSMOOTHEDTANGENTS;
	}
	else if (token == "mirror")
	{
		_materialFlags |= Material::FLAG_MIRROR;
		_coverage = Material::MC_OPAQUE;
	}
	else if (token == "decalinfo")
	{
		// Syntax: decalInfo <staySeconds> <fadeSeconds> [start rgb] [end rgb]
		// Example: decalInfo 10 5 ( 1 1 1 1 ) ( 0 0 0 0 )
		_decalInfo.stayMilliSeconds = static_cast<int>(string::convert<float>(tokeniser.nextToken()) * 1000);
		_decalInfo.fadeMilliSeconds = static_cast<int>(string::convert<float>(tokeniser.nextToken()) * 1000);

		// Start colour
		tokeniser.assertNextToken("(");

		_decalInfo.startColour.x() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.startColour.y() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.startColour.z() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.startColour.w() = string::convert<float>(tokeniser.nextToken());

		tokeniser.assertNextToken(")");

		// End colour
		tokeniser.assertNextToken("(");

		_decalInfo.endColour.x() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.endColour.y() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.endColour.z() = string::convert<float>(tokeniser.nextToken());
		_decalInfo.endColour.w() = string::convert<float>(tokeniser.nextToken());

		tokeniser.assertNextToken(")");
	}
	else if (token == "deform")
	{
		std::string type = tokeniser.nextToken();
		boost::algorithm::to_lower(type);

		if (type == "sprite")
		{
			_deformType = Material::DEFORM_SPRITE;
		}
		else if (type == "tube")
		{
			_deformType = Material::DEFORM_TUBE;
		}
		else if (type == "flare")
		{
			_deformType = Material::DEFORM_FLARE;

			parseSingleExpressionTerm(tokeniser); // skip size info
		}
		else if (type == "expand")
		{
			_deformType = Material::DEFORM_EXPAND;

			parseSingleExpressionTerm(tokeniser); // skip amount
		}
		else if (type == "move")
		{
			_deformType = Material::DEFORM_MOVE;

			parseSingleExpressionTerm(tokeniser); // skip amount
		}
		else if (type == "turbulent")
		{
			_deformType = Material::DEFORM_TURBULENT;

			tokeniser.skipTokens(1); // skip table name

			parseSingleExpressionTerm(tokeniser); // range
			parseSingleExpressionTerm(tokeniser); // timeoffset
			parseSingleExpressionTerm(tokeniser); // domain
		}
		else if (type == "eyeball")
		{
			_deformType = Material::DEFORM_EYEBALL;
		}
		else if (type == "particle")
		{
			_deformType = Material::DEFORM_PARTICLE;

			tokeniser.skipTokens(1); // skip particle name
		}
		else if (type == "particle2")
		{
			_deformType = Material::DEFORM_PARTICLE2;

			tokeniser.skipTokens(1); // skip particle name
		}
	}
	else if (token == "renderbump")
	{
		// Skip over this renderbump directive
		// Syntax: renderBump [-size <width> <height>] [-aa <0/1/2>] [-trace <0.01 - 1.0>] <normalMapImage> <highPolyModel>
		std::string next = tokeniser.nextToken();
		boost::algorithm::to_lower(next);

		// Skip over the optional args
		while (next.length() > 0 && next[0] == '-')
		{
			if (next == "-size")
			{
				tokeniser.skipTokens(2); // skip width, height
			}
			else if (next == "-aa")
			{
				tokeniser.skipTokens(1);
			}
			else if (next == "-trace")
			{
				tokeniser.skipTokens(1);
			}

			next = tokeniser.nextToken();
			boost::algorithm::to_lower(next);
		}

		// The map token is already loaded in "next", skip the highpoly model name
		tokeniser.skipTokens(1);
	}
	else if (token == "renderbumpflat")
	{
		// Skip over this renderbump directive
		// Syntax: RenderBumpFlat [-size <width> <height>] <modelfile>
		std::string next = tokeniser.nextToken();
		boost::algorithm::to_lower(next);

		// Skip over the optional args
		if (next == "-size")
		{
			tokeniser.skipTokens(3); // skip width, height and model
		}
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
	else if (token == "spectrum")
	{
		std::string value = tokeniser.nextToken();

		try
		{
			_spectrum = boost::lexical_cast<int>(value);
		}
		catch (boost::bad_lexical_cast& e)
		{
			rWarning() << "Expect integer number as spectrum value, found " << 
				value << ": " << e.what() << std::endl;
		}
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
				_currentLayer->setTexGenParam(i, string::convert<float>(tokeniser.nextToken()));
			}
		}
	}
	else if (token == "cubemap")
    {
		// Parse the cubemap expression, but don't do anything with it for now
		MapExpression::createForToken(tokeniser);
    }
	else if (token == "videomap")
    {
		std::string nextToken = tokeniser.nextToken();
		boost::algorithm::to_lower(nextToken);

		if (nextToken == "loop")
		{
			// Skip looping keyword and ignore the videomap expression
			tokeniser.skipTokens(1);
		}
    }
	else if (token == "soundmap")
	{
		// This stage should render as sound meter/map - skip this information for now
		if (tokeniser.peek() == "waveform")
		{
			tokeniser.skipTokens(1);
		}
	}
	else if (token == "remoterendermap")
	{
		try
		{
			string::convert<int>(tokeniser.nextToken());
			string::convert<int>(tokeniser.nextToken());
		}
		catch (boost::bad_lexical_cast& e)
		{
			rWarning() << "Error parsing remoteRenderMap. Expected two integers: " 
				<< e.what() << std::endl;
		}
	}
	else if (token == "mirrorrendermap")
	{
		try
		{
			string::convert<int>(tokeniser.nextToken());
			string::convert<int>(tokeniser.nextToken());
		}
		catch (boost::bad_lexical_cast& e)
		{
			rWarning() << "Error parsing mirrorRenderMap. Expected two integers: "
				<< e.what() << std::endl;
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
		
		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RED, expr);
		}
		else
		{
			rWarning() << "Could not parse red expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "green")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);

		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_GREEN, expr);
		}
		else
		{
			rWarning() << "Could not parse green expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "blue")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		
		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_BLUE, expr);
		}
		else
		{
			rWarning() << "Could not parse blue expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "alpha")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		
		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_ALPHA, expr);
		}
		else
		{
			rWarning() << "Could not parse alpha expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "color")
	{
		// color <exp0>, <exp1>, <exp2>, <exp3>
		IShaderExpressionPtr red = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr green = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr blue = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr alpha = ShaderExpression::createFromTokens(tokeniser);

		if (red && green && blue && alpha)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RED, red);
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_GREEN, green);
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_BLUE, blue);
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_ALPHA, alpha);
		}
		else
		{
			rWarning() << "Could not parse color expressions in shader: " << getName() << std::endl;
		}
	}
	else if (token == "rgb")
	{
		// Get the colour value
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);

		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RGB, expr);
		}
		else
		{
			rWarning() << "Could not parse rgb expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "rgba")
	{
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);

		if (expr)
		{
			_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RGBA, expr);
		}
		else
		{
			rWarning() << "Could not parse rgba expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "fragmentprogram")
	{
		_currentLayer->setFragmentProgram(tokeniser.nextToken());
	}
	else if (token == "vertexprogram")
	{
		_currentLayer->setVertexProgram(tokeniser.nextToken());
	}
	else if (token == "program")
	{
		std::string prog = tokeniser.nextToken();
		_currentLayer->setFragmentProgram(prog);
		_currentLayer->setVertexProgram(prog);
	}
	else if (token == "vertexparm")
	{
		// vertexParm		<parmNum>		<parm1> [,<parm2>] [,<parm3>] [,<parm4>]
		int parmNum = string::convert<int>(tokeniser.nextToken());

		IShaderExpressionPtr parm0 = ShaderExpression::createFromTokens(tokeniser);

		if (tokeniser.peek() == ",")
		{
			tokeniser.nextToken();

			IShaderExpressionPtr parm1 = ShaderExpression::createFromTokens(tokeniser);

			if (tokeniser.peek() == ",")
			{
				tokeniser.nextToken();

				IShaderExpressionPtr parm2 = ShaderExpression::createFromTokens(tokeniser);

				if (tokeniser.peek() == ",")
				{
					tokeniser.nextToken();

					IShaderExpressionPtr parm3 = ShaderExpression::createFromTokens(tokeniser);

					// All 4 layers specified
					_currentLayer->setVertexParm(parmNum, parm0, parm1, parm2, parm3);
				}
				else
				{
					// Pass NULL as fourth component, it will be set to 1 by the shaderlayer
					_currentLayer->setVertexParm(parmNum, parm0, parm1, parm2);
				}
			}
			else
			{
				// Pass NULL as components z and w, the shaderlayer will set z and w to 0, 1
				_currentLayer->setVertexParm(parmNum, parm0, parm1);
			}
		}
		else
		{
			// Pass only one component, the shaderlayer will repeat the first parm 4 times
			_currentLayer->setVertexParm(parmNum, parm0);
		}
	}
	else if (token == "fragmentmap")
	{
		// fragmentMap <index> [options] <map>
		int mapNum = string::convert<int>(tokeniser.nextToken());

		std::string next = tokeniser.peek();
		boost::algorithm::to_lower(next);

		// These are all valid option keywords
		while (next == "cubemap" || next == "cameracubemap" || next == "nearest" ||
			next == "linear" || next == "clamp" || next == "noclamp" ||
			next == "zeroclamp" || next == "alphazeroclamp" || next == "forcehighquality" ||
			next == "uncompressed" || next == "highquality" || next == "nopicmip")
		{
			tokeniser.nextToken();

			next = tokeniser.peek();
			boost::algorithm::to_lower(next);
		}

		// Get the map expression (but don't really use it)
		_currentLayer->setFragmentMap(mapNum, MapExpression::createForToken(tokeniser));
	}
    else if (token == "alphatest")
    {
		// Get the alphatest expression
		IShaderExpressionPtr expr = ShaderExpression::createFromTokens(tokeniser);
		   
		if (expr)
		{
			_currentLayer->setAlphaTest(expr);
		}
		else
		{
			rWarning() << "Could not parse alphatest expression in shader: " << getName() << std::endl;
		}

		_coverage = Material::MC_PERFORATED;
    }
	else if (token == "scale")
	{
		IShaderExpressionPtr xScaleExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yScaleExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xScaleExpr && yScaleExpr)
		{
			_currentLayer->setScale(xScaleExpr, yScaleExpr);
		}
		else
		{
			rWarning() << "Could not parse scale expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "centerscale")
	{
		IShaderExpressionPtr xScaleExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yScaleExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xScaleExpr && yScaleExpr)
		{
			_currentLayer->setScale(xScaleExpr, yScaleExpr);
			_currentLayer->setStageFlag(ShaderLayer::FLAG_CENTERSCALE);	// enable centerScale
		}
		else
		{
			rWarning() << "Could not parse centerScale expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "translate" || token == "scroll")
	{
		IShaderExpressionPtr xTranslateExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yTranslateExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xTranslateExpr && yTranslateExpr)
		{
			_currentLayer->setTranslation(xTranslateExpr, yTranslateExpr);
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "shear")
	{
		IShaderExpressionPtr xShearExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpressionPtr yShearExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xShearExpr && yShearExpr)
		{
			_currentLayer->setShear(xShearExpr, yShearExpr);
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "rotate")
	{
		IShaderExpressionPtr rotExpr = ShaderExpression::createFromTokens(tokeniser);

		if (rotExpr)
		{
			_currentLayer->setRotation(rotExpr);
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "ignorealphatest")
	{
		_currentLayer->setStageFlag(ShaderLayer::FLAG_IGNORE_ALPHATEST);
	}
	else if (token == "colored")
	{
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_RED, ShaderExpression::createFromString("parm0"));
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_GREEN, ShaderExpression::createFromString("parm1"));
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_BLUE, ShaderExpression::createFromString("parm2"));
		_currentLayer->setColourExpression(Doom3ShaderLayer::COMP_ALPHA, ShaderExpression::createFromString("parm3"));
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
	else if (token == "privatepolygonoffset")
	{
		_currentLayer->setPrivatePolygonOffset(string::convert<float>(tokeniser.nextToken()));
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
		_surfaceFlags |= Material::SURF_ENTITYGUI;
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

/* Saves the accumulated data (m_type, m_blendFunc etc.) to the _layers vector.
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

						rWarning() << "Material keyword not recognised: " << token << std::endl;

                        break;
                    case 2: // stage level
						if (parseCondition(tokeniser, token)) continue;
                        if (parseBlendType(tokeniser, token)) continue;
                        if (parseBlendMaps(tokeniser, token)) continue;
                        if (parseStageModifiers(tokeniser, token)) continue;

						rWarning() << "Stage keyword not recognised: " << token << std::endl;

                        break;
                }
            }
        }
    }
    catch (parser::ParseException& p)
	{
        rError() << "Error while parsing shader " << _name << ": "
            << p.what() << std::endl;
    }

	// greebo: It appears that D3 is applying default sort values for material without
	// an explicitly defined sort value, depending on a couple of things I didn't really investigate
	// Some blend materials get SORT_MEDIUM applied by default, diffuses get OPAQUE assigned, but lights do not, etc.
	if (_sortReq == SORT_UNDEFINED)
	{
		// Translucent materials need to be drawn after opaque ones, if not explicitly specified otherwise
		if (_materialFlags & Material::FLAG_TRANSLUCENT)
		{
			_sortReq = Material::SORT_MEDIUM;
		}
		else
		{
			_sortReq = Material::SORT_OPAQUE;
		}
	}

	std::size_t numAmbientStages = 0;

	for (Layers::const_iterator i = _layers.begin(); i != _layers.end(); ++i)
	{
		if ((*i)->getType() == ShaderLayer::BLEND)
		{
			numAmbientStages++;
		}
	}
	
	// Determine coverage if not yet done
	if (_coverage == Material::MC_UNDETERMINED)
	{
		// automatically set MC_TRANSLUCENT if we don't have any interaction stages and 
		// the first stage is blended and not an alpha test mask or a subview
		if (_layers.empty())
		{
			// non-visible
			_coverage = Material::MC_TRANSLUCENT;
		} 
		else if (_layers.size() != numAmbientStages)
		{
			// we have an interaction draw
			_coverage = Material::MC_OPAQUE;
		}
		else
		{
			const Doom3ShaderLayer& firstLayer = **_layers.begin();

			BlendFunc blend = firstLayer.getBlendFunc();

			// If the layers are blended with the destination, we consider it translucent
			if (blend.dest != GL_ZERO || 
				blend.src == GL_DST_COLOR ||
				blend.src == GL_ONE_MINUS_DST_COLOR ||
				blend.src == GL_DST_ALPHA ||
				blend.src == GL_ONE_MINUS_DST_ALPHA)
			{
				_coverage = Material::MC_TRANSLUCENT;
			}
			else
			{
				_coverage = Material::MC_OPAQUE;
			}
		}
	}

	// translucent automatically implies noshadows
	if (_coverage == Material::MC_TRANSLUCENT)
	{
		_materialFlags |= Material::FLAG_NOSHADOWS;
	} 
	else 
	{
		// mark the contents as opaque
		_surfaceFlags |= Material::SURF_OPAQUE;
	}
}

void ShaderTemplate::addLayer(const Doom3ShaderLayerPtr& layer)
{
	// Add the layer
	_layers.push_back(layer);

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

	for (Layers::const_iterator i = _layers.begin(); i != _layers.end(); ++i)
    {
        if ((*i)->getType() == ShaderLayer::DIFFUSE)
        {
            return true;
        }
    }

	return false; // no diffuse found
}

} // namespace

