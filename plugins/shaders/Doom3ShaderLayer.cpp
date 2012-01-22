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
	_bindableTex(btex),
	_type(type),
	_blendFuncStrings("gl_one", "gl_zero"), // needs to be lowercase
	_vertexColourMode(VERTEX_COLOUR_NONE),
	_cubeMapMode(CUBE_MAP_NONE),
	_stageFlags(0),
	_clampType(CLAMP_REPEAT),
	_alphaTest(REG_ZERO),
	_texGenType(TEXGEN_NORMAL),
	_privatePolygonOffset(0)
{
	_registers[REG_ZERO] = 0;
	_registers[REG_ONE] = 1;

	// Init the colour to 1,1,1,1
	_colIdx[0] = REG_ONE;
	_colIdx[1] = REG_ONE;
	_colIdx[2] = REG_ONE;
	_colIdx[3] = REG_ONE;

	// Scale is set to 1,1 by default
	_scale[0] = REG_ONE;
	_scale[1] = REG_ONE;

	// Translation is set to 0,0 by default
	_translation[0] = REG_ZERO;
	_translation[1] = REG_ZERO;

	// Rotation is set to 0 by default
	_rotation = REG_ZERO;

	// No shearing so far
	_shear[0] = REG_ZERO;
	_shear[1] = REG_ZERO;

	_texGenParams[0] = _texGenParams[1] = _texGenParams[2] = 0;
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

void Doom3ShaderLayer::setColourExpression(ColourComponentSelector comp, const IShaderExpressionPtr& expr)
{
	// Store the expression and link it to our registers
	_expressions.push_back(expr);

	std::size_t index = expr->linkToRegister(_registers);

	// Now assign the index to our colour components
	switch (comp)
	{
	case COMP_RED:
		_colIdx[0] = index;
		break;
	case COMP_GREEN:
		_colIdx[1] = index;
		break;
	case COMP_BLUE:
		_colIdx[2] = index;
		break;
	case COMP_ALPHA:
		_colIdx[3] = index;
		break;
	case COMP_RGB:
		_colIdx[0] = index;
		_colIdx[1] = index;
		_colIdx[2] = index;
		break;
	case COMP_RGBA:
		_colIdx[0] = index;
		_colIdx[1] = index;
		_colIdx[2] = index;
		_colIdx[3] = index;
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
			_colIdx[i] = getNewRegister(col[i]);
		}
		else
		{
			// Already using a custom register
			setRegister(_colIdx[i], col[i]);
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

float Doom3ShaderLayer::getAlphaTest() const
{
    return _registers[_alphaTest];
}

TexturePtr Doom3ShaderLayer::getFragmentMap(int index)
{
	if (index < 0 || index >= _fragmentMaps.size())
	{
		return TexturePtr();
	}

	return GetTextureManager().getBinding(_fragmentMaps[index]);
}

}

