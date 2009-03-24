#pragma once

#include <ishaders.h>

namespace shaders
{

enum LayerTypeId
{
  LAYER_NONE,
  LAYER_DIFFUSEMAP,
  LAYER_BUMPMAP,
  LAYER_SPECULARMAP
};

typedef std::pair<std::string, std::string> StringPair;


/**
 * \brief
 * Implementation of ShaderLayer for Doom 3 shaders.
 */
class Doom3ShaderLayer
: public ShaderLayer
{
private:

	// The Map Expression for this stage
	shaders::MapExpressionPtr _mapExpr;

    // The Texture object, created from the map expression
    mutable TexturePtr _texture;

    // Layer type (diffuse, bump, specular or nothing)
    LayerTypeId _type;

    // Blend function as strings (e.g. "GL_ONE", "GL_ZERO")
    StringPair _blendFuncStrings;

    // Multiplicative layer colour (set with "red 0.6", "green 0.2" etc)
    Vector3 _colour;

    // Vertex colour blend mode
    VertexColourMode _vertexColourMode;

public:

	// Constructor
	Doom3ShaderLayer() 
	: _mapExpr(shaders::MapExpressionPtr()),
	  _type(LAYER_NONE), 
	  _blendFuncStrings("GL_ONE", "GL_ZERO"), 
      _colour(1, 1, 1),
      _vertexColourMode(VERTEX_COLOUR_NONE)
	{ }

    /* ShaderLayer implementation */
    TexturePtr getTexture() const;
    BlendFunc getBlendFunc() const;
    Vector3 getColour() const;
    VertexColourMode getVertexColourMode() const;

    /**
     * \brief
     * Set the map expression.
     */
    void setMapExpression(MapExpressionPtr exp)
    {
        _mapExpr = exp;
    }

    /**
     * \brief
     * Get the map expression.
     */
    MapExpressionPtr getMapExpression() const
    {
        return _mapExpr;
    }

    /**
     * \brief
     * Set the layer type.
     */
    void setLayerType(LayerTypeId type)
    {
        _type = type;
    }

    /**
     * \brief
     * Get the layer type.
     */
    LayerTypeId getLayerType() const
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
};

/**
 * \brief
 * Pointer typedef for Doom3ShaderLayer.
 */
typedef boost::shared_ptr<Doom3ShaderLayer> Doom3ShaderLayerPtr;

}

