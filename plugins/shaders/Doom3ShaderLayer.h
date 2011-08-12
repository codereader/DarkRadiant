#pragma once

#include <ishaders.h>
#include <vector>

#include "math/Vector4.h"
#include "NamedBindable.h"

namespace shaders
{

typedef std::pair<std::string, std::string> StringPair;

class ShaderTemplate;

/**
 * \brief
 * Implementation of ShaderLayer for Doom 3 shaders.
 */
class Doom3ShaderLayer
: public ShaderLayer
{
public:
	// An enum used to select which colour components are affected by an operation
	enum ColourComponentSelector
	{
		COMP_RED,	// red only
		COMP_GREEN,	// green only
		COMP_BLUE,	// blue only
		COMP_ALPHA,	// alpha only
		COMP_RGB,	// red, green and blue
		COMP_RGBA,	// all: red, greeb, blue, alpha
	};

private:
	// The owning material template
	ShaderTemplate& _material;

	// The registers keeping the results of expression evaluations
	Registers _registers;

	// The expressions used in this stage
	typedef std::vector<IShaderExpressionPtr> Expressions;
	Expressions _expressions;

	static const IShaderExpressionPtr NULL_EXPRESSION;

	// The condition register for this stage. Points to a register to be interpreted as bool.
	std::size_t _condition;

	// The bindable texture for this stage
	NamedBindablePtr _bindableTex;

    // The Texture object, created from the bindable texture
    mutable TexturePtr _texture;

    // Layer type (diffuse, bump, specular or nothing)
    ShaderLayer::Type _type;

    // Blend function as strings (e.g. "gl_one", "gl_zero")
    StringPair _blendFuncStrings;

    // Multiplicative layer colour (set with "red 0.6", "green 0.2" etc)
	// The 4 numbers are indices into the registers array in the parent material
	std::size_t _colour[4];

    // Vertex colour blend mode
    VertexColourMode _vertexColourMode;

    // Cube map mode
    CubeMapMode _cubeMapMode;

	// Flags for this stage (forceHighQuality, ignoreAlphaTest, etc.)
	int _stageFlags;

	// Per-stage clamping
	ClampType _clampType;

    // Alpha test value, pointing into the register array. 0 means no test, otherwise must be within (0 - 1]
    std::size_t _alphaTest;

	// texgen normal, reflect, skybox, wobblesky
	TexGenType _texGenType;
	float _texGenParams[3]; // 3 parameters for wobblesky texgen

	// The register indices of this stage's scale expressions
	std::size_t _scale[2];

	// The register indices of this stage's translate expressions
	std::size_t _translation[2];

	// The rotation register index
	std::size_t _rotation;

	// The register indices of this stage's shear expressions
	std::size_t _shear[2];

	// The shader programs used in this stage
	std::string _vertexProgram;
	std::string _fragmentProgram;

public:

	// Constructor
	Doom3ShaderLayer(ShaderTemplate& material, 
					 ShaderLayer::Type type = ShaderLayer::BLEND,
                     const NamedBindablePtr& btex = NamedBindablePtr());

    /* ShaderLayer implementation */
    TexturePtr getTexture() const;
    BlendFunc getBlendFunc() const;
    Vector4 getColour() const;
    VertexColourMode getVertexColourMode() const;
    CubeMapMode getCubeMapMode() const;
    float getAlphaTest() const;

	// True if the condition for this stage is fulfilled 
	// (expressions must have been evaluated before this call)
	bool isVisible() const
	{
		return _registers[_condition] != 0;
	}

	void setCondition(const IShaderExpressionPtr& conditionExpr)
	{
		// Store the expression in our list
		_expressions.push_back(conditionExpr);

		// Link the result to our local registers
		_condition = conditionExpr->linkToRegister(_registers);
	}

	void evaluateExpressions(std::size_t time) 
	{
		for (Expressions::iterator i = _expressions.begin(); i != _expressions.end(); ++i)
		{
			(*i)->evaluate(time);
		}
	}

    /**
     * \brief
     * Set the bindable texture object.
     */
    void setBindableTexture(NamedBindablePtr btex)
    {
        _bindableTex = btex;
    }

    /**
     * \brief
     * Get the bindable texture object.
     */
    NamedBindablePtr getBindableTexture() const
    {
        return _bindableTex;
    }

    /**
     * \brief
     * Set the layer type.
     */
    void setLayerType(ShaderLayer::Type type)
    {
        _type = type;
    }

    /**
     * \brief
     * Get the layer type.
     */
    ShaderLayer::Type getType() const
    {
        return _type;
    }

	int getStageFlags() const
	{
		return _stageFlags;
	}

	void setStageFlags(int flags)
	{
		_stageFlags = flags;
	}

	void setStageFlag(ShaderLayer::Flags flag)
	{
		_stageFlags |= flag;
	}

	void clearStageFlag(ShaderLayer::Flags flag)
	{
		_stageFlags &= ~flag;
	}

	ClampType getClampType() const 
	{
		return _clampType;
	}

	void setClampType(ClampType type) 
	{
		_clampType = type;
	}

	TexGenType getTexGenType() const
	{
		return _texGenType;
	}

	void setTexGenType(TexGenType type)
	{
		_texGenType = type;
	}

	float getTexGenParam(std::size_t index) const 
	{
		assert(index < 3);
		return _texGenParams[index];
	}

	void setTexGenParam(std::size_t index, float value)
	{
		assert(index < 3);
		_texGenParams[index] = value;
	}

    /**
     * \brief
     * Set the blend function string.
     */
    void setBlendFuncStrings(const StringPair& func)
    {
        _blendFuncStrings = func;
    }

    /**
     * \brief
     * Set vertex colour mode.
     */
    void setVertexColourMode(VertexColourMode mode)
    {
        _vertexColourMode = mode;
    }

    /**
     * \brief
     * Set the colour to a constant Vector4. Calling this method
	 * will override any colour expressions that might have been assigned to
	 * this shader layer at an earlier point.
     */
    void setColour(const Vector4& col);

	/**
	 * Set the given colour component to use the given expression. This can be a single
	 * component out of the 4 available ones (R, G, B, A) or one of the two combos RGB and RGBA.
	 */
	void setColourExpression(ColourComponentSelector comp, const IShaderExpressionPtr& expr);
	
    /**
     * \brief
     * Set a texture object (overrides the map expression when getTexture is
     * called).
     */
    void setTexture(const TexturePtr& tex)
    {
        _texture = tex;
    }

	Vector2 getScale() 
	{
		return Vector2(_registers[_scale[0]], _registers[_scale[1]]);
	}

	/**
	 * Set the scale expressions of this stage, overwriting any previous scales.
	 */
	void setScale(const IShaderExpressionPtr& xExpr, const IShaderExpressionPtr& yExpr)
	{
		_expressions.push_back(xExpr);
		_expressions.push_back(yExpr);

		_scale[0] = xExpr->linkToRegister(_registers);
		_scale[1] = yExpr->linkToRegister(_registers);
	}

	Vector2 getTranslation() 
	{
		return Vector2(_registers[_translation[0]], _registers[_translation[1]]);
	}

	/**
	 * Set the "translate" expressions of this stage, overwriting any previous expressions.
	 */
	void setTranslation(const IShaderExpressionPtr& xExpr, const IShaderExpressionPtr& yExpr)
	{
		_expressions.push_back(xExpr);
		_expressions.push_back(yExpr);

		_translation[0] = xExpr->linkToRegister(_registers);
		_translation[1] = yExpr->linkToRegister(_registers);
	}

	float getRotation() 
	{
		return _registers[_rotation];
	}

	/**
	 * Set the "rotate" expression of this stage, overwriting any previous one.
	 */
	void setRotation(const IShaderExpressionPtr& expr)
	{
		_expressions.push_back(expr);

		_rotation = expr->linkToRegister(_registers);
	}

	Vector2 getShear() 
	{
		return Vector2(_registers[_shear[0]], _registers[_shear[1]]);
	}

	/**
	 * Set the shear expressions of this stage, overwriting any previous ones.
	 */
	void setShear(const IShaderExpressionPtr& xExpr, const IShaderExpressionPtr& yExpr)
	{
		_expressions.push_back(xExpr);
		_expressions.push_back(yExpr);

		_shear[0] = xExpr->linkToRegister(_registers);
		_shear[1] = yExpr->linkToRegister(_registers);
	}

    /**
     * \brief
     * Set cube map mode.
     */
    void setCubeMapMode(CubeMapMode mode)
    {
        _cubeMapMode = mode;
    }

    /**
     * \brief
     * Set alphatest expression
     */
    void setAlphaTest(const IShaderExpressionPtr& expr)
    {
		_expressions.push_back(expr);
		_alphaTest = expr->linkToRegister(_registers);
    }

	// Returns the value of the given register
	float getRegister(std::size_t index) const
	{
		assert(index < _registers.size());
		return _registers[index];
	}

	void setRegister(std::size_t index, float value)
	{
		assert(index < _registers.size());
		_registers[index] = value;
	}

	// Allocates a new register, initialised with the given value
	std::size_t getNewRegister(float newVal)
	{
		_registers.push_back(newVal);
		return _registers.size() - 1;
	}

	// Vertex program name
	const std::string& getVertexProgram()
	{
		return _vertexProgram;
	}

	void setVertexProgram(const std::string& name)
	{
		_vertexProgram = name;
	}

	// Fragment program name
	const std::string& getFragmentProgram()
	{
		return _fragmentProgram;
	}

	void setFragmentProgram(const std::string& name)
	{
		_fragmentProgram = name;
	}
};

/**
 * \brief
 * Pointer typedef for Doom3ShaderLayer.
 */
typedef boost::shared_ptr<Doom3ShaderLayer> Doom3ShaderLayerPtr;

}

