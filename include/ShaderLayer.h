#pragma once

#include "Texture.h"

#include <boost/shared_ptr.hpp>
#include <vector>

/**
 * \brief
 * Representation of a GL blend function.
 *
 * A GL blend function consists of two GLenums representing the operations that
 * should be performed on the source and destination pixel colours respectively,
 * before the two results are added together into a final pixel colour.
 */
class BlendFunc
{
public:

    // Source pixel function
    GLenum src;

    // Destination pixel function
    GLenum dest;

    // Constructor
    BlendFunc(GLenum s, GLenum d) 
    : src(s), dest(d)
    { }
};

/**
 * \brief
 * A single layer of a material shader.
 *
 * Each shader layer contains an image texture, a blend mode (e.g. add,
 * modulate) and various other data.
 */
class ShaderLayer 
{
public:

    /**
     * \brief
     * Enumeration of layer types.
     */
    enum Type
    {
        DIFFUSE,
        BUMP,
        SPECULAR,
        BLEND
    };

    /**
     * \brief
     * Return the layer type.
     */
    virtual Type getType() const = 0;

    /**
     * \brief
     * Return the Texture object corresponding to this layer (may be NULL).
     */
    virtual TexturePtr getTexture() const = 0;

    /**
     * \brief
     * Return the GL blend function for this layer.
     *
     * Only layers of type BLEND use a BlendFunc. Layers of type DIFFUSE, BUMP
     * and SPECULAR do not use blend functions.
     */
    virtual BlendFunc getBlendFunc() const = 0;

    /**
     * \brief
     * Multiplicative layer colour (set with "red 0.6", "green 0.2" etc)
     */
    virtual Vector3 getColour() const = 0;

    /**
     * \brief
     * Vertex colour blend mode enumeration.
     */
    enum VertexColourMode
    {
        VERTEX_COLOUR_NONE, // no vertex colours
        VERTEX_COLOUR_MULTIPLY, // "vertexColor"
        VERTEX_COLOUR_INVERSE_MULTIPLY // "inverseVertexColor"
    };
    
    /**
     * \brief
     * Get the vertex colour mode for this layer.
     */
    virtual VertexColourMode getVertexColourMode() const = 0;

    /**
     * \brief
     * Enumeration of cube map modes for this layer.
     */
    enum CubeMapMode
    {
        CUBE_MAP_NONE,
        CUBE_MAP_CAMERA, // cube map in camera space ("cameraCubeMap")
        CUBE_MAP_OBJECT  // cube map in object space ("cubeMap")
    };

    /**
     * \brief
     * Get the cube map mode for this layer.
     */
    virtual CubeMapMode getCubeMapMode() const = 0;

    /**
     * \brief
     * Get the alpha test value for this layer.
     *
     * \return
     * The alpha test value, between 0.0 and 1.0 if it is set. If no alpha test
     * value is set, -1 will be returned.
     */
    virtual double getAlphaTest() const = 0;
};

/**
 * \brief
 * Shader pointer for ShaderLayer,
 */
typedef boost::shared_ptr<ShaderLayer> ShaderLayerPtr;

/**
 * \brief
 * Vector of ShaderLayer pointers.
 */
typedef std::vector<ShaderLayerPtr> ShaderLayerVector;

