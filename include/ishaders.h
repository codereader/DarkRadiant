#pragma once

#include "iimage.h"
#include "ideclmanager.h"
#include "imodule.h"
#include "ifilesystem.h"
#include <sigc++/signal.h>

#include "math/Vector3.h"
#include "math/Vector4.h"

#include <ostream>
#include <vector>

#include "Texture.h"
#include "ishaderlayer.h"
#include "ishaderexpression.h"

class Image;

/**
 * \brief Interface for a material shader.
 *
 * A material shader consists of global parameters, an editor image, and zero
 * or more shader layers (including diffusemap, bumpmap and specularmap
 * textures which are handled specially).
 *
 * Most material shaders are defined in .mtr files within the mod asset tree,
 * but there are also internal materials generated for Radiant-specific
 * internal OpenGL shaders, such as those used to render wireframes.
 */
class Material
{
public:

	enum CullType
	{
		CULL_BACK,		// default backside culling, for materials without special flags
		CULL_FRONT,		// "backSided"
		CULL_NONE,		// "twoSided"
	};

	// Global material flags
	enum Flags
	{
		FLAG_NOSHADOWS				= 1 << 0,		// noShadows
		FLAG_NOSELFSHADOW			= 1 << 1,		// noSelfShadow
		FLAG_FORCESHADOWS			= 1 << 2,		// forceShadows
		FLAG_NOOVERLAYS				= 1 << 3,		// noOverlays
		FLAG_FORCEOVERLAYS			= 1 << 4,		// forceOverlays
		FLAG_TRANSLUCENT			= 1 << 5,		// translucent
		FLAG_FORCEOPAQUE			= 1 << 6,		// forceOpaque
		FLAG_NOFOG					= 1 << 7,		// noFog
		FLAG_NOPORTALFOG			= 1 << 8,		// noPortalFog
		FLAG_UNSMOOTHEDTANGENTS		= 1 << 9,		// unsmoothedTangents
		FLAG_MIRROR					= 1 << 10,		// mirror
		FLAG_POLYGONOFFSET			= 1 << 11,		// has polygonOffset
		FLAG_ISLIGHTGEMSURF			= 1 << 12,		// is used by the TDM lightgem
		FLAG_HAS_SORT_DEFINED		= 1 << 13,		// whether a sort value has been defined
	};

    // Parser flags, used to give some hints to the material editor GUI
    // about what the material sources looked like
    enum ParseFlags
    {
        PF_HasSortDefined           = 1 << 0, // has a "sort" keyword in its definition
        PF_HasAmbientRimColour      = 1 << 1, // has an "ambientRimColor" keyword in its definition
        PF_HasSpectrum              = 1 << 2, // has a "spectrum" keyword in its definition
        PF_HasDecalInfo             = 1 << 3, // has a "decalinfo" keyword in its definition
        PF_HasDecalMacro            = 1 << 4, // has a "DECAL_MACRO" keyword in its definition
        PF_HasTwoSidedDecalMacro    = 1 << 5, // has a "TWOSIDED_DECAL_MACRO" keyword in its definition
        PF_HasParticleMacro         = 1 << 6, // has a "PARTICLE_MACRO" keyword in its definition
        PF_HasGlassMacro            = 1 << 7, // has a "GLASS_MACRO" keyword in its definition
    };

	// Surface Flags
	enum SurfaceFlags
	{
		SURF_SOLID					= 1 << 0,
		SURF_OPAQUE					= 1 << 1,
		SURF_WATER					= 1 << 2,
		SURF_PLAYERCLIP				= 1 << 3,
		SURF_MONSTERCLIP			= 1 << 4,
		SURF_MOVEABLECLIP			= 1 << 5,
		SURF_IKCLIP					= 1 << 6,
		SURF_BLOOD					= 1 << 7,
		SURF_TRIGGER				= 1 << 8,
		SURF_AASSOLID				= 1 << 9,
		SURF_AASOBSTACLE			= 1 << 10,
		SURF_FLASHLIGHT_TRIGGER		= 1 << 11,
		SURF_NONSOLID				= 1 << 12,
		SURF_NULLNORMAL				= 1 << 13,
		SURF_AREAPORTAL				= 1 << 14,
		SURF_NOCARVE				= 1 << 15,
		SURF_DISCRETE				= 1 << 16,
		SURF_NOFRAGMENT				= 1 << 17,
		SURF_SLICK					= 1 << 18,
		SURF_COLLISION				= 1 << 19,
		SURF_NOIMPACT				= 1 << 20,
		SURF_NODAMAGE				= 1 << 21,
		SURF_LADDER					= 1 << 22,
		SURF_NOSTEPS				= 1 << 23,
		SURF_GUISURF				= 1 << 24,  // has guisurf in its material def
		SURF_ENTITYGUI				= 1 << 25,  // guisurf entity
		SURF_ENTITYGUI2				= 1 << 26,  // guisurf entity2
		SURF_ENTITYGUI3				= 1 << 27,  // guisurf entity3
	};

	// Surface Type (plastic, stone, etc.)
	enum SurfaceType
	{
		SURFTYPE_DEFAULT,
		SURFTYPE_METAL,
		SURFTYPE_STONE,
		SURFTYPE_FLESH,
		SURFTYPE_WOOD,
		SURFTYPE_CARDBOARD,
		SURFTYPE_LIQUID,
		SURFTYPE_GLASS,
		SURFTYPE_PLASTIC,
		SURFTYPE_RICOCHET,
		SURFTYPE_AASOBSTACLE,
		SURFTYPE_10,
		SURFTYPE_11,
		SURFTYPE_12,
		SURFTYPE_13,
		SURFTYPE_14,
		SURFTYPE_15
	};

    /**
     * \brief
     * Requested sort position from material declaration (e.g. "sort decal").
	 * The actual sort order of a material is stored as a floating point number,
	 * these enumerations represent some regularly used shortcuts in material decls.
	 * The values of this enum have been modeled after the ones found in the D3 SDK.
     */
    enum SortRequest
    {
		SORT_SUBVIEW = -3,		// mirrors, viewscreens, etc
		SORT_GUI = -2,			// guis
		SORT_BAD = -1,
		SORT_OPAQUE,			// opaque
		SORT_PORTAL_SKY,
		SORT_DECAL,				// scorch marks, etc.
		SORT_FAR,
		SORT_MEDIUM,			// normal translucent
		SORT_CLOSE,
		SORT_ALMOST_NEAREST,	// gun smoke puffs
		SORT_NEAREST,			// screen blood blobs
        SORT_AFTER_FOG    = 90, // TDM-specific
		SORT_POST_PROCESS = 100	// after a screen copy to texture
    };

	// Deform Type
	enum DeformType
	{
		DEFORM_NONE,
		DEFORM_SPRITE,
		DEFORM_TUBE,
		DEFORM_FLARE,
		DEFORM_EXPAND,
		DEFORM_MOVE,
		DEFORM_TURBULENT,
		DEFORM_EYEBALL,
		DEFORM_PARTICLE,
		DEFORM_PARTICLE2,
	};

	struct DecalInfo
	{
		int		stayMilliSeconds;
		int		fadeMilliSeconds;
		Vector4	startColour;
		Vector4	endColour;

        DecalInfo() :
            stayMilliSeconds(0),
            fadeMilliSeconds(0),
            startColour(0,0,0,0),
            endColour(0,0,0,0)
        {}
	};

	enum Coverage
	{
		MC_UNDETERMINED,
		MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
		MC_PERFORATED,		// may have alpha tested holes
		MC_TRANSLUCENT		// blended with background
	};

    // TDM 2.11 frob stage keyword
    enum class FrobStageType
    {
        Default,     // no frobstage keyword in this material
        Diffuse,     // frobstage_diffuse has been declared: frobstage_diffuse 0.25 0.50
        Texture,     // frobstage_texture textures/some/thing 0.15 0.40
        NoFrobStage, // frobstage_none
    };

	virtual ~Material() {}

    /// Return the editor image texture for this shader.
    virtual TexturePtr getEditorImage() = 0;

    /// Return true if the editor image is no tex for this shader.
    virtual bool isEditorImageNoTex() = 0;

    // Returns the expression defining the editor image of this material, as passed to qer_editorimage statement,
    // or an empty string if this keyword was not used at all in this declaration.
    virtual shaders::IMapExpression::Ptr getEditorImageExpression() = 0;

    // Set the editor image path of this material
    virtual void setEditorImageExpressionFromString(const std::string& editorImagePath) = 0;

    /// Get the string name of this material
    virtual std::string getName() const = 0;

    virtual bool IsInUse() const = 0;
    virtual void SetInUse(bool bInUse) = 0;

    /// Return true if this is an internal material not corresponding to a .mtr
    virtual bool IsDefault() const = 0;

    /// get shader file name (ie the file where this one is defined)
    virtual const char* getShaderFileName() const = 0;

    // Set the mtr file name to define where this material should be saved to
    // This will throw an exception if the given path (absolute or relative) 
    // is not located within the current mod file tree (VFS)
    virtual void setShaderFileName(const std::string& fullPath) = 0;

    // Returns the VFS info structure of the file this shader is defined in
    virtual const vfs::FileInfo& getShaderFileInfo() const = 0;

    /**
     * \brief Return the requested sort position of this material.
     */
    virtual float getSortRequest() const = 0;

    // Set the sort value for this material, see the SortRequest enum for predefined values
    virtual void setSortRequest(float sortRequest) = 0;

    // Resets the sort request to the default value
    virtual void resetSortRequest() = 0;

    /// Return a polygon offset if one is defined. The default is 0.
    virtual float getPolygonOffset() const = 0;

    // Set the polygon offset of this material. Clear the FLAG_POLYGONOFFSET to disable the offset altogether.
    virtual void setPolygonOffset(float offset) = 0;

	/// Get the desired texture repeat behaviour.
	virtual ClampType getClampType() const = 0;

    // Set the clamp type for this material
    virtual void setClampType(ClampType type) = 0;

	/// Get the cull type (none, back, front)
	virtual CullType getCullType() const = 0;

    // Set the cull type
    virtual void setCullType(CullType type) = 0;

	/// Get the global material flags (translucent, noshadows, etc.)
	virtual int getMaterialFlags() const = 0;

    // Set the given material flag
	virtual void setMaterialFlag(Flags flag) = 0;

    // Clear the given material flags
	virtual void clearMaterialFlag(Flags flag) = 0;

	/// Surface flags (areaportal, nonsolid, etc.)
	virtual int getSurfaceFlags() const = 0;

    // Set the given surface flag
    virtual void setSurfaceFlag(Material::SurfaceFlags flag) = 0;

    // Clear the given surface flag
    virtual void clearSurfaceFlag(Material::SurfaceFlags flag) = 0;

	/// Surface Type (wood, stone, surfType15, ...)
	virtual SurfaceType getSurfaceType() const = 0;

    // Set the surface type of this material
    virtual void setSurfaceType(SurfaceType type) = 0;

	/// Get the deform type of this material
	virtual DeformType getDeformType() const = 0;

    // Returns the shader expression used to define the deform parameters (valid indices in [0..2])
    virtual shaders::IShaderExpression::Ptr getDeformExpression(std::size_t index) = 0;

    // Used for Deform_Particle/Particle2 defines the name of the particle def
    virtual std::string getDeformDeclName() = 0;

	/// Returns the spectrum of this shader, 0 is the default value (even without keyword in the material)
	virtual int getSpectrum() const = 0;

    // Sets the spectrum of this material.
    virtual void setSpectrum(int spectrum) = 0;

	/// Retrieves the decal info structure of this material.
	virtual DecalInfo getDecalInfo() const = 0;

    // Sets the decal info structure on this material.
    // If the structure is not empty, it will enable the ParseFlag PF_HasDecalInfo,
    // an empty/defaulted decalInfo structure will clear the flag
    virtual void setDecalInfo(const DecalInfo& info) = 0;

	/// Returns the coverage type of this material, also needed by the map compiler.
	virtual Coverage getCoverage() const = 0;

	/**
	 * Returns the raw shader definition block, as parsed by the material manager.
	 * The definition is lacking the outermost curly braces.
	 */
	virtual std::string getDefinition() = 0;

	/** Determine whether this is an ambient light shader, i.e. the
	 * material def contains the global "ambientLight" keyword.
	 */
	virtual bool isAmbientLight() const = 0;

	/** Determine whether this is an blend light shader, i.e. the
	 * material def contains the global "blendLight" keyword.
	 */
	virtual bool isBlendLight() const = 0;

	/** Determine whether this is an fog light shader, i.e. the
	 * material def contains the global "fogLight" keyword.
	 */
	virtual bool isFogLight() const = 0;

    /** Determine whether this is a cubic light shader, i.e. the
     * material def contains the global "cubicLight" keyword.
     */
    virtual bool isCubicLight() const = 0;

    virtual void setIsAmbientLight(bool newValue) = 0;
    virtual void setIsBlendLight(bool newValue) = 0;
    virtual void setIsFogLight(bool newValue) = 0;
    virtual void setIsCubicLight(bool newValue) = 0;

	/**
	 * For light shaders: implicitly no-shadows lights (ambients, fogs, etc)
	 * will never cast shadows but individual light entities can also override this value.
	 */
	virtual bool lightCastsShadows() const = 0;

	// returns true if the material will generate shadows, not making a
	// distinction between global and no-self shadows
	virtual bool surfaceCastsShadow() const = 0;

	/**
	 * returns true if the material will draw anything at all.  Triggers, portals,
	 * etc, will not have anything to draw.  A not drawn surface can still castShadow,
	 * which can be used to make a simplified shadow hull for a complex object set as noShadow.
	 */
	virtual bool isDrawn() const = 0;

	/**
	 * a discrete surface will never be merged with other surfaces by dmap, which is
	 * necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
	 * special effects from being combined into a single surface
	 * guis, merging sprites or other effects, mirrors and remote views are always discrete
	 */
	virtual bool isDiscrete() const = 0;

    // Returns the number of layers in this material
    virtual std::size_t getNumLayers() = 0;

    // Returns the n-th layer of this material (0-based index)
    virtual IShaderLayer::Ptr getLayer(std::size_t index) = 0;

    /// Return the first material layer, if any
	virtual IShaderLayer* firstLayer() = 0;

    /**
     * \brief Visit all layers in this material using the given functor.
     * The functor can return false to abort the traversal, true will continue.
     * This includes all diffuse, bump, specular or blend layers.
     */
    virtual void foreachLayer(const std::function<bool(const IShaderLayer::Ptr&)>& functor) = 0;

    // Add a new (typed) layer to this material, returning the index of the new layer
    virtual std::size_t addLayer(IShaderLayer::Type type) = 0;

    // Removes the indexed layer from this material
    virtual void removeLayer(std::size_t index) = 0;

    // Duplicates the given layer and returns the index to the copied one
    virtual std::size_t duplicateLayer(std::size_t index) = 0;

    // Swaps the position of the two layers
    virtual void swapLayerPosition(std::size_t first, std::size_t second) = 0;

    // Returns the edit interface for the given shader layer. Calling this method
    // will immediately mark this Material as modified.
    virtual IEditableShaderLayer::Ptr getEditableLayer(std::size_t index) = 0;

    /// Return the light falloff texture, if this is a light shader
    virtual TexturePtr lightFalloffImage() = 0;

    // Return the expression of the light falloff map for use with this shader.
    virtual shaders::IMapExpression::Ptr getLightFalloffExpression() = 0;

    // Set the lightFallOff expression to define the image/cubemap to use
    virtual void setLightFalloffExpressionFromString(const std::string& expressionString) = 0;

    // Return the type of the light fall off image 
    // (can be MapType::Map (lightFalloffImage or MapType::CameraCubeMap for lightFalloffCubeMap)
    virtual IShaderLayer::MapType getLightFalloffCubeMapType() = 0;

    // Set the type of the light fall off image 
    // (can be MapType::Map (lightFalloffImage or MapType::CameraCubeMap for lightFalloffCubeMap)
    virtual void setLightFalloffCubeMapType(IShaderLayer::MapType type) = 0;

	// greebo: Returns the description as defined in the material
	virtual std::string getDescription() const = 0;

    // Set the description text of this material
	virtual void setDescription(const std::string& description) = 0;

    // Returns the frob stage type this material is using (defaults to FrobStageType::Default)
    virtual FrobStageType getFrobStageType() = 0;

    // Sets the frob stage type. Might assign a _white frob stage map expression (if empty).
    virtual void setFrobStageType(FrobStageType type) = 0;

    // When FrobStageType::Texture: defines the texture that has been declared using frobstage_texture
    virtual shaders::IMapExpression::Ptr getFrobStageMapExpression() = 0;

    // Sets the frob stage map expression (applicable to FrobStageType::Texture)
    virtual void setFrobStageMapExpressionFromString(const std::string& expr) = 0;

    // frobstage_diffuse and frobstage_texture accept two (r g b) or float expressions
    // Index is [0..1]. The first parameter is additive, second is multiplicative
    virtual Vector3 getFrobStageRgbParameter(std::size_t index) = 0;

    // Assigns the RGB components to the frob stage parameter with the given index => "(x y z)"
    virtual void setFrobStageRgbParameter(std::size_t index, const Vector3& value) = 0;

    // Assigns a single uniform value to the frob stage parameter with the given index => "x"
    virtual void setFrobStageParameter(std::size_t index, double value) = 0;

	 /// Return TRUE if the shader is visible, FALSE if it is filtered or
	 /// disabled in any other way.
	virtual bool isVisible() const = 0;

	/// Sets the visibility of this shader.
	virtual void setVisible(bool visible) = 0;

    // Returns the flags set by the material parser
    virtual int getParseFlags() const = 0;

    // Returns the argument string after the renderbump keyword, or an empty string if no statement is present
    virtual std::string getRenderBumpArguments() = 0;

    // Returns the argument string after the renderbumpflat keyword, or an empty string if no statement is present
    virtual std::string getRenderBumpFlatArguments() = 0;

    // The argument to the "guisurf" keyword, if not entity[2]3]. 
    // In case entity[2]3] is set, the corresponding surface flags are enabled
    virtual const std::string& getGuiSurfArgument() = 0;

    // True if this mateiral has been altered from its original definition
    virtual bool isModified() = 0;

    // Roll back the changes made to this material
    virtual void revertModifications() = 0;

    // Reloads the textures used by this material from disk
    virtual void refreshImageMaps() = 0;

    struct ParseResult
    {
        bool success;           // whether the update was successful
        std::string parseError; // if success == false, this contains the error message
    };

    // Attempts to redefine this material from the given source text, which is
    // just the block contents, exlcuding the outermost curly braces of the decl.
    // Returns true on success, returns false if the source text could not
    // be successfully parsed (e.g. due to malformed syntax).
    virtual ParseResult updateFromSourceText(const std::string& sourceText) = 0;

    // Signal emitted when this material is modified
    virtual sigc::signal<void>& sig_materialChanged() = 0;
};

typedef std::shared_ptr<Material> MaterialPtr;

/// Stream insertion of Material for debugging purposes.
inline
std::ostream& operator<< (std::ostream& os, const Material& shader)
{
	os << "Material(name=" << shader.getName()
	   << ", filename=" << shader.getShaderFileName()
	   << ")";
	return os;
}

/// Debug stream insertion of possibly null material pointer
inline std::ostream& operator<< (std::ostream& os, const Material* m)
{
    if (m)
        return os << *m;
    else
        return os << "[no material]";
}

typedef std::function<void(const std::string&)> ShaderNameCallback;

// Represents a table declaration in the .mtr files
class ITableDefinition :
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<ITableDefinition>;

    virtual ~ITableDefinition() {}

    // Retrieve a value from this table, respecting the clamp and snap flags
    virtual float getValue(float index) = 0;
};

constexpr const char* const MODULE_SHADERSYSTEM = "MaterialManager";

/**
 * \brief
 * Interface for the material manager.
 *
 * The material manager parses all of the MTR declarations and provides access
 * to Material objects representing the loaded materials.
 */
class IMaterialManager
: public RegisterableModule
{
public:
  // NOTE: shader and texture names used must be full path.
  // Shaders usable as textures have prefix equal to getTexturePrefix()

	/**
     * \brief Return the shader with the given name. The default shader will be
     * returned if name is not found.
	 *
	 * \param name
	 * The text name of the shader to load.
	 *
	 * \returns
	 * MaterialPtr corresponding to the named shader object.
	 */
	virtual MaterialPtr getMaterial(const std::string& name) = 0;

	/**
	 * greebo: Returns true if the named material is existing, false otherwise.
	 * In the latter case getMaterialForName() would return a default "shader not found".
	 */
	virtual bool materialExists(const std::string& name) = 0;

    /**
     * A material can be modified if it has been declared in a physical file,
     * i.e. outside a PAK file.
     */
    virtual bool materialCanBeModified(const std::string& name) = 0;

	virtual void foreachShaderName(const ShaderNameCallback& callback) = 0;

    /**
     * Visit each material with the given function object. Replaces the legacy foreachShader().
     */
    virtual void foreachMaterial(const std::function<void(const MaterialPtr&)>& func) = 0;

    // Set the callback to be invoked when the active shaders list has changed
	virtual sigc::signal<void> signal_activeShadersChanged() const = 0;

    // Is invoked when a new material is inserted into the resource tree, passing the name as argument
    virtual sigc::signal<void, const std::string&>& signal_materialCreated() = 0;

    // Is called when a material name is changed, passing the old and the new name as arguments
    virtual sigc::signal<void, const std::string&, const std::string&>& signal_materialRenamed() = 0;

    // Is emitted when a named material is removed from the library
    virtual sigc::signal<void, const std::string&>& signal_materialRemoved() = 0;

    /**
     * Enable or disable active shaders updates (for performance).
     */
    virtual void setActiveShaderUpdates(bool val) = 0;

  virtual const char* getTexturePrefix() const = 0;

    /**
     * \brief
     * Return the default texture to be used for lighting mode rendering if it
     * is not defined for a shader.
     *
     * \param type
     * The type of interaction layer whose default texture is required.
     */
    virtual TexturePtr getDefaultInteractionTexture(IShaderLayer::Type type) = 0;

	/**
	 * greebo: This is a substitution for the "old" TexturesCache method
	 * used to load an image from a file to graphics memory for arbitrary
	 * use (e.g. the Overlay module).
	 *
	 * @param filename
	 * The absolute filename.
	 *
	 * @param moduleNames
	 * The space-separated list of image modules (default is "GDK").
	 */
	virtual TexturePtr loadTextureFromFile(const std::string& filename) = 0;

	/**
	 * Creates a new shader expression for the given string. This can be used to create standalone
	 * expression objects for unit testing purposes.
	 */
	virtual shaders::IShaderExpression::Ptr createShaderExpressionFromString(const std::string& exprStr) = 0;

    // Creates a new material using the given name. In case the name is already in use,
    // a generated one will be assigned to the created material
    virtual MaterialPtr createEmptyMaterial(const std::string& name) = 0;

    // Creates a copy of the given material and returns the reference to it
    virtual MaterialPtr copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy) = 0;

    // Renames the material named oldName to newName, and returns true if the operation was successful. 
    // If the new name is already in use, this returns false too.
    virtual bool renameMaterial(const std::string& oldName, const std::string& newName) = 0;

    // Removes the named material
    virtual void removeMaterial(const std::string& name) = 0;

    // Saves the named material to the file location as specified in its shaderfile info.
    // If the path is not writable or the material is not suitable for saving, this will throw an exception
    virtual void saveMaterial(const std::string& name) = 0;

    // Tries to find the named table, returns an empty reference if nothing found
    virtual ITableDefinition::Ptr getTable(const std::string& name) = 0;

    // Reload the textures used by the active shaders
    virtual void reloadImages() = 0;
};

inline IMaterialManager& GlobalMaterialManager()
{
	static module::InstanceReference<IMaterialManager> _reference(MODULE_SHADERSYSTEM);
	return _reference;
}
