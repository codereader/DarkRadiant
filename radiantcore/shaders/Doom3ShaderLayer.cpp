#include "Doom3ShaderLayer.h"
#include "Doom3ShaderSystem.h"

namespace shaders
{

// Map string blend functions to their GLenum equivalents
GLenum glBlendFromString(const std::string& value)
{
	if (value == "gl_zero") {
		return GL_ZERO;
	}
	if (value == "gl_one") {
		return GL_ONE;
	}
	if (value == "gl_src_color") {
		return GL_SRC_COLOR;
	}
	if (value == "gl_one_minus_src_color") {
		return GL_ONE_MINUS_SRC_COLOR;
	}
	if (value == "gl_src_alpha") {
		return GL_SRC_ALPHA;
	}
	if (value == "gl_one_minus_src_alpha") {
		return GL_ONE_MINUS_SRC_ALPHA;
	}
	if (value == "gl_dst_color") {
		return GL_DST_COLOR;
	}
	if (value == "gl_one_minus_dst_color") {
		return GL_ONE_MINUS_DST_COLOR;
	}
	if (value == "gl_dst_alpha") {
		return GL_DST_ALPHA;
	}
	if (value == "gl_one_minus_dst_alpha") {
		return GL_ONE_MINUS_DST_ALPHA;
	}
	if (value == "gl_src_alpha_saturate") {
		return GL_SRC_ALPHA_SATURATE;
	}

	return GL_ZERO;
}

// Convert a string pair describing a blend function into a BlendFunc object
BlendFunc blendFuncFromStrings(const StringPair& blendFunc)
{
    // Handle predefined blend modes first: add, modulate, filter
    if (blendFunc.first == "add")
    {
        return BlendFunc(GL_ONE, GL_ONE);
    }
    else if (blendFunc.first == "modulate" || blendFunc.first == "filter")
    {
        return BlendFunc(GL_DST_COLOR, GL_ZERO);
    }
	else if (blendFunc.first == "blend")
	{
		return BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (blendFunc.first == "none")
	{
		return BlendFunc(GL_ZERO, GL_ONE);
	}
    else
    {
        // Not predefined, just use the specified blend function directly
        return BlendFunc(
            glBlendFromString(blendFunc.first),
            glBlendFromString(blendFunc.second)
        );
    }
}

// ShaderLayer implementation

const IShaderExpressionPtr Doom3ShaderLayer::NULL_EXPRESSION;

Doom3ShaderLayer::Doom3ShaderLayer(ShaderTemplate& material, ShaderLayer::Type type, const NamedBindablePtr& btex)
:	_material(material),
	_registers(NUM_RESERVED_REGISTERS),
	_condition(REG_ONE),
    _conditionExpression(NOT_DEFINED),
	_bindableTex(btex),
	_type(type),
    _mapType(MapType::Map),
	_blendFuncStrings("gl_one", "gl_zero"), // needs to be lowercase
	_vertexColourMode(VERTEX_COLOUR_NONE),
	_cubeMapMode(CUBE_MAP_NONE),
	_stageFlags(0),
	_clampType(CLAMP_REPEAT),
	_alphaTest(REG_ZERO),
	_texGenType(TEXGEN_NORMAL),
	_privatePolygonOffset(0),
    _parseFlags(0)
{
	_registers[REG_ZERO] = 0;
	_registers[REG_ONE] = 1;

	// Init the colour to 1,1,1,1
	_colIdx[0] = _colIdx[1] = _colIdx[2] = _colIdx[3] = REG_ONE;
    _colExpression[0] = _colExpression[1] = _colExpression[2] = _colExpression[3] = NOT_DEFINED;

	// Scale is set to 1,1 by default
	_scale[0] = _scale[1] = REG_ONE;
    _scaleExpression[0] = _scaleExpression[1] = NOT_DEFINED;

	// Translation is set to 0,0 by default
	_translation[0] = _translation[1] = REG_ZERO;
    _translationExpression[0] = _translationExpression[1] = NOT_DEFINED;

	// Rotation is set to 0 by default
	_rotation = REG_ZERO;
    _rotationExpression = NOT_DEFINED;

	// No shearing so far
	_shear[0] = REG_ZERO;
	_shear[1] = REG_ZERO;
    _shearExpression[0] = _shearExpression[1] = NOT_DEFINED;

	_texGenParams[0] = _texGenParams[1] = _texGenParams[2] = REG_ZERO;
}

TexturePtr Doom3ShaderLayer::getTexture() const
{
    // Bind texture to GL if needed
    if (!_texture && _bindableTex)
    {
        _texture = GetTextureManager().getBinding(_bindableTex);
    }

    return _texture;
}

BlendFunc Doom3ShaderLayer::getBlendFunc() const
{
    return blendFuncFromStrings(_blendFuncStrings);
}

Colour4 Doom3ShaderLayer::getColour() const
{
	// Resolve the register values
    Colour4 colour(getRegisterValue(_colIdx[0]), getRegisterValue(_colIdx[1]),
				   getRegisterValue(_colIdx[2]), getRegisterValue(_colIdx[3]));

    if (!colour.isValid())
    {
        return Colour4::WHITE();
    }

    return colour;
}

const IShaderExpressionPtr& Doom3ShaderLayer::getColourExpression(ColourComponentSelector component)
{
    std::size_t expressionIndex = NOT_DEFINED;

    switch (component)
    {
    case COMP_RED:
        expressionIndex = _colExpression[0];
        break;
    case COMP_GREEN:
        expressionIndex = _colExpression[1];
        break;
    case COMP_BLUE:
        expressionIndex = _colExpression[2];
        break;
    case COMP_ALPHA:
        expressionIndex = _colExpression[3];
        break;
    case COMP_RGB:
        // Select if all RGB are using the same expression
        if (_colExpression[0] == _colExpression[1] && _colExpression[1] == _colExpression[2])
        {
            expressionIndex = _colExpression[0];
        }
        break;
    case COMP_RGBA:
        // Select if all RGBA are using the same expression
        if (_colExpression[0] == _colExpression[1] && 
            _colExpression[1] == _colExpression[2] && 
            _colExpression[2] == _colExpression[3])
        {
            expressionIndex = _colExpression[0];
        }
        break;
    };

    return expressionIndex != NOT_DEFINED ? _expressions[expressionIndex] : NULL_EXPRESSION;
}

void Doom3ShaderLayer::setColourExpression(ColourComponentSelector comp, const IShaderExpressionPtr& expr)
{
	// Store the expression and link it to our registers
    auto expressionIndex = _expressions.size();
	_expressions.emplace_back(expr);

	std::size_t index = expr->linkToRegister(_registers);

	// Now assign the index to our colour components
	switch (comp)
	{
	case COMP_RED:
		_colIdx[0] = index;
		_colExpression[0] = expressionIndex;
		break;
	case COMP_GREEN:
		_colIdx[1] = index;
        _colExpression[1] = expressionIndex;
		break;
	case COMP_BLUE:
		_colIdx[2] = index;
        _colExpression[2] = expressionIndex;
		break;
	case COMP_ALPHA:
		_colIdx[3] = index;
        _colExpression[3] = expressionIndex;
		break;
	case COMP_RGB:
		_colIdx[0] = index;
		_colIdx[1] = index;
		_colIdx[2] = index;
        _colExpression[0] = expressionIndex;
        _colExpression[1] = expressionIndex;
        _colExpression[2] = expressionIndex;
		break;
	case COMP_RGBA:
		_colIdx[0] = index;
		_colIdx[1] = index;
		_colIdx[2] = index;
		_colIdx[3] = index;
        _colExpression[0] = expressionIndex;
        _colExpression[1] = expressionIndex;
        _colExpression[2] = expressionIndex;
        _colExpression[3] = expressionIndex;
		break;
	};
}

void Doom3ShaderLayer::setColour(const Vector4& col)
{
	// Assign all 3 components of the colour, allocating new registers on the fly where needed
	for (std::size_t i = 0; i < 4; ++i)
	{
		// Does this colour component refer to a reserved constant index?
		if (_colIdx[i] < NUM_RESERVED_REGISTERS)
		{
			// Yes, break this up by allocating a new register for this value
			_colIdx[i] = getNewRegister(static_cast<float>(col[i]));
		}
		else
		{
			// Already using a custom register
            setRegister(_colIdx[i], static_cast<float>(col[i]));
		}
	}
}

ShaderLayer::VertexColourMode Doom3ShaderLayer::getVertexColourMode() const
{
    return _vertexColourMode;
}

ShaderLayer::CubeMapMode Doom3ShaderLayer::getCubeMapMode() const
{
    return _cubeMapMode;
}

ShaderLayer::MapType Doom3ShaderLayer::getMapType() const
{
    return _mapType;
}

void Doom3ShaderLayer::setMapType(MapType type)
{
    _mapType = type;
}

const Vector2& Doom3ShaderLayer::getRenderMapSize()
{
    return _renderMapSize;
}

void Doom3ShaderLayer::setRenderMapSize(const Vector2& size)
{
    _renderMapSize = size;
}

bool Doom3ShaderLayer::hasAlphaTest() const
{
    return _alphaTest != REG_ZERO;
}

float Doom3ShaderLayer::getAlphaTest() const
{
    return _registers[_alphaTest];
}

TexturePtr Doom3ShaderLayer::getFragmentMapTexture(int index)
{
	if (index < 0 || index >= static_cast<int>(_fragmentMaps.size()))
	{
		return TexturePtr();
	}

	return GetTextureManager().getBinding(std::dynamic_pointer_cast<NamedBindable>(_fragmentMaps[index].map));
}

const Doom3ShaderLayer::FragmentMap& Doom3ShaderLayer::getFragmentMap(int index)
{
    assert(index >= 0 && index < static_cast<int>(_fragmentMaps.size()));

    return _fragmentMaps[index];
}

void Doom3ShaderLayer::addFragmentMap(const ShaderLayer::FragmentMap& fragmentMap)
{
    assert(fragmentMap.index >= 0);

    if (fragmentMap.index >= _fragmentMaps.size())
    {
        _fragmentMaps.resize(fragmentMap.index + 1);
    }

    _fragmentMaps[fragmentMap.index] = fragmentMap;
}

std::string Doom3ShaderLayer::getMapImageFilename()
{
    auto image = std::dynamic_pointer_cast<ImageExpression>(_bindableTex);

    if (image)
    {
        return image->getIdentifier();
    }
     
    return std::string();
}

IMapExpression::Ptr Doom3ShaderLayer::getMapExpression()
{
    return std::dynamic_pointer_cast<IMapExpression>(_bindableTex);
}

int Doom3ShaderLayer::getParseFlags()
{
    return _parseFlags;
}

void Doom3ShaderLayer::setParseFlag(ParseFlags flag)
{
    _parseFlags |= flag;
}

Vector4 Doom3ShaderLayer::getVertexParmValue(int parm)
{
    if (static_cast<std::size_t>(parm) >= _vertexParms.size() / 4)
    {
        return Vector4(0, 0, 0, 1);
    }

    std::size_t offset = parm * 4;

    return Vector4(_registers[_vertexParms[offset + 0]], _registers[_vertexParms[offset + 1]],
        _registers[_vertexParms[offset + 2]], _registers[_vertexParms[offset + 3]]);
}

const ShaderLayer::VertexParm& Doom3ShaderLayer::getVertexParm(int parm)
{
    return _vertexParmDefinitions[parm];
}

int Doom3ShaderLayer::getNumVertexParms()
{
    return static_cast<int>(_vertexParmDefinitions.size());
}

void Doom3ShaderLayer::addVertexParm(const VertexParm& parm)
{
    if (_vertexParmDefinitions.size() <= parm.index)
    {
        _vertexParmDefinitions.resize(parm.index + 1);
    }

    // Store the expressions in a separate location
    _vertexParmDefinitions[parm.index] = parm;

    assert(parm.expressions[0]);

    _expressions.emplace_back(parm.expressions[0]);
    std::size_t parm0Reg = parm.expressions[0]->linkToRegister(_registers);

    if (_vertexParms.size() <= (parm.index + 1) * 4)
    {
        _vertexParms.resize((parm.index + 1) * 4, REG_ZERO);
    }

    auto offset = parm.index * 4;
    _vertexParms[offset + 0] = parm0Reg;

    if (parm.expressions[1])
    {
        _expressions.emplace_back(parm.expressions[1]);
        _vertexParms[offset + 1] = parm.expressions[1]->linkToRegister(_registers);

        if (parm.expressions[2])
        {
            _expressions.emplace_back(parm.expressions[2]);
            _vertexParms[offset + 2] = parm.expressions[2]->linkToRegister(_registers);

            if (parm.expressions[3])
            {
                _expressions.emplace_back(parm.expressions[3]);
                _vertexParms[offset + 3] = parm.expressions[3]->linkToRegister(_registers);
            }
            else
            {
                // No fourth parameter set, set w to 1
                _vertexParms[offset + 3] = REG_ONE;
            }
        }
        else
        {
            // Only 2 expressions given, set z and w to 0 and 1, respectively.
            _vertexParms[offset + 2] = REG_ZERO;
            _vertexParms[offset + 3] = REG_ONE;
        }
    }
    else
    {
        // no parm1 given, repeat the one we have 4 times => insert 3 more times
        _vertexParms[offset + 1] = parm0Reg;
        _vertexParms[offset + 2] = parm0Reg;
        _vertexParms[offset + 3] = parm0Reg;
    }

    // At this point the array needs to be empty or its size a multiple of 4
    assert(_vertexParms.size() % 4 == 0);
}

}

