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

inline IShaderExpression::Ptr getDefaultExpressionForTransformType(IShaderLayer::TransformType type)
{
    if (type == IShaderLayer::TransformType::CenterScale ||
        type == IShaderLayer::TransformType::Scale)
    {
        return ShaderExpression::createConstant(1);
    }

    return ShaderExpression::createConstant(0);
}

// IShaderLayer implementation

const IShaderExpression::Ptr Doom3ShaderLayer::NULL_EXPRESSION;

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
    _textureMatrix(_expressionSlots, _registers),
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

    // Initialise the texture matrix to an identity transform
    _textureMatrix.setIdentity();

    _expressionSlots[Expression::TexGenParam1].registerIndex = REG_ZERO;
    _expressionSlots[Expression::TexGenParam2].registerIndex = REG_ZERO;
    _expressionSlots[Expression::TexGenParam3].registerIndex = REG_ZERO;
}

Doom3ShaderLayer::Doom3ShaderLayer(const Doom3ShaderLayer& other, ShaderTemplate& material) :
    _material(material),
    _registers(other._registers),
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
    _transformations(other._transformations),
    _textureMatrix(_expressionSlots, _registers), // no copying necessary
    _vertexProgram(other._vertexProgram),
    _fragmentProgram(other._fragmentProgram),
    _vertexParms(other._vertexParms),
    _vertexParmDefinitions(other._vertexParmDefinitions),
    _fragmentMaps(other._fragmentMaps),
    _privatePolygonOffset(other._privatePolygonOffset),
    _renderMapSize(other._renderMapSize),
    _parseFlags(other._parseFlags)
{}

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

void Doom3ShaderLayer::appendTransformation(const Transformation& transform)
{
    Transformation copy(transform);

    if (!copy.expression1)
    {
        copy.expression1 = getDefaultExpressionForTransformType(transform.type);
    }

    if (!copy.expression2 && transform.type != TransformType::Rotate)
    {
        copy.expression2 = getDefaultExpressionForTransformType(transform.type);
    }

    // Store this original transformation, we need it later
    _transformations.emplace_back(copy);

    // Construct a transformation matrix and multiply it on top of the existing one
    _textureMatrix.applyTransformation(copy);
}

const std::vector<IShaderLayer::Transformation>& Doom3ShaderLayer::getTransformations()
{
    return _transformations;
}

Matrix4 Doom3ShaderLayer::getTextureTransform()
{
    return _textureMatrix.getMatrix4();
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

    return Vector4(_registers[_vertexParms[offset + 0].registerIndex],
                   _registers[_vertexParms[offset + 1].registerIndex],
                   _registers[_vertexParms[offset + 2].registerIndex],
                   _registers[_vertexParms[offset + 3].registerIndex]);
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
    assert(parm.expressions[0]);

    if (_vertexParmDefinitions.size() <= parm.index)
    {
        _vertexParmDefinitions.resize(parm.index + 1);
    }

    // Store the expressions in a separate location
    _vertexParmDefinitions[parm.index] = parm;

    // Resize the parms array, it will take multiples of 4
    if (_vertexParms.size() <= (parm.index + 1) * 4)
    {
        _vertexParms.resize((parm.index + 1) * 4);
    }

    auto offset = parm.index * 4;

    // Store the first expression
    _vertexParms[offset + 0].expression = parm.expressions[0];

    std::size_t parm0Reg = parm.expressions[0]->linkToRegister(_registers);
    _vertexParms[offset + 0].registerIndex = parm0Reg;

    if (parm.expressions[1])
    {
        _vertexParms[offset + 1].expression = parm.expressions[1];
        _vertexParms[offset + 1].registerIndex = parm.expressions[1]->linkToRegister(_registers);

        if (parm.expressions[2])
        {
            _vertexParms[offset + 2].expression = parm.expressions[2];
            _vertexParms[offset + 2].registerIndex = parm.expressions[2]->linkToRegister(_registers);

            if (parm.expressions[3])
            {
                _vertexParms[offset + 3].expression = parm.expressions[3];
                _vertexParms[offset + 3].registerIndex = parm.expressions[3]->linkToRegister(_registers);
            }
            else
            {
                // No fourth parameter set, set w to 1
                _vertexParms[offset + 3].registerIndex = REG_ONE;
            }
        }
        else
        {
            // Only 2 expressions given, set z and w to 0 and 1, respectively.
            _vertexParms[offset + 2].registerIndex = REG_ZERO;
            _vertexParms[offset + 3].registerIndex = REG_ONE;
        }
    }
    else
    {
        // no parm1 given, repeat the one we have 4 times => insert 3 more times
        _vertexParms[offset + 1].registerIndex = parm0Reg;
        _vertexParms[offset + 2].registerIndex = parm0Reg;
        _vertexParms[offset + 3].registerIndex = parm0Reg;
    }

    // At this point the array needs to be empty or its size a multiple of 4
    assert(_vertexParms.size() % 4 == 0);
}

void Doom3ShaderLayer::recalculateTransformationMatrix()
{
    _textureMatrix.setIdentity();

    for (const auto& transform : _transformations)
    {
        _textureMatrix.applyTransformation(transform);
    }
}

std::size_t Doom3ShaderLayer::addTransformation(TransformType type, const std::string& expression1, const std::string& expression2)
{
    _transformations.emplace_back(Transformation
    {
        type,
        ShaderExpression::createFromString(expression1),
        type != TransformType::Rotate ? ShaderExpression::createFromString(expression2) : IShaderExpression::Ptr()
    });

    recalculateTransformationMatrix();

    return _transformations.size() - 1;
}

void Doom3ShaderLayer::removeTransformation(std::size_t index)
{
    assert(index >= 0 && index < _transformations.size());

    _transformations.erase(_transformations.begin() + index);

    recalculateTransformationMatrix();
}

void Doom3ShaderLayer::updateTransformation(std::size_t index, TransformType type, const std::string& expression1, const std::string& expression2)
{
    assert(index >= 0 && index < _transformations.size());

    _transformations[index].type = type;
    auto expr1 = ShaderExpression::createFromString(expression1);
    _transformations[index].expression1 = expr1 ? expr1 : getDefaultExpressionForTransformType(type);;

    if (type != TransformType::Rotate)
    {
        auto expr2 = ShaderExpression::createFromString(expression2);
        _transformations[index].expression2 = expr2 ? expr2 : getDefaultExpressionForTransformType(type);
    }
    else
    {
        _transformations[index].expression2.reset();
    }

    recalculateTransformationMatrix();
}

void Doom3ShaderLayer::setColourExpressionFromString(ColourComponentSelector component, const std::string& expression)
{
    if (expression.empty())
    {
        setColourExpression(component, IShaderExpression::Ptr());
        return;
    }

    auto expr = ShaderExpression::createFromString(expression);

    if (expr)
    {
        setColourExpression(component, expr);
    }
}

void Doom3ShaderLayer::setConditionExpressionFromString(const std::string& expression)
{
    if (expression.empty())
    {
        setCondition(IShaderExpression::Ptr());
        return;
    }

    auto expr = ShaderExpression::createFromString(expression);

    if (expr)
    {
        setCondition(expr);
    }
}

}
