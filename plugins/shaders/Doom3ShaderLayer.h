#pragma once

#include <ishaders.h>
#include <vector>

#include "NamedBindable.h"

namespace shaders
{

typedef std::vector<float> Registers;

// The indices to the constants in the registers array
enum ReservedRegisters
{
	REG_ZERO = 0,
	REG_ONE  = 1,
	NUM_RESERVED_REGISTERS,
};

typedef std::pair<std::string, std::string> StringPair;

class ShaderTemplate;

/**
 * \brief
 * Implementation of ShaderLayer for Doom 3 shaders.
 */
class Doom3ShaderLayer
: public ShaderLayer
{
private:
	// The owning material (which holds the registers)
	ShaderTemplate& _material;

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

    // Alpha test value. -1 means no test, otherwise must be 0 - 1
    float _alphaTest;

	// texgen normal, reflect, skybox, wobblesky
	TexGenType _texGenType;
	float _texGenParams[3]; // 3 parameters for wobblesky texgen

public:

	// Constructor
	Doom3ShaderLayer(ShaderTemplate& material, 
					 ShaderLayer::Type type = ShaderLayer::BLEND,
                     NamedBindablePtr btex = NamedBindablePtr())
	: _material(material),
	  _bindableTex(btex),
	  _type(type),
	  _blendFuncStrings("gl_one", "gl_zero"), // needs to be lowercase
      _vertexColourMode(VERTEX_COLOUR_NONE),
      _cubeMapMode(CUBE_MAP_NONE),
	  _stageFlags(0),
	  _clampType(CLAMP_REPEAT),
      _alphaTest(-1.0),
	  _texGenType(TEXGEN_NORMAL)
	{ 
		// Init the colour to 1,1,1,1
		_colour[0] = REG_ONE;
		_colour[1] = REG_ONE;
		_colour[2] = REG_ONE;
		_colour[3] = REG_ONE;

		_texGenParams[0] = _texGenParams[1] = _texGenParams[2] = 0;
	}

    /* ShaderLayer implementation */
    TexturePtr getTexture() const;
    BlendFunc getBlendFunc() const;
    Vector3 getColour() const;
    VertexColourMode getVertexColourMode() const;
    CubeMapMode getCubeMapMode() const;
    double getAlphaTest() const;

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
     * Set the colour.
     */
    void setColour(const Vector3& col);

    /**
     * \brief
     * Set a texture object (overrides the map expression when getTexture is
     * called).
     */
    void setTexture(TexturePtr tex)
    {
        _texture = tex;
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
     * Set alphatest value
     */
    void setAlphaTest(float alphaTest)
    {
        _alphaTest = alphaTest;
    }
};

/**
 * \brief
 * Pointer typedef for Doom3ShaderLayer.
 */
typedef boost::shared_ptr<Doom3ShaderLayer> Doom3ShaderLayerPtr;

}

