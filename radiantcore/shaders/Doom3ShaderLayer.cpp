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

// IShaderLayer implementation

const IShaderExpression::Ptr Doom3ShaderLayer::NULL_EXPRESSION;

void ExpressionSlots::assign(IShaderLayer::Expression::Slot slot, const IShaderExpression::Ptr& newExpression, std::size_t defaultRegisterIndex)
{
    auto& expressionSlot = at(slot);

    if (!newExpression)
    {
        expressionSlot.expression.reset();
        expressionSlot.registerIndex = defaultRegisterIndex;
        return;
    }

    // Non-empty expression, overwrite if we have an existing expression in the slot
    // Beware of the fact that some expressions could be shared across slots, before re-using the same register
    if (expressionSlot.expression && !registerIsShared(expressionSlot.registerIndex))
    {
        // We assume that if there was an expression in the slot, it shouldn't point to the default registers
        assert(expressionSlot.registerIndex != defaultRegisterIndex);
        
        // Re-use the register index
        expressionSlot.expression = newExpression;
        expressionSlot.expression->linkToSpecificRegister(_registers, expressionSlot.registerIndex);
    }
    else
    {
        expressionSlot.expression = newExpression;
        expressionSlot.registerIndex = expressionSlot.expression->linkToRegister(_registers);
    }
}

void ExpressionSlots::assignFromString(IShaderLayer::Expression::Slot slot, const std::string& expressionString, std::size_t defaultRegisterIndex)
{
    // An empty string will clear the expression
    if (expressionString.empty())
    {
        assign(slot, IShaderExpression::Ptr(), defaultRegisterIndex);
        return;
    }

    // Attempt to parse the string
    auto expression = ShaderExpression::createFromString(expressionString);

    if (!expression)
    {
        return; // parsing failures will not overwrite the expression slot
    }

    assign(slot, expression, defaultRegisterIndex);
}

bool ExpressionSlots::registerIsShared(std::size_t index) const
{
    std::size_t useCount = 0;

    for (const auto& slot : *this)
    {
        if (slot.registerIndex == index && ++useCount > 1)
        {
            return true;
        }
    }

    return false;
}

Doom3ShaderLayer::Doom3ShaderLayer(ShaderTemplate& material, IShaderLayer::Type type, const NamedBindablePtr& btex)
:	_material(material),
	_registers(NUM_RESERVED_REGISTERS),
    _expressionSlots(_registers),
	_bindableTex(btex),
	_type(type),
    _mapType(MapType::Map),
	_blendFuncStrings("gl_one", "gl_zero"), // needs to be lowercase
	_vertexColourMode(VERTEX_COLOUR_NONE),
	_cubeMapMode(CUBE_MAP_NONE),
	_stageFlags(0),
	_clampType(CLAMP_REPEAT),
	_texGenType(TEXGEN_NORMAL),
	_privatePolygonOffset(0),
    _parseFlags(0)
{
	_registers[REG_ZERO] = 0;
	_registers[REG_ONE] = 1;

    _expressionSlots[Expression::AlphaTest].registerIndex = REG_ZERO;
    _expressionSlots[Expression::Condition].registerIndex = REG_ONE;

	// Init the colour to 1,1,1,1
    _expressionSlots[Expression::ColourRed].registerIndex = REG_ONE;
    _expressionSlots[Expression::ColourGreen].registerIndex = REG_ONE;
    _expressionSlots[Expression::ColourBlue].registerIndex = REG_ONE;
    _expressionSlots[Expression::ColourAlpha].registerIndex = REG_ONE;

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

    _expressionSlots[Expression::TexGenParam1].registerIndex = REG_ZERO;
    _expressionSlots[Expression::TexGenParam2].registerIndex = REG_ZERO;
    _expressionSlots[Expression::TexGenParam3].registerIndex = REG_ZERO;
}

Doom3ShaderLayer::Doom3ShaderLayer(const Doom3ShaderLayer& other, ShaderTemplate& material) :
    _material(material),
    _registers(other._registers),
    _expressions(other._expressions),
    _expressionSlots(other._expressionSlots, _registers),
    _bindableTex(other._bindableTex),
    _texture(other._texture),
    _type(other._type),
    _mapType(other._mapType),
    _blendFuncStrings(other._blendFuncStrings),
    _vertexColourMode(other._vertexColourMode),
    _cubeMapMode(other._cubeMapMode),
    _stageFlags(other._stageFlags),
    _clampType(other._clampType),
    _texGenType(other._texGenType),
    _rotation(other._rotation),
    _rotationExpression(other._rotationExpression),
    _vertexProgram(other._vertexProgram),
    _fragmentProgram(other._fragmentProgram),
    _vertexParms(other._vertexParms),
    _vertexParmDefinitions(other._vertexParmDefinitions),
    _fragmentMaps(other._fragmentMaps),
    _privatePolygonOffset(other._privatePolygonOffset),
    _renderMapSize(other._renderMapSize),
    _parseFlags(other._parseFlags)
{
    _scale[0] = other._scale[0];
    _scale[1] = other._scale[1];
    _scaleExpression[0] = other._scaleExpression[0];
    _scaleExpression[1] = other._scaleExpression[1];

    _translation[0] = other._translation[0];
    _translation[1] = other._translation[1];
    _translationExpression[0] = other._translationExpression[0];
    _translationExpression[1] = other._translationExpression[1];

    _shear[0] = other._shear[0];
    _shear[1] = other._shear[1];
    _shearExpression[0] = other._shearExpression[0];
    _shearExpression[1] = other._shearExpression[1];
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
    Colour4 colour(getRegisterValue(_expressionSlots[Expression::ColourRed].registerIndex), 
                   getRegisterValue(_expressionSlots[Expression::ColourGreen].registerIndex),
                   getRegisterValue(_expressionSlots[Expression::ColourBlue].registerIndex), 
                   getRegisterValue(_expressionSlots[Expression::ColourAlpha].registerIndex));

    if (!colour.isValid())
    {
        return Colour4::WHITE();
    }

    return colour;
}

const IShaderExpression::Ptr& Doom3ShaderLayer::getColourExpression(ColourComponentSelector component) const
{
    switch (component)
    {
    case COMP_RED:
        return _expressionSlots[Expression::ColourRed].expression;
    case COMP_GREEN:
        return _expressionSlots[Expression::ColourGreen].expression;
    case COMP_BLUE:
        return _expressionSlots[Expression::ColourBlue].expression;
    case COMP_ALPHA:
        return _expressionSlots[Expression::ColourAlpha].expression;
    case COMP_RGB:
        // Select if all RGB are using the same expression
        if (_expressionSlots[Expression::ColourRed].expression == _expressionSlots[Expression::ColourGreen].expression && 
            _expressionSlots[Expression::ColourGreen].expression == _expressionSlots[Expression::ColourBlue].expression)
        {
            return _expressionSlots[Expression::ColourRed].expression;
        }
        break;
    case COMP_RGBA:
        if (_expressionSlots[Expression::ColourRed].expression == _expressionSlots[Expression::ColourGreen].expression &&
            _expressionSlots[Expression::ColourGreen].expression == _expressionSlots[Expression::ColourBlue].expression &&
            _expressionSlots[Expression::ColourBlue].expression == _expressionSlots[Expression::ColourAlpha].expression)
        {
            return _expressionSlots[Expression::ColourRed].expression;
        }
        break;
    };

    return NULL_EXPRESSION;
}

void Doom3ShaderLayer::setColourExpression(ColourComponentSelector comp, const IShaderExpression::Ptr& expr)
{
	// Now assign the index to our colour components
	switch (comp)
	{
	case COMP_RED:
        _expressionSlots.assign(Expression::ColourRed, expr, REG_ONE);
		break;
	case COMP_GREEN:
        _expressionSlots.assign(Expression::ColourGreen, expr, REG_ONE);
		break;
	case COMP_BLUE:
        _expressionSlots.assign(Expression::ColourBlue, expr, REG_ONE);
		break;
	case COMP_ALPHA:
        _expressionSlots.assign(Expression::ColourAlpha, expr, REG_ONE);
		break;
	case COMP_RGB:
        _expressionSlots.assign(Expression::ColourRed, expr, REG_ONE);
        _expressionSlots[Expression::ColourGreen].registerIndex = _expressionSlots[Expression::ColourRed].registerIndex;
        _expressionSlots[Expression::ColourGreen].expression = _expressionSlots[Expression::ColourRed].expression;
        _expressionSlots[Expression::ColourBlue].registerIndex = _expressionSlots[Expression::ColourRed].registerIndex;
        _expressionSlots[Expression::ColourBlue].expression = _expressionSlots[Expression::ColourRed].expression;
		break;
	case COMP_RGBA:
        _expressionSlots.assign(Expression::ColourRed, expr, REG_ONE);
        _expressionSlots[Expression::ColourGreen].registerIndex = _expressionSlots[Expression::ColourRed].registerIndex;
        _expressionSlots[Expression::ColourGreen].expression = _expressionSlots[Expression::ColourRed].expression;
        _expressionSlots[Expression::ColourBlue].registerIndex = _expressionSlots[Expression::ColourRed].registerIndex;
        _expressionSlots[Expression::ColourBlue].expression = _expressionSlots[Expression::ColourRed].expression;
        _expressionSlots[Expression::ColourAlpha].registerIndex = _expressionSlots[Expression::ColourRed].registerIndex;
        _expressionSlots[Expression::ColourAlpha].expression = _expressionSlots[Expression::ColourRed].expression;
		break;
	};
}

void Doom3ShaderLayer::setColour(const Vector4& col)
{
	// Assign all 3 components of the colour, allocating new registers on the fly where needed
	for (std::size_t i = 0; i < 4; ++i)
	{
        auto slot = static_cast<Expression::Slot>(Expression::ColourRed + i);

		// Does this colour component refer to a reserved constant index?
		if (_expressionSlots[slot].registerIndex < NUM_RESERVED_REGISTERS)
		{
			// Yes, break this up by allocating a new register for this value
            _expressionSlots[slot].registerIndex = getNewRegister(static_cast<float>(col[i]));
		}
		else
		{
			// Already using a custom register
            setRegister(_expressionSlots[slot].registerIndex, static_cast<float>(col[i]));
		}
	}
}

IShaderLayer::VertexColourMode Doom3ShaderLayer::getVertexColourMode() const
{
    return _vertexColourMode;
}

IShaderLayer::CubeMapMode Doom3ShaderLayer::getCubeMapMode() const
{
    return _cubeMapMode;
}

IShaderLayer::MapType Doom3ShaderLayer::getMapType() const
{
    return _mapType;
}

void Doom3ShaderLayer::setMapType(MapType type)
{
    _mapType = type;
}

const Vector2& Doom3ShaderLayer::getRenderMapSize() const
{
    return _renderMapSize;
}

void Doom3ShaderLayer::setRenderMapSize(const Vector2& size)
{
    _renderMapSize = size;
}

bool Doom3ShaderLayer::hasAlphaTest() const
{
    return _expressionSlots[Expression::AlphaTest].expression != nullptr;
}

float Doom3ShaderLayer::getAlphaTest() const
{
    return _registers[_expressionSlots[Expression::AlphaTest].registerIndex];
}

const IShaderExpression::Ptr& Doom3ShaderLayer::getAlphaTestExpression() const
{
    return _expressionSlots[Expression::AlphaTest].expression;
}

TexturePtr Doom3ShaderLayer::getFragmentMapTexture(int index) const
{
	if (index < 0 || index >= static_cast<int>(_fragmentMaps.size()))
	{
		return TexturePtr();
	}

	return GetTextureManager().getBinding(std::dynamic_pointer_cast<NamedBindable>(_fragmentMaps[index].map));
}

const Doom3ShaderLayer::FragmentMap& Doom3ShaderLayer::getFragmentMap(int index) const
{
    assert(index >= 0 && index < static_cast<int>(_fragmentMaps.size()));

    return _fragmentMaps[index];
}

void Doom3ShaderLayer::addFragmentMap(const IShaderLayer::FragmentMap& fragmentMap)
{
    assert(fragmentMap.index >= 0);

    if (fragmentMap.index >= _fragmentMaps.size())
    {
        _fragmentMaps.resize(fragmentMap.index + 1);
    }

    _fragmentMaps[fragmentMap.index] = fragmentMap;
}

std::string Doom3ShaderLayer::getMapImageFilename() const
{
    auto image = std::dynamic_pointer_cast<ImageExpression>(_bindableTex);

    if (image)
    {
        return image->getIdentifier();
    }
     
    return std::string();
}

IMapExpression::Ptr Doom3ShaderLayer::getMapExpression() const
{
    return std::dynamic_pointer_cast<IMapExpression>(_bindableTex);
}

void Doom3ShaderLayer::setMapExpressionFromString(const std::string& expression)
{
    _texture.reset();
    setBindableTexture(MapExpression::createForString(expression));
}

int Doom3ShaderLayer::getParseFlags() const
{
    return _parseFlags;
}

void Doom3ShaderLayer::setParseFlag(ParseFlags flag)
{
    _parseFlags |= flag;
}

Vector4 Doom3ShaderLayer::getVertexParmValue(int parm) const
{
    if (static_cast<std::size_t>(parm) >= _vertexParms.size() / 4)
    {
        return Vector4(0, 0, 0, 1);
    }

    std::size_t offset = parm * 4;

    return Vector4(_registers[_vertexParms[offset + 0]], _registers[_vertexParms[offset + 1]],
        _registers[_vertexParms[offset + 2]], _registers[_vertexParms[offset + 3]]);
}

const IShaderLayer::VertexParm& Doom3ShaderLayer::getVertexParm(int parm) const
{
    return _vertexParmDefinitions[parm];
}

int Doom3ShaderLayer::getNumVertexParms() const
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

void Doom3ShaderLayer::assignExpression(const IShaderExpression::Ptr& expression,
    std::size_t& expressionIndex, std::size_t& registerIndex, std::size_t defaultRegisterIndex)
{
    if (!expression)
    {
        // Assigning an empty expression will reset the slot to NOT_DEFINED
        if (expressionIndex != NOT_DEFINED)
        {
            // Release the previous expression
            assert(_expressions[expressionIndex]);

            _expressions[expressionIndex]->unlinkFromRegisters();
            _expressions[expressionIndex].reset();
        }

        expressionIndex = NOT_DEFINED;
        registerIndex = defaultRegisterIndex;
        return;
    }

    // Non-empty expression, overwrite if we have an existing expression in the slot
    if (expressionIndex != NOT_DEFINED)
    {
        // Try to re-use the previous register position
        auto previousExpression = _expressions[expressionIndex];
        _expressions[expressionIndex] = expression;

        if (previousExpression->isLinked())
        {
            registerIndex = previousExpression->unlinkFromRegisters();
            expression->linkToSpecificRegister(_registers, registerIndex);
        }
        else
        {
            registerIndex = expression->linkToRegister(_registers);
        }
    }
    else
    {
        expressionIndex = _expressions.size();
        _expressions.emplace_back(expression);

        registerIndex = expression->linkToRegister(_registers);
    }
}

void Doom3ShaderLayer::assignExpressionFromString(const std::string& expressionString, 
    std::size_t& expressionIndex, std::size_t& registerIndex, std::size_t defaultRegisterIndex)
{
    // An empty string will clear the expression
    if (expressionString.empty())
    {
        assignExpression(IShaderExpression::Ptr(), expressionIndex, registerIndex, defaultRegisterIndex);
        return;
    }

    // Attempt to parse the string
    auto expression = ShaderExpression::createFromString(expressionString);

    if (!expression)
    {
        return; // parsing failures will not overwrite the expression slot
    }

    assignExpression(expression, expressionIndex, registerIndex, defaultRegisterIndex);
}

}
