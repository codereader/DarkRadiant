#pragma once

#include <ishaders.h>

#include "NamedBindable.h"

namespace shaders
{

typedef std::pair<std::string, std::string> StringPair;


/**
 * \brief
 * Implementation of ShaderLayer for Doom 3 shaders.
 */
class Doom3ShaderLayer
: public ShaderLayer
{
private:

	// The bindable texture for this stage
	NamedBindablePtr _bindableTex;

    // The Texture object, created from the bindable texture
    mutable TexturePtr _texture;

    // Layer type (diffuse, bump, specular or nothing)
    ShaderLayer::Type _type;

    // Blend function as strings (e.g. "GL_ONE", "GL_ZERO")
    StringPair _blendFuncStrings;

    // Multiplicative layer colour (set with "red 0.6", "green 0.2" etc)
    Vector3 _colour;

    // Vertex colour blend mode
    VertexColourMode _vertexColourMode;

    // Cube map mode
    CubeMapMode _cubeMapMode;

    // Alpha test value. -1 means no test, otherwise must be 0 - 1
    float _alphaTest;

public:

	// Constructor
	Doom3ShaderLayer(ShaderLayer::Type type = ShaderLayer::BLEND,
                     NamedBindablePtr btex = NamedBindablePtr()) 
	: _bindableTex(btex),
	  _type(type), 
	  _blendFuncStrings("GL_ONE", "GL_ZERO"), 
      _colour(1, 1, 1),
      _vertexColourMode(VERTEX_COLOUR_NONE),
      _cubeMapMode(CUBE_MAP_NONE),
      _alphaTest(-1.0)
	{ }

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
    void setColour(const Vector3& col)
    {
        _colour = col;
    }

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

