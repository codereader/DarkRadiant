#pragma once

#include "Texture.h"
#include "ishaderexpression.h"

#include <memory>
#include <vector>
#include <string>

#include "math/Vector2.h"
#include "math/Vector4.h"
#include "render/Colour4.h"

class IRenderEntity;

// Texture repeat types
enum ClampType
{
	CLAMP_REPEAT				= 1 << 0,		// default = no clamping
	CLAMP_NOREPEAT				= 1 << 1,		// "clamp"
	CLAMP_ZEROCLAMP				= 1 << 2,		// "zeroclamp"
	CLAMP_ALPHAZEROCLAMP		= 1 << 3,		// "alphazeroclamp"
};

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

	// Stage flags
	enum Flags
	{
		FLAG_IGNORE_ALPHATEST		= 1 << 0,
		FLAG_FILTER_NEAREST			= 1 << 1,
		FLAG_FILTER_LINEAR			= 1 << 2,
		FLAG_HIGHQUALITY			= 1 << 3,	// "uncompressed"
		FLAG_FORCE_HIGHQUALITY		= 1 << 4,
		FLAG_NO_PICMIP				= 1 << 5,
		FLAG_MASK_RED				= 1 << 6,
		FLAG_MASK_GREEN				= 1 << 7,
		FLAG_MASK_BLUE				= 1 << 8,
		FLAG_MASK_ALPHA				= 1 << 9,
		FLAG_MASK_DEPTH				= 1 << 10,
		FLAG_CENTERSCALE			= 1 << 11,  // whether to translate -0.5, scale and translate +0.5
		FLAG_IGNORE_DEPTH			= 1 << 12,  // use depthfunc always
	};

	enum TexGenType
	{
		TEXGEN_NORMAL		= 1 << 0,
		TEXGEN_REFLECT		= 1 << 1,
		TEXGEN_SKYBOX		= 1 << 2,
		TEXGEN_WOBBLESKY	= 1 << 3,
		TEXGEN_SCREEN	    = 1 << 4, // screen aligned, for mirrorRenders and screen space temporaries
	};

    /**
     * \brief
	 * Destructor
	 */
	virtual ~ShaderLayer() {}

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
	 * Evaluate all shader expressions used in this stage. Call this once (each frame) 
	 * before requesting things like getAlphaTest(), getColour() or isVisible()
	 */
	virtual void evaluateExpressions(std::size_t time) = 0;

	/**
	 * Like evaluateExpressions(time), but with an additional renderentity as argument
	 * to give this stage the ability to resolve parm0..parm11 values.
	 */
	virtual void evaluateExpressions(std::size_t time, const IRenderEntity& entity) = 0;

	/**
	 * The flags set on this stage.
	 */
	virtual int getStageFlags() const = 0;

	/**
	 * Each stage can have its own clamp type, overriding the per-material one.
	 */
	virtual ClampType getClampType() const = 0;

	/**
	 * Returns the texgen type: normal, reflect, skybox, etc.
	 * Use getTexGenParam(i) to retrieve the wobblesky parameters [0..2]
	 */
	virtual TexGenType getTexGenType() const = 0;

	/**
	 * TexGen type wobblesky has 3 parameters, get them here, with index in [0..2]
	 */
	virtual float getTexGenParam(std::size_t index) const = 0;

    /**
     * \brief
     * Return the GL blend function for this layer.
     *
     * Only layers of type BLEND use a BlendFunc. Layers of type DIFFUSE, BUMP
     * and SPECULAR do not use blend functions.
     */
    virtual BlendFunc getBlendFunc() const = 0;

    // Get the blend string as defined in the material def, e.g. "add" or "gl_one, gl_zero"
    virtual const std::pair<std::string, std::string>& getBlendFuncStrings() const = 0;

    /**
     * \brief
     * Multiplicative layer colour (set with "red 0.6", "green 0.2" etc)
     */
    virtual Colour4 getColour() const = 0;

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

    enum class MapType
    {
        Map,            // regular map
        CubeMap,        // corresponds to CUBE_MAP_OBJECT
        CameraCubeMap,  // corresponds to CUBE_MAP_CAMERA
        VideoMap,
        SoundMap,
        MirrorRenderMap,
        RemoteRenderMap,
    };

    // Get the map type used by this stage
    virtual MapType getMapType() const = 0;

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
     * Returns the dimensions specifying the map size for 
     * stages using the "mirrorRenderMap", "remoteRenderMap" keywords.
     */
    virtual const Vector2& getRenderMapSize() = 0;

	/**
	 * Returns the value of the scale expressions of this stage.
	 */
	virtual Vector2 getScale() = 0;

	/**
	 * Returns the value of the translate expressions of this stage.
	 */
	virtual Vector2 getTranslation() = 0;

	/**
	 * Returns the value of the rotate expression of this stage.
	 */
	virtual float getRotation() = 0;

	/**
	 * Returns the value of the 'shear' expressions of this stage.
	 */
	virtual Vector2 getShear() = 0;

    // Returns true if this layer has an alphatest expression defined
    virtual bool hasAlphaTest() const = 0;

    /**
     * \brief
     * Get the alpha test value for this layer.
     *
     * \return
     * The alpha test value, within (0..1] if it is set. If no alpha test
     * value is set, 0 will be returned.
     */
    virtual float getAlphaTest() const = 0;

	/**
	 * Whether this stage is active. Unconditional stages always return true,
	 * conditional ones return the result of the most recent condition expression evaluation.
	 */
	virtual bool isVisible() const = 0;

	/**
	 * Returns the name of this stage's fragment program.
	 */
	virtual const std::string& getVertexProgram() = 0;

	/**
	 * Returns the name of this stage's fragment program.
	 */
	virtual const std::string& getFragmentProgram() = 0;

	/**
	 * Returns the 4 parameter values for the vertexParm index <parm>.
	 */
	virtual Vector4 getVertexParm(int parm) = 0;

	/**
	 * Returns the number of fragment maps in this stage.
	 */
	virtual std::size_t getNumFragmentMaps() = 0;

	/**
	 * Returns the fragment map with the given index. 
	 */
	virtual TexturePtr getFragmentMap(int index) = 0;

	/**
	 * Stage-specific polygon offset, overriding the "global" one defined on the material.
	 */
	virtual float getPrivatePolygonOffset() = 0;

    // If this stage is referring to a single image file, this will return 
    // the VFS path to it with the file extension removed.
    // If this layer doesn't refer to a single image file, an empty string is returned
    virtual std::string getMapImageFilename() = 0;

    // The map expression used to generate/define the texture of this stage
    virtual shaders::IMapExpression::Ptr getMapExpression() = 0;
};

/**
 * \brief
 * Shader pointer for ShaderLayer,
 */
typedef std::shared_ptr<ShaderLayer> ShaderLayerPtr;

/**
 * \brief
 * Vector of ShaderLayer pointers.
 */
typedef std::vector<ShaderLayerPtr> ShaderLayerVector;

