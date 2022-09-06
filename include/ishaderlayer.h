#pragma once

#include "Texture.h"
#include "ishaderexpression.h"

#include <memory>
#include <vector>
#include <string>

#include "math/Vector2.h"
#include "math/Vector4.h"
#include "math/Matrix4.h"
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
 * \brief Representation of a GL blend function.
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
 * \brief A single layer of a material shader.
 *
 * Each shader layer contains an image texture, a blend mode (e.g. add,
 * modulate) and various other data.
 */
class IShaderLayer
{
public:
    using Ptr = std::shared_ptr<IShaderLayer>;

    /// Enumeration of layer types
    /// The enum value is also used to sort interaction stages: bump before diffuse before specular
    enum Type
    {
        BUMP,
        DIFFUSE,
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
		FLAG_IGNORE_DEPTH			= 1 << 11,  // use depthfunc always
	};

	enum TexGenType
	{
		TEXGEN_NORMAL		= 1 << 0,
		TEXGEN_REFLECT		= 1 << 1,
		TEXGEN_SKYBOX		= 1 << 2,
		TEXGEN_WOBBLESKY	= 1 << 3,
		TEXGEN_SCREEN	    = 1 << 4, // screen aligned, for mirrorRenders and screen space temporaries
	};

    enum ParseFlags
    {
        PF_HasTexGenKeyword =      1 << 1, // texgen has been specified
        PF_HasNoclampKeyword =     1 << 2, // noclamp has been specified
        PF_HasColoredKeyword =     1 << 3, // colored has been specified
    };

    // Expression slot selector
    struct Expression
    {
        enum Slot
        {
            AlphaTest = 0,
            Condition,
            TexGenParam1,
            TexGenParam2,
            TexGenParam3,
            ColourRed,
            ColourGreen,
            ColourBlue,
            ColourAlpha,
            TextureMatrixRow0Col0,
            TextureMatrixRow0Col1,
            TextureMatrixRow0Col2,
            TextureMatrixRow1Col0,
            TextureMatrixRow1Col1,
            TextureMatrixRow1Col2,
            NumExpressionSlots
        };
    };

    // The various texture transformations
    enum class TransformType
    {
        Translate,
        Scale,
        CenterScale,
        Shear,
        Rotate,
    };

    // A texture transformation consists of a type and two arguments at most
    struct Transformation
    {
        TransformType type;
        shaders::IShaderExpression::Ptr expression1;
        shaders::IShaderExpression::Ptr expression2;
    };

    /**
     * \brief
	 * Destructor
	 */
	virtual ~IShaderLayer() {}

    /// Return the layer type.
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

    // Returns the requested expression
    virtual shaders::IShaderExpression::Ptr getExpression(Expression::Slot slot) = 0;

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

    // The expressions used to calculate the tex gen params. Index in [0..2]
    virtual shaders::IShaderExpression::Ptr getTexGenExpression(std::size_t index) const = 0;

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

    // An enum used to select which colour components are affected by an operation
    enum ColourComponentSelector
    {
        COMP_RED,   // red only
        COMP_GREEN, // green only
        COMP_BLUE,  // blue only
        COMP_ALPHA, // alpha only
        COMP_RGB,   // red, green and blue
        COMP_RGBA,  // all: red, greeb, blue, alpha
    };

    // Returns the expression to calculate the RGBA vertex colour values
    virtual const shaders::IShaderExpression::Ptr& getColourExpression(ColourComponentSelector component) const = 0;

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
    virtual const Vector2& getRenderMapSize() const = 0;

    // Adds a transformation at the end of the list of existing transforms in this stage
    virtual void appendTransformation(const Transformation& transform) = 0;

    // The list of transformations defined in this stage, in the order they appear in the declaration
    virtual const std::vector<Transformation>& getTransformations() = 0;

    // Returns the current texture matrix
    virtual Matrix4 getTextureTransform() = 0;

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

    // Returns the expression used to calculate the alpha test value
    virtual const shaders::IShaderExpression::Ptr& getAlphaTestExpression() const = 0;

	/**
	 * Whether this stage is active. Unconditional stages always return true,
	 * conditional ones return the result of the most recent condition expression evaluation.
	 */
	virtual bool isVisible() const = 0;

    // Stages can be enabled/disabled programmatically (used when editing materials)
    // This returns true if the stage is enabled
    virtual bool isEnabled() const = 0;

    // Returns the if-expression used to evaluate this stage's visibility, or null if none defined
    virtual const shaders::IShaderExpression::Ptr& getConditionExpression() const = 0;

	/**
	 * Returns the name of this stage's vertex program.
	 */
	virtual const std::string& getVertexProgram()const = 0;

	/**
	 * Returns the name of this stage's fragment program.
	 */
	virtual const std::string& getFragmentProgram() const = 0;

    // The number of defined vertex parameters
    virtual int getNumVertexParms() const = 0;

	/**
	 * Returns the 4 parameter values for the vertexParm index <parm>.
	 */
	virtual Vector4 getVertexParmValue(int parm) const = 0;

    // A vertex parm has an index and 4 expressions at most
    struct VertexParm
    {
        VertexParm() :
            index(-1) // a negative index indicates this parm has not been defined in the stage
        {}

        int index;
        shaders::IShaderExpression::Ptr expressions[4];
    };

    // Returns the vertex parameter with the given index [0..3]
    virtual const VertexParm& getVertexParm(int index) const = 0;

	/**
	 * Returns the number of fragment maps in this stage.
	 */
	virtual std::size_t getNumFragmentMaps() const = 0;

    struct FragmentMap
    {
        FragmentMap() :
            index(-1)
        {}

        int index;
        std::vector<std::string> options;
        shaders::IMapExpression::Ptr map;
    };

    virtual const FragmentMap& getFragmentMap(int index) const = 0;

	/**
	 * Returns the fragment map image with the given index.
	 */
	virtual TexturePtr getFragmentMapTexture(int index) const = 0;

	/**
	 * Stage-specific polygon offset, overriding the "global" one defined on the material.
	 */
	virtual float getPrivatePolygonOffset() const = 0;

    // If this stage is referring to a single image file, this will return
    // the VFS path to it with the file extension removed.
    // If this layer doesn't refer to a single image file, an empty string is returned
    virtual std::string getMapImageFilename() const = 0;

    // The map expression used to generate/define the texture of this stage
    virtual shaders::IMapExpression::Ptr getMapExpression() const = 0;

    // Parser information, to reconstruct the use of certain keywords
    virtual int getParseFlags() const = 0;
};

/**
 * \brief
 * Vector of IShaderLayer pointers.
 */
typedef std::vector<IShaderLayer::Ptr> IShaderLayerVector;

// Interface extension to IShaderLayer, offering editing functions
class IEditableShaderLayer :
    public IShaderLayer
{
public:
    using Ptr = std::shared_ptr<IEditableShaderLayer>;

    virtual ~IEditableShaderLayer() {}

    // Enable/disable this stage - a disabled stage will return false to isVisible()
    virtual void setEnabled(bool enabled) = 0;

    // Set the given stage flag(s)
    virtual void setStageFlag(IShaderLayer::Flags flag) = 0;

    // Clears the given stage flag(s)
    virtual void clearStageFlag(IShaderLayer::Flags flag) = 0;

    // Set the map type to use by this stage (map, cameraCubeMap, etc.)
    virtual void setMapType(MapType mapType) = 0;

    // Set the blend func from the given pair of strings
    virtual void setBlendFuncStrings(const std::pair<std::string, std::string>& pair) = 0;

    // Set the alpha test expression from the given string
    virtual void setAlphaTestExpressionFromString(const std::string& expression) = 0;

    // Update the "map" expression of this stage
    virtual void setMapExpressionFromString(const std::string& expression) = 0;

    // Adds a empty transformation to this layer, returning its new index
    virtual std::size_t addTransformation(TransformType type, const std::string& expression1, const std::string& expression2) = 0;

    // Removes the indexed transformation
    virtual void removeTransformation(std::size_t index) = 0;

    // Set the type and expressions of the indexed transformation. The transformation index must not be
    // out of bounds, use appendTransformation() to add new ones
    virtual void updateTransformation(std::size_t index, TransformType type,
        const std::string& expression1, const std::string& expression2) = 0;

    // Set the colour expression from the given string
    virtual void setColourExpressionFromString(ColourComponentSelector component, const std::string& expression) = 0;

    // Set the stage condition expression
    virtual void setConditionExpressionFromString(const std::string& expression) = 0;
    
    // Sets the texgen type of this stage
    virtual void setTexGenType(TexGenType type) = 0;

    // Set the texgen expression (used for wobblesky)
    virtual void setTexGenExpressionFromString(std::size_t index, const std::string& expression) = 0;

    // Set the vertex colour mode of this stage
    virtual void setVertexColourMode(VertexColourMode mode) = 0;

    // Set the clamp type of this stage
    virtual void setClampType(ClampType clampType) = 0;

    // Sets the privatePolygonOffset value of this stage
    virtual void setPrivatePolygonOffset(double offset) = 0;

    // Sets width and height of the mirrorRenderMap/remoteRenderMap images
    virtual void setRenderMapSize(const Vector2& size) = 0;

    // For stages with map type SoundMap, this is used to set the waveform flag
    virtual void setSoundMapWaveForm(bool waveForm) = 0;

    // For stages with map type VideoMap, this is used to update the properties
    virtual void setVideoMapProperties(const std::string& filePath, bool looping) = 0;
};
