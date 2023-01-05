#include "ShaderTemplate.h"
#include "MapExpression.h"
#include "CameraCubeMapDecl.h"

#include "MapExpression.h"
#include "MaterialSourceGenerator.h"
#include "VideoMapExpression.h"
#include "SoundMapExpression.h"

#include "itextstream.h"

#include "os/path.h"
#include "string/convert.h"
#include "parser/DefTokeniser.h"

#include "string/case_conv.h"
#include "string/trim.h"
#include <iostream>

#include "ShaderExpression.h"
#include "util/ScopedBoolLock.h"
#include "materials/ParseLib.h"

namespace shaders
{

ShaderTemplate::ShaderTemplate(const ShaderTemplate& other) :
    decl::EditableDeclaration<shaders::IShaderTemplate>(other),
    _name(other._name),
    _currentLayer(new Doom3ShaderLayer(*this)),
    _suppressChangeSignal(false),
    _lightFalloff(other._lightFalloff),
    _lightFalloffCubeMapType(other._lightFalloffCubeMapType),
    fogLight(other.fogLight),
    ambientLight(other.ambientLight),
    blendLight(other.blendLight),
    _cubicLight(other._cubicLight),
    description(other.description),
    _materialFlags(other._materialFlags),
    _cullType(other._cullType),
    _clampType(other._clampType),
    _surfaceFlags(other._surfaceFlags),
    _surfaceType(other._surfaceType),
    _deformType(other._deformType),
    _deformExpressions(other._deformExpressions),
    _deformDeclName(other._deformDeclName),
    _spectrum(other._spectrum),
    _sortReq(other._sortReq),
    _polygonOffset(other._polygonOffset),
    _decalInfo(other._decalInfo),
    _coverage(other._coverage),
    _renderBumpArguments(other._renderBumpArguments),
    _renderBumpFlatArguments(other._renderBumpFlatArguments),
    _parseFlags(other._parseFlags),
    _guiDeclName(other._guiDeclName),
    _frobStageType(other._frobStageType)
{
    _editorTex = other._editorTex ? MapExpression::createForString(other._editorTex->getExpressionString()) : MapExpressionPtr();
    _frobStageMapExpression = other._frobStageMapExpression ? 
        MapExpression::createForString(other._frobStageMapExpression->getExpressionString()) : MapExpressionPtr();

    _frobStageRgbParameter[0] = other._frobStageRgbParameter[0];
    _frobStageRgbParameter[1] = other._frobStageRgbParameter[1];

    _ambientRimColour[0] = other._ambientRimColour[0];
    _ambientRimColour[1] = other._ambientRimColour[1];
    _ambientRimColour[2] = other._ambientRimColour[2];

    // Clone the layers
    for (const auto& otherLayer : other._layers)
    {
        _layers.emplace_back(std::make_shared<Doom3ShaderLayer>(*otherLayer, *this));
    }
}

std::shared_ptr<ShaderTemplate> ShaderTemplate::clone()
{
    ensureParsed();

    return std::make_shared<ShaderTemplate>(*this);
}

const MapExpressionPtr& ShaderTemplate::getEditorTexture()
{
    ensureParsed();

    return _editorTex;
}

void ShaderTemplate::setEditorImageExpressionFromString(const std::string& expression)
{
    ensureParsed();

    _editorTex = !expression.empty() ? MapExpression::createForString(expression) : MapExpressionPtr();
    onTemplateChanged();
}

void ShaderTemplate::setDecalInfo(const Material::DecalInfo& info)
{
    ensureParsed();

    _decalInfo = info;

    // Check if this decal info is empty, if yes: clear the flag 
    Material::DecalInfo emptyInfo;

    if (_decalInfo.stayMilliSeconds == emptyInfo.stayMilliSeconds &&
        _decalInfo.fadeMilliSeconds == emptyInfo.fadeMilliSeconds &&
        _decalInfo.startColour == emptyInfo.startColour &&
        _decalInfo.endColour == emptyInfo.endColour)
    {
        _parseFlags &= ~Material::PF_HasDecalInfo;
    }
    else
    {
        _parseFlags |= Material::PF_HasDecalInfo;
    }

    onTemplateChanged();
}

void ShaderTemplate::setFrobStageType(Material::FrobStageType type)
{
    ensureParsed();

    _frobStageType = type;

    onTemplateChanged();
}

void ShaderTemplate::setFrobStageMapExpressionFromString(const std::string& expr)
{
    ensureParsed();

    if (!expr.empty())
    {
        _frobStageMapExpression = MapExpression::createForString(expr);
    }
    else
    {
        _frobStageMapExpression.reset();
    }

    onTemplateChanged();
}

void ShaderTemplate::setFrobStageParameter(std::size_t index, double value)
{
    setFrobStageRgbParameter(index, Vector3(value, value, value));
}

void ShaderTemplate::setFrobStageRgbParameter(std::size_t index, const Vector3& value)
{
    if (index > 1) throw std::out_of_range("Index must be 0 or 1");

    ensureParsed();

    _frobStageRgbParameter[index] = value;

    onTemplateChanged();
}

IShaderExpression::Ptr ShaderTemplate::parseSingleExpressionTerm(parser::DefTokeniser& tokeniser)
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
        _parseFlags |= Material::PF_HasDecalMacro;

        _materialFlags |= Material::FLAG_TRANSLUCENT|Material::FLAG_NOSHADOWS;
        _materialFlags |= Material::FLAG_HAS_SORT_DEFINED;
        _materialFlags |= Material::FLAG_POLYGONOFFSET;
        _sortReq = Material::SORT_DECAL;
        _polygonOffset = 1.0f;
		_surfaceFlags |= Material::SURF_DISCRETE | Material::SURF_NONSOLID;
    }
    else if (token == "twosided_decal_macro")
    {
        _parseFlags |= Material::PF_HasTwoSidedDecalMacro;

        _materialFlags |= Material::FLAG_TRANSLUCENT | Material::FLAG_NOSHADOWS | Material::FLAG_NOSELFSHADOW;
        _materialFlags |= Material::FLAG_HAS_SORT_DEFINED;
        _materialFlags |= Material::FLAG_POLYGONOFFSET;
        _sortReq = Material::SORT_DECAL;
        _polygonOffset = 1.0f;
        _surfaceFlags |= Material::SURF_DISCRETE | Material::SURF_NOIMPACT | Material::SURF_NONSOLID;
        _cullType = Material::CULL_NONE;

        _coverage = Material::MC_TRANSLUCENT;
    }
    else if (token == "particle_macro")
    {
        _parseFlags |= Material::PF_HasParticleMacro;

        _materialFlags |= Material::FLAG_NOSHADOWS | Material::FLAG_NOSELFSHADOW;
        _surfaceFlags |= Material::SURF_DISCRETE | Material::SURF_NOIMPACT | Material::SURF_NONSOLID;
        _coverage = Material::MC_TRANSLUCENT;
    }
    else if (token == "glass_macro")
    {
        _parseFlags |= Material::PF_HasGlassMacro;

        _cullType = Material::CULL_NONE;
        _materialFlags |= Material::FLAG_NOSHADOWS | Material::FLAG_NOSELFSHADOW;
        _coverage = Material::MC_TRANSLUCENT;
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
        _materialFlags |= Material::FLAG_POLYGONOFFSET;

        // The value argument is optional, try to parse the next token
        if (tokeniser.hasMoreTokens())
        {
            try
            {
                _polygonOffset = std::stof(tokeniser.peek());
                // success, exhaust the token
                tokeniser.skipTokens(1);
            }
            catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
            {
                _polygonOffset = 1.0f;
            }
        }
        else
        {
            _polygonOffset = 1.0f;
        }
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
        _parseFlags |= Material::PF_HasSortDefined;
        _materialFlags |= Material::FLAG_HAS_SORT_DEFINED;

		auto sortVal = tokeniser.nextToken();
        string::to_lower(sortVal);

        for (const auto& predefinedSortValue : shaders::PredefinedSortValues)
        {
            if (sortVal == predefinedSortValue.first)
            {
                _sortReq = static_cast<float>(predefinedSortValue.second);
                return true;
            }
        }

		// no special sort keyword, try to parse the numeric value
		//  Strip any quotes
		string::trim(sortVal, "\"");

		_sortReq = string::convert<float>(sortVal, SORT_UNDEFINED); // fall back to UNDEFINED in case of parsing failures
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
        _parseFlags |= Material::PF_HasDecalInfo;

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
		string::to_lower(type);

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

            _deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // skip size info
		}
		else if (type == "expand")
		{
			_deformType = Material::DEFORM_EXPAND;

            _deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // skip amount
		}
		else if (type == "move")
		{
			_deformType = Material::DEFORM_MOVE;

            _deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // skip amount
		}
		else if (type == "turbulent")
		{
			_deformType = Material::DEFORM_TURBULENT;

            _deformDeclName = tokeniser.nextToken(); // table name

			_deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // range
			_deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // timeoffset
            _deformExpressions.emplace_back(parseSingleExpressionTerm(tokeniser)); // domain
		}
		else if (type == "eyeball")
		{
			_deformType = Material::DEFORM_EYEBALL;
		}
		else if (type == "particle")
		{
			_deformType = Material::DEFORM_PARTICLE;
            _deformDeclName = tokeniser.nextToken();
		}
		else if (type == "particle2")
		{
			_deformType = Material::DEFORM_PARTICLE2;
            _deformDeclName = tokeniser.nextToken();
		}
	}
	else if (token == "renderbump")
	{
		// Skip over this renderbump directive
		// Syntax: renderBump [-size <width> <height>] [-aa <0/1/2>] [-trace <0.01 - 1.0>] <normalMapImage> <highPolyModel>
		std::string next = tokeniser.nextToken();
		string::to_lower(next);

		// Skip over the optional args
		while (next.length() > 0 && next[0] == '-')
		{
			if (next == "-size")
			{
                // Store -size argument in the command arguments
                _renderBumpArguments += !_renderBumpArguments.empty() ? " " : "";
                auto width = tokeniser.nextToken();
                auto height = tokeniser.nextToken();
                _renderBumpArguments += fmt::format("{0} {1} {2}", next, width, height);
			}
			else if (next == "-aa")
			{
                // Store -aa argument in the command arguments
                _renderBumpArguments += !_renderBumpArguments.empty() ? " " : "";
                _renderBumpArguments += fmt::format("{0} {1}", next, tokeniser.nextToken());
			}
			else if (next == "-trace")
			{
                // Store -trace argument in the command arguments
                _renderBumpArguments += !_renderBumpArguments.empty() ? " " : "";
                _renderBumpArguments += fmt::format("{0} {1}", next, tokeniser.nextToken());
			}

			next = tokeniser.nextToken();
			string::to_lower(next);
		}

		// The map token is already loaded in "next", add the highpoly model name
        _renderBumpArguments += !_renderBumpArguments.empty() ? " " : "";
        _renderBumpArguments += fmt::format("{0} {1}", next, tokeniser.nextToken());
        string::trim(_renderBumpArguments);
	}
	else if (token == "renderbumpflat")
	{
		// Skip over this renderbump directive
		// Syntax: RenderBumpFlat [-size <width> <height>] <modelfile>
		std::string next = tokeniser.nextToken();
		string::to_lower(next);

		// Skip over the optional args
		if (next == "-size")
		{
            auto width = tokeniser.nextToken();
            auto height = tokeniser.nextToken();
            _renderBumpFlatArguments += fmt::format("{0} {1} {2}", next, width, height);

            next = tokeniser.nextToken();
		}

        // The highpoly model token is already loaded in "next"
        _renderBumpFlatArguments += !_renderBumpFlatArguments.empty() ? " " : "";
        _renderBumpFlatArguments += next;
        string::trim(_renderBumpFlatArguments);
	}
    else if (token == "ambientrimcolor")
    {
        _parseFlags |= Material::PF_HasAmbientRimColour;

        // ambientRimColor <exp0>, <exp1>, <exp2>
        auto red = ShaderExpression::createFromTokens(tokeniser);
        tokeniser.assertNextToken(",");
        auto green = ShaderExpression::createFromTokens(tokeniser);
        tokeniser.assertNextToken(",");
        auto blue = ShaderExpression::createFromTokens(tokeniser);

        if (red && green && blue)
        {
            // ambientrimcolor support not yet added, we need material registers first
            _ambientRimColour[0] = red;
            _ambientRimColour[1] = green;
            _ambientRimColour[2] = blue;
        }
        else
        {
            rWarning() << "Could not parse ambientRimColor expressions in shader: " << getName() << std::endl;
        }
    }
    else if (token == "islightgemsurf")
    {
        _materialFlags |= Material::FLAG_ISLIGHTGEMSURF;
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
    else if (token == "cubliclight")
    {
        _cubicLight = true;
    }
    else if (token == "ambientcubiclight")
    {
        ambientLight = true;
        _cubicLight = true;
    }
    else if (!fogLight && token == "lightfalloffimage")
	{
        _lightFalloffCubeMapType = IShaderLayer::MapType::Map;
        _lightFalloff = MapExpression::createForToken(tokeniser);
    }
    else if (token == "lightfalloffcubemap")
    {
        _lightFalloffCubeMapType = IShaderLayer::MapType::CameraCubeMap;
        _lightFalloff = MapExpression::createForToken(tokeniser);
    }
	else if (token == "spectrum")
	{
        _parseFlags |= Material::PF_HasSpectrum;

		std::string value = tokeniser.nextToken();

		try
		{
			_spectrum = std::stoi(value);
		}
		catch (std::logic_error& e)
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
        addLayer(IShaderLayer::DIFFUSE, MapExpression::createForToken(tokeniser));
    }
    else if (token == "specularmap")
    {
		addLayer(IShaderLayer::SPECULAR, MapExpression::createForToken(tokeniser));
    }
    else if (token == "bumpmap")
    {
		addLayer(IShaderLayer::BUMP, MapExpression::createForToken(tokeniser));
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
        // Special blend type, either predefined like "add" or "modulate",
        // or an explicit combination of GL blend modes
        StringPair blendFuncStrings;
        blendFuncStrings.first = string::to_lower_copy(tokeniser.nextToken());

        if (blendFuncStrings.first.substr(0, 3) == "gl_")
        {
            // This is an explicit GL blend mode
            tokeniser.assertNextToken(",");
            blendFuncStrings.second = string::to_lower_copy(tokeniser.nextToken());
        }
        else
        {
            blendFuncStrings.second = "";
        }

        _currentLayer->setBlendFuncStrings(blendFuncStrings);
	    return true;
    }
	
    return false; // unrecognised token, return false
}

/* Searches for the map keyword in stage 2, expects token to be lowercase
 */
bool ShaderTemplate::parseBlendMaps(parser::DefTokeniser& tokeniser, const std::string& token)
{
    if (token == "map")
    {
        _currentLayer->setBindableTexture(MapExpression::createForToken(tokeniser));
        // Don't reset the map type of this layer, "map" can occur in multiple scenarios
    }
    else if (token == "cameracubemap")
    {
        std::string cubeMapPrefix = tokeniser.nextToken();
        _currentLayer->setBindableTexture(
            CameraCubeMapDecl::createForPrefix(cubeMapPrefix)
        );
        _currentLayer->setMapType(IShaderLayer::MapType::CameraCubeMap);
        _currentLayer->setCubeMapMode(IShaderLayer::CUBE_MAP_CAMERA);
    }
	else if (token == "texgen")
	{
		std::string type = tokeniser.nextToken();
        _currentLayer->setParseFlag(IShaderLayer::PF_HasTexGenKeyword);

		if (type == "skybox")
		{
			_currentLayer->setTexGenType(IShaderLayer::TEXGEN_SKYBOX);
		}
		else if (type == "reflect")
		{
			_currentLayer->setTexGenType(IShaderLayer::TEXGEN_REFLECT);
		}
		else if (type == "normal")
		{
			_currentLayer->setTexGenType(IShaderLayer::TEXGEN_NORMAL);
		}
		else if (type == "wobblesky")
		{
			_currentLayer->setTexGenType(IShaderLayer::TEXGEN_WOBBLESKY);

			// Parse the 3 wobblesky expressions
            _currentLayer->setTexGenExpression(0, parseSingleExpressionTerm(tokeniser));
            _currentLayer->setTexGenExpression(1, parseSingleExpressionTerm(tokeniser));
            _currentLayer->setTexGenExpression(2, parseSingleExpressionTerm(tokeniser));
		}
	}
	else if (token == "cubemap")
    {
		// Parse the cubemap expression, but don't do anything with it for now
        _currentLayer->setBindableTexture(MapExpression::createForToken(tokeniser));
        _currentLayer->setMapType(IShaderLayer::MapType::CubeMap);
        _currentLayer->setCubeMapMode(IShaderLayer::CUBE_MAP_OBJECT);
    }
	else if (token == "videomap")
    {
        _currentLayer->setMapType(IShaderLayer::MapType::VideoMap);
        _currentLayer->setBindableTexture(
            VideoMapExpression::CreateForTokens(tokeniser)
        );
    }
	else if (token == "soundmap")
	{
        _currentLayer->setMapType(IShaderLayer::MapType::SoundMap);
        _currentLayer->setBindableTexture(
            SoundMapExpression::CreateForTokens(tokeniser)
        );
	}
	else if (token == "remoterendermap")
	{
        _currentLayer->setMapType(IShaderLayer::MapType::RemoteRenderMap);
        parseRenderMapSize(tokeniser, true); // remoteRenderMap dimension is mandatory
	}
	else if (token == "mirrorrendermap")
	{
        _currentLayer->setMapType(IShaderLayer::MapType::MirrorRenderMap);
        _currentLayer->setTexGenType(IShaderLayer::TexGenType::TEXGEN_SCREEN);
        parseRenderMapSize(tokeniser, true); // mirrorRenderMap dimension is optional
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

void ShaderTemplate::parseRenderMapSize(parser::DefTokeniser& tokeniser, bool optional)
{
    // Parse the dimensions without immediately exhausting the upcoming token
    // Will not exhaust the tokens that are not convertible to an integer
    int width;
    if (string::tryConvertToInt(tokeniser.peek(), width))
    {
        tokeniser.nextToken(); // exhaust
    }
    else if (!optional)
    {
        rWarning() << "Error parsing render map width. Expected two integers." << std::endl;
        return;
    }

    int height;
    if (string::tryConvertToInt(tokeniser.peek(), height))
    {
        tokeniser.nextToken(); // exhaust
    }
    else if (!optional)
    {
        rWarning() << "Error parsing render map height. Expected two integers." << std::endl;
        return;
    }

    _currentLayer->setRenderMapSize({ width, height });
}

bool ShaderTemplate::parseStageModifiers(parser::DefTokeniser& tokeniser,
                                         const std::string& token)
{
    if (token == "vertexcolor")
    {
        _currentLayer->setVertexColourMode(
            IShaderLayer::VERTEX_COLOUR_MULTIPLY
        );
    }
    else if (token == "inversevertexcolor")
    {
        _currentLayer->setVertexColourMode(
            IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY
        );
    }
	else if (token == "red")
	{
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);
		
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
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);

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
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);
		
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
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);
		
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
		IShaderExpression::Ptr red = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpression::Ptr green = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpression::Ptr blue = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		IShaderExpression::Ptr alpha = ShaderExpression::createFromTokens(tokeniser);

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
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);

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
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);

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
        IShaderLayer::VertexParm parm;

		// vertexParm		<parmNum>		<parm1> [,<parm2>] [,<parm3>] [,<parm4>]
		parm.index = string::convert<int>(tokeniser.nextToken());

        if (parm.index < 0 || parm.index >= NUM_MAX_VERTEX_PARMS)
        {
            throw parser::ParseException(fmt::format("A material stage can have {0} vertex parameters at most", NUM_MAX_VERTEX_PARMS));
        }

        parm.expressions[0] = ShaderExpression::createFromTokens(tokeniser);

        for (int i = 1; i < 4 && tokeniser.peek() == ","; ++i)
        {
            tokeniser.nextToken();

            parm.expressions[i] = ShaderExpression::createFromTokens(tokeniser);
        }

        _currentLayer->addVertexParm(parm);
	}
	else if (token == "fragmentmap")
	{
        IShaderLayer::FragmentMap map;

		// fragmentMap <index> [options] <map>
        map.index = string::convert<int>(tokeniser.nextToken());

        if (map.index < 0 || map.index >= NUM_MAX_FRAGMENT_MAPS)
        {
            throw parser::ParseException(fmt::format("A material stage can have {0} fragment maps at most", NUM_MAX_FRAGMENT_MAPS));
        }

		std::string next = tokeniser.peek();
		string::to_lower(next);

		// These are all valid option keywords
		while (next == "cubemap" || next == "cameracubemap" || next == "nearest" ||
			next == "linear" || next == "clamp" || next == "noclamp" ||
			next == "zeroclamp" || next == "alphazeroclamp" || next == "forcehighquality" ||
			next == "uncompressed" || next == "highquality" || next == "nopicmip")
		{
            map.options.emplace_back(tokeniser.nextToken());

			next = tokeniser.peek();
			string::to_lower(next);
		}

		// Get the map expression and add the fragment map to the stage
        map.map = MapExpression::createForToken(tokeniser);

		_currentLayer->addFragmentMap(map);
	}
    else if (token == "alphatest")
    {
		// Get the alphatest expression
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);
		   
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
		auto xScaleExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		auto yScaleExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xScaleExpr && yScaleExpr)
		{
            _currentLayer->appendTransformation(IShaderLayer::Transformation
            {
                IShaderLayer::TransformType::Scale, 
                xScaleExpr, 
                yScaleExpr
            });
		}
		else
		{
			rWarning() << "Could not parse scale expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "centerscale")
	{
		auto xScaleExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		auto yScaleExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xScaleExpr && yScaleExpr)
		{
            _currentLayer->appendTransformation(IShaderLayer::Transformation
            {
                IShaderLayer::TransformType::CenterScale,
                xScaleExpr,
                yScaleExpr
            });
		}
		else
		{
			rWarning() << "Could not parse centerScale expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "translate" || token == "scroll")
	{
		auto xTranslateExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		auto yTranslateExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xTranslateExpr && yTranslateExpr)
		{
            _currentLayer->appendTransformation(IShaderLayer::Transformation
                {
                    IShaderLayer::TransformType::Translate,
                    xTranslateExpr,
                    yTranslateExpr
                });
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "shear")
	{
		auto xShearExpr = ShaderExpression::createFromTokens(tokeniser);
		tokeniser.assertNextToken(",");
		auto yShearExpr = ShaderExpression::createFromTokens(tokeniser);

		if (xShearExpr && yShearExpr)
		{
            _currentLayer->appendTransformation(IShaderLayer::Transformation
            {
                IShaderLayer::TransformType::Shear,
                xShearExpr,
                yShearExpr
            });
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "rotate")
	{
		auto rotationExpr = ShaderExpression::createFromTokens(tokeniser);

		if (rotationExpr)
		{
            _currentLayer->appendTransformation(IShaderLayer::Transformation
            {
                IShaderLayer::TransformType::Rotate,
                rotationExpr
            });
		}
		else
		{
			rWarning() << "Could not parse " << token << " expression in shader: " << getName() << std::endl;
		}
	}
	else if (token == "ignorealphatest")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_IGNORE_ALPHATEST);
	}
	else if (token == "colored")
	{
        _currentLayer->setParseFlag(IShaderLayer::PF_HasColoredKeyword);
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
        _currentLayer->setParseFlag(IShaderLayer::PF_HasNoclampKeyword);
	}
	else if (token == "uncompressed" || token == "highquality")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_HIGHQUALITY);
	}
	else if (token == "forcehighquality")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_FORCE_HIGHQUALITY);
	}
	else if (token == "nopicmip")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_NO_PICMIP);
	}
	else if (token == "maskred")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
	}
	else if (token == "maskgreen")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
	}
	else if (token == "maskblue")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
	}
	else if (token == "maskalpha")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_ALPHA);
	}
	else if (token == "maskcolor")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
	}
	else if (token == "maskdepth")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_MASK_DEPTH);
	}
    else if (token == "ignoredepth")
    {
        _currentLayer->setStageFlag(IShaderLayer::FLAG_IGNORE_DEPTH);
    }
	else if (token == "privatepolygonoffset")
	{
		_currentLayer->setPrivatePolygonOffset(string::convert<float>(tokeniser.nextToken()));
	}
	else if (token == "nearest")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_FILTER_NEAREST);
	}
	else if (token == "linear")
	{
		_currentLayer->setStageFlag(IShaderLayer::FLAG_FILTER_LINEAR);
	}
	else
	{
		return false; // unrecognised token, return false
	}

	return true;
}

bool ShaderTemplate::parseMaterialType(parser::DefTokeniser& tokeniser, const std::string& token)
{
    for (const auto& pair : SurfaceTypeMapping)
    {
        if (token == pair.first)
        {
            _surfaceType = pair.second;
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::parseFrobstageKeywords(parser::DefTokeniser& tokeniser, const std::string& token)
{
    if (!string::starts_with(token, "frobstage_")) return false;

    auto suffix = token.substr(10);

    if (suffix == "texture")
    {
        _frobStageType = Material::FrobStageType::Texture;
        _frobStageMapExpression = MapExpression::createForToken(tokeniser);
        _frobStageRgbParameter[0] = parseScalarOrVector3(tokeniser);
        _frobStageRgbParameter[1] = parseScalarOrVector3(tokeniser);
        return true;
    }

    if (suffix == "diffuse")
    {
        _frobStageType = Material::FrobStageType::Diffuse;
        _frobStageRgbParameter[0] = parseScalarOrVector3(tokeniser);
        _frobStageRgbParameter[1] = parseScalarOrVector3(tokeniser);
        return true;
    }
    
    if (suffix == "none")
    {
        _frobStageType = Material::FrobStageType::NoFrobStage;
        return true;
    }

    return false;
}

Vector3 ShaderTemplate::parseScalarOrVector3(parser::DefTokeniser& tokeniser)
{
    auto token = tokeniser.nextToken();

    if (token == "(") // vector
    {
        auto x = string::convert<Vector3::ElementType>(tokeniser.nextToken());
        auto y = string::convert<Vector3::ElementType>(tokeniser.nextToken());
        auto z = string::convert<Vector3::ElementType>(tokeniser.nextToken());
        tokeniser.assertNextToken(")");

        return Vector3(x, y, z);
    }

    // scalar
    auto value = string::convert<Vector3::ElementType>(token);
    return Vector3(value, value, value);
}

bool ShaderTemplate::parseSurfaceFlags(parser::DefTokeniser& tokeniser,
                                       const std::string& token)
{
    for (const auto& pair : SurfaceFlags)
    {
        if (token == pair.first)
        {
            _surfaceFlags |= pair.second;
            return true;
        }
    }

	if (token == "guisurf")
	{
		// "guisurf blah.gui" or "guisurf entity[2|3]"
		_surfaceFlags |= Material::SURF_GUISURF;

        auto argument = tokeniser.nextToken();

        if (string::to_lower_copy(argument) == "entity")
        {
            _surfaceFlags |= Material::SURF_ENTITYGUI;
        }
        else if (string::to_lower_copy(argument) == "entity2")
        {
            _surfaceFlags |= Material::SURF_ENTITYGUI2;
        }
        else if (string::to_lower_copy(argument) == "entity3")
        {
            _surfaceFlags |= Material::SURF_ENTITYGUI3;
        }
        else
        {
            _guiDeclName = argument;
        }

        return true;
	}
	
    return false; // unrecognised token, return false
}

bool ShaderTemplate::parseCondition(parser::DefTokeniser& tokeniser, const std::string& token)
{
	if (token == "if")
	{
		// Parse condition
		IShaderExpression::Ptr expr = ShaderExpression::createFromTokens(tokeniser);
		
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
    if (_currentLayer->getBindableTexture() || 
        _currentLayer->getMapType() == IShaderLayer::MapType::RemoteRenderMap ||
        _currentLayer->getMapType() == IShaderLayer::MapType::MirrorRenderMap ||
        !_currentLayer->getVertexProgram().empty() || !_currentLayer->getFragmentProgram().empty())
    {
		addLayer(_currentLayer);
    }

    // Clear the currentLayer structure for possible future layers
    _currentLayer = std::make_shared<Doom3ShaderLayer>(*this);

    return true;
}

void ShaderTemplate::clear()
{
    _layers.clear();
    _currentLayer.reset(new Doom3ShaderLayer(*this));

    description.clear();
    _suppressChangeSignal = false;
    _lightFalloffCubeMapType = IShaderLayer::MapType::Map;
    fogLight = false;
    ambientLight = false;
    blendLight = false;
    _cubicLight = false;
    _materialFlags = 0;
    _cullType = Material::CULL_BACK;
    _clampType = CLAMP_REPEAT;
    _surfaceFlags = 0;
    _surfaceType = Material::SURFTYPE_DEFAULT;
    _deformType = Material::DEFORM_NONE;
    _spectrum = 0;
    _sortReq = SORT_UNDEFINED;	// will be set to default values after the shader has been parsed
    _polygonOffset = 0.0f;
    _coverage = Material::MC_UNDETERMINED;
    _parseFlags = 0;
    _frobStageType = Material::FrobStageType::Default;
    _frobStageMapExpression.reset();
    _frobStageRgbParameter[0].set(0, 0, 0);
    _frobStageRgbParameter[1].set(0, 0, 0);
    
    _decalInfo.stayMilliSeconds = 0;
    _decalInfo.fadeMilliSeconds = 0;
    _decalInfo.startColour = Vector4(1, 1, 1, 1);
    _decalInfo.endColour = Vector4(0, 0, 0, 0);
}

void ShaderTemplate::onBeginParsing()
{
    clear();
}

void ShaderTemplate::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    util::ScopedBoolLock parseLock(_suppressChangeSignal);

    int level = 1;  // we always start at top level

    while (level > 0 && tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        
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
            string::to_lower(token);

            switch (level)
            {
                case 1: // global level
                    if (parseShaderFlags(tokeniser, token)) continue;
                    if (parseLightKeywords(tokeniser, token)) continue;
                    if (parseBlendShortcuts(tokeniser, token)) continue;
                    if (parseSurfaceFlags(tokeniser, token)) continue;
                    if (parseMaterialType(tokeniser, token)) continue;
                    if (parseFrobstageKeywords(tokeniser, token)) continue;

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

	// greebo: It appears that D3 is applying default sort values for material without
	// an explicitly defined sort value, depending on a couple of things I didn't really investigate
	// Some blend materials get SORT_MEDIUM applied by default, diffuses get OPAQUE assigned, but lights do not, etc.
	if (_sortReq == SORT_UNDEFINED)
	{
        resetSortRequest();
	}

	determineCoverage();
}

void ShaderTemplate::determineCoverage()
{
    // An explicit translucent keyword makes it clear
    if (_materialFlags & Material::FLAG_TRANSLUCENT)
    {
        _coverage = Material::MC_TRANSLUCENT;
        return;
    }

    // Determine coverage if not yet done
    if (_coverage == Material::MC_UNDETERMINED)
    {
        std::size_t numAmbientStages = 0;

        for (const auto& layer : _layers)
        {
            if (layer->getType() == IShaderLayer::BLEND)
            {
                numAmbientStages++;
            }
        }

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

            // Check if we have alpha-tested layers
            for (const auto& layer : _layers)
            {
                if (layer->hasAlphaTest())
                {
                    _coverage = Material::MC_PERFORATED;
                    break;
                }
            }
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

void ShaderTemplate::addLayer(const Doom3ShaderLayer::Ptr& layer)
{
    ensureParsed();

    // Add the layer
    _layers.emplace_back(layer);

    // If this is our first layer, redetermine the coverage
    if (_layers.size() == 1)
    {
        _coverage = Material::MC_UNDETERMINED;
        determineCoverage();
    }

    onTemplateChanged();
}

void ShaderTemplate::addLayer(IShaderLayer::Type type, const MapExpressionPtr& mapExpr)
{
	// Construct a layer out of this mapexpression and pass the call
	addLayer(std::make_shared<Doom3ShaderLayer>(*this, type, mapExpr));
}

std::size_t ShaderTemplate::addLayer(IShaderLayer::Type type)
{
    // Determine the default map expression to use for this type
    std::shared_ptr<MapExpression> map;

    switch (type)
    {
    case IShaderLayer::BUMP:
        map = MapExpression::createForString("_flat");
        break;
    case IShaderLayer::SPECULAR:
        map = MapExpression::createForString("_black");
        break;
    case IShaderLayer::DIFFUSE:
        map = MapExpression::createForString("_white");
    case IShaderLayer::BLEND:
    default:
        break;
    }

    addLayer(std::make_shared<Doom3ShaderLayer>(*this, type, map));

    return _layers.size() - 1;
}

void ShaderTemplate::removeLayer(std::size_t index)
{
    _layers.erase(_layers.begin() + index);

    if (_layers.empty())
    {
        _coverage = Material::MC_UNDETERMINED;
        determineCoverage();
    }

    onTemplateChanged();
}

void ShaderTemplate::swapLayerPosition(std::size_t first, std::size_t second)
{
    if (first >= _layers.size() || second >= _layers.size())
    {
        return;
    }

    _layers[first].swap(_layers[second]);

    onTemplateChanged();
}

std::size_t ShaderTemplate::duplicateLayer(std::size_t index)
{
    if (index >= _layers.size())
    {
        throw std::runtime_error("Cannot duplicate this stage, index invalid");
    }

    _layers.emplace_back(std::make_shared<Doom3ShaderLayer>(*_layers[index], *this));

    onTemplateChanged();

    return _layers.size() - 1;
}

bool ShaderTemplate::hasDiffusemap()
{
	ensureParsed();

	for (const auto& layer : _layers)
    {
        if (layer->getType() == IShaderLayer::DIFFUSE)
        {
            return true;
        }
    }

	return false; // no diffuse found
}

int ShaderTemplate::getParseFlags()
{
    ensureParsed();

    return _parseFlags;
}

std::string ShaderTemplate::getRenderBumpArguments()
{
    ensureParsed();

    return _renderBumpArguments;
}

std::string ShaderTemplate::getRenderBumpFlatArguments()
{
    ensureParsed();

    return _renderBumpFlatArguments;
}

bool ShaderTemplate::evaluateMacroUsage()
{
    ensureParsed();

    auto oldDecalMacroFlag = _parseFlags & Material::PF_HasDecalMacro;

    // Reset the decal_macro flag and evaluate
    _parseFlags &= ~Material::PF_HasDecalMacro;

    if (getPolygonOffset() == 1.0f &&
        getSortRequest() == Material::SORT_DECAL &&
        (getSurfaceFlags() & Material::SURF_DISCRETE) != 0 &&
        (getMaterialFlags() & Material::FLAG_NOSHADOWS) != 0)
    {
        _parseFlags |= Material::PF_HasDecalMacro;
    }

    // If the flag changed => return true
    return oldDecalMacroFlag != (_parseFlags & Material::PF_HasDecalMacro);
}

std::string ShaderTemplate::generateSyntax()
{
    return MaterialSourceGenerator::GenerateDefinitionBlock(*this);
}

void ShaderTemplate::onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block)
{
    EditableDeclaration<IShaderTemplate>::onSyntaxBlockAssigned(block);

    // Don't call onTemplateChanged() since that is meant is to be used
    // when the template is modified after parsing
    // Just emit the template changed signal
    _sigTemplateChanged.emit();
}

} // namespace

