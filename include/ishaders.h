/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include "iimage.h"
#include "imodule.h"

#include "math/Vector3.h"
#include "math/Vector4.h"

#include <ostream>
#include <vector>

#include "Texture.h"
#include "ShaderLayer.h"
#include "ishaderexpression.h"

class Image;

// Forward declaration
namespace shaders {

class MapExpression;
typedef boost::shared_ptr<MapExpression> MapExpressionPtr;

} // namespace shaders

/**
 * \brief
 * Interface for a material shader.
 *
 * A material shader consists of global parameters, an editor image, and zero or
 * more shader layers (including diffusemap, bumpmap and specularmap textures
 * which are handled specially).
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
		SURF_ENTITYGUI				= 1 << 24,
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
	};

	enum Coverage
	{
		MC_UNDETERMINED,
		MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
		MC_PERFORATED,		// may have alpha tested holes
		MC_TRANSLUCENT		// blended with background
	};

	virtual ~Material() {}

    /**
     * \brief
     * Return the editor image texture for this shader.
     */
    virtual TexturePtr getEditorImage() = 0;

    /**
     * \brief
     * Return true if the editor image is no tex for this shader.
     */
    virtual bool isEditorImageNoTex() = 0;

    /**
     * \brief
     * Get the string name of this shader.
     */
    virtual std::string getName() const = 0;

  virtual bool IsInUse() const = 0;
  virtual void SetInUse(bool bInUse) = 0;
  
  // test if it's a true shader, or a default shader created to wrap around a texture
  virtual bool IsDefault() const = 0;
  
  // get shader file name (ie the file where this one is defined)
  virtual const char* getShaderFileName() const = 0;

    /**
     * \brief
     * Return the requested sort position of this material.
	 * greebo: D3 is using floating points for the sort value but
	 * as far as I can see only rounded numbers have been used.
     */
    virtual int getSortRequest() const = 0;

    /**
     * \brief
     * Return a polygon offset if one is defined. The default is 0.
     */
    virtual float getPolygonOffset() const = 0;

	/**
	 * Get the desired texture repeat behaviour.
	 */
	virtual ClampType getClampType() const = 0;

	// Get the cull type (none, back, front)
	virtual CullType getCullType() const = 0;

	/**
	 * Get the global material flags (translucent, noshadows, etc.)
	 */
	virtual int getMaterialFlags() const = 0;

	/**
	 * Surface flags (areaportal, nonsolid, etc.)
	 */
	virtual int getSurfaceFlags() const = 0;

	/**
	 * Surface Type (wood, stone, surfType15, ...)
	 */
	virtual SurfaceType getSurfaceType() const = 0;

	/**
	 * Get the deform type of this material
	 */
	virtual DeformType getDeformType() const = 0;

	/**
	 * Returns the spectrum of this shader, -1 means "no defined spectrum"
	 */
	virtual int getSpectrum() const = 0;

	/**
	 * Retrieves the decal info structure of this material.
	 */
	virtual const DecalInfo& getDecalInfo() const = 0;

	/**
	 * Returns the coverage type of this material, also needed
	 * by the map compiler.
	 */
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

	virtual ShaderLayer* firstLayer() const = 0;

    /**
     * \brief
     * Return a std::vector containing all layers in this material shader.
     *
     * This includes all diffuse, bump, specular or blend layers.
     */
    virtual const ShaderLayerVector& getAllLayers() const = 0;

  virtual TexturePtr lightFalloffImage() = 0;

	// greebo: Returns the description as defined in the material
	virtual std::string getDescription() const = 0;

	/**
	 * greebo: Returns TRUE if the shader is visible, FALSE if it
	 *         is filtered or disabled in any other way.
	 */
	virtual bool isVisible() const = 0;

	/**
	 * greebo: Sets the visibility of this shader.
	 */
	virtual void setVisible(bool visible) = 0;
};

typedef boost::shared_ptr<Material> MaterialPtr;

/**
 * Stream insertion of Material for debugging purposes.
 */
inline
std::ostream& operator<< (std::ostream& os, const Material& shader) {
	os << "Material { name = " << shader.getName()
	   << ", filename = " << shader.getShaderFileName()
	   << ", firstlayer = " << shader.firstLayer()
	   << " }";
	return os;
}

namespace shaders
{

/**
 * greebo: Abstract visitor class for traversing shader lists.
 */
class ShaderVisitor
{
public:
    virtual ~ShaderVisitor() {}
	virtual void visit(const MaterialPtr& shader) = 0;
};

} // namespace shaders

typedef boost::function<void(const std::string&)> ShaderNameCallback;

class ModuleObserver;

const char* const MODULE_SHADERSYSTEM = "MaterialManager";

/**
 * \brief
 * Interface for the material manager.
 *
 * The material manager parses all of the MTR declarations and provides access
 * to Material objects representing the loaded materials.
 */
class MaterialManager
: public RegisterableModule
{
public:
  // NOTE: shader and texture names used must be full path.
  // Shaders usable as textures have prefix equal to getTexturePrefix()

  virtual void realise() = 0;
  virtual void unrealise() = 0;
  virtual void refresh() = 0;

	/** Determine whether the shader system is realised. This may be used
	 * by components which need to ensure the shaders are realised before
	 * they start trying to display them.
	 *
	 * @returns
	 * true if the shader system is realised, false otherwise
	 */
	virtual bool isRealised() = 0;


	/** Activate the shader for a given name and return it. The default shader
	 * will be returned if name is not found.
	 *
	 * @param name
	 * The text name of the shader to load.
	 *
	 * @returns
	 * MaterialPtr shared ptr corresponding to the named shader object.
	 */
	virtual MaterialPtr getMaterialForName(const std::string& name) = 0;

	/**
	 * greebo: Returns true if the named material is existing, false otherwise.
	 * In the latter case getMaterialForName() would return a default "shader not found".
	 */
	virtual bool materialExists(const std::string& name) = 0;

	virtual void foreachShaderName(const ShaderNameCallback& callback) = 0;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
	virtual void foreachShader(shaders::ShaderVisitor& visitor) = 0;

	// The observer gets notified when the list of active shaders changes
	class ActiveShadersObserver
	{
	public:
		virtual ~ActiveShadersObserver() {}

		virtual void onActiveShadersChanged() = 0;
	};
	typedef boost::shared_ptr<ActiveShadersObserver> ActiveShadersObserverPtr;

    // Set the callback to be invoked when the active shaders list has changed
	virtual void addActiveShadersObserver(const ActiveShadersObserverPtr& observer) = 0;
	virtual void removeActiveShadersObserver(const ActiveShadersObserverPtr& observer) = 0;

    /**
     * Enable or disable active shaders updates (for performance).
     */
    virtual void setActiveShaderUpdates(bool val) = 0;

  virtual void attach(ModuleObserver& observer) = 0;
  virtual void detach(ModuleObserver& observer) = 0;

  virtual void setLightingEnabled(bool enabled) = 0;

  virtual const char* getTexturePrefix() const = 0;

    /**
     * \brief
     * Return the default texture to be used for lighting mode rendering if it
     * is not defined for a shader.
     *
     * \param type
     * The type of interaction layer whose default texture is required.
     */
    virtual TexturePtr getDefaultInteractionTexture(ShaderLayer::Type type) = 0;

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
	virtual TexturePtr loadTextureFromFile(
			const std::string& filename,
			const std::string& moduleNames = "GDK") = 0;

	/**
	 * Creates a new shader expression for the given string. This can be used to create standalone
	 * expression objects for unit testing purposes.
	 */ 
	virtual shaders::IShaderExpressionPtr createShaderExpressionFromString(const std::string& exprStr) = 0;
};

inline MaterialManager& GlobalMaterialManager()
{
	// Cache the reference locally
	static MaterialManager& _shaderSystem(
		*boost::static_pointer_cast<MaterialManager>(
			module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM)
		)
	);
	return _shaderSystem;
}
