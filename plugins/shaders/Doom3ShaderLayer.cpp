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
	_condition(REG_ZERO),
	_bindableTex(btex),
	_type(type),
	_blendFuncStrings("gl_one", "gl_zero"), // needs to be lowercase
	_vertexColourMode(VERTEX_COLOUR_NONE),
	_cubeMapMode(CUBE_MAP_NONE),
	_stageFlags(0),
	_clampType(CLAMP_REPEAT),
	_alphaTest(REG_ONE),
	_texGenType(TEXGEN_NORMAL)
{ 
	_registers[REG_ZERO] = 0;
	_registers[REG_ONE] = 1;

	// Init the colour to 1,1,1,1
	_colour[0] = REG_ONE;
	_colour[1] = REG_ONE;
	_colour[2] = REG_ONE;
	_colour[3] = REG_ONE;

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

Vector4 Doom3ShaderLayer::getColour() const
{
	// Resolve the register values
    return Vector4(getRegister(_colour[0]), getRegister(_colour[1]), 
				   getRegister(_colour[2]), getRegister(_colour[3]));
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
		_colour[0] = index;
		break;
	case COMP_GREEN:
		_colour[1] = index;
		break;
	case COMP_BLUE:
		_colour[2] = index;
		break;
	case COMP_ALPHA:
		_colour[3] = index;
		break;
	case COMP_RGB:
		_colour[0] = index;
		_colour[1] = index;
		_colour[2] = index;
		break;
	case COMP_RGBA:
		_colour[0] = index;
		_colour[1] = index;
		_colour[2] = index;
		_colour[3] = index;
		break;
	};
}

void Doom3ShaderLayer::setColour(const Vector4& col)
{
	// Assign all 3 components of the colour, allocating new registers on the fly where needed
	for (std::size_t i = 0; i < 4; ++i)
	{
		// Does this colour component refer to a reserved constant index?
		if (_colour[i] < NUM_RESERVED_REGISTERS)
		{
			// Yes, break this up by allocating a new register for this value
			_colour[i] = getNewRegister(col[i]);
		}
		else
		{
			// Already using a custom register
			setRegister(_colour[i], col[i]);
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

}

