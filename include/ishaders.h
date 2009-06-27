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

#if !defined(INCLUDED_ISHADERS_H)
#define INCLUDED_ISHADERS_H

#include "generic/callbackfwd.h"
#include "iimage.h"
#include "imodule.h"

#include "math/Vector3.h"

#include <ostream>
#include <vector>

#include "Texture.h"
#include "ShaderLayer.h"

class Image;

// Forward declaration
namespace shaders {
	
class MapExpression;
typedef boost::shared_ptr<MapExpression> MapExpressionPtr;

} // namespace shaders

enum
{
  QER_TRANS = 1 << 0,
  QER_NOCARVE = 1 << 1,
  QER_NODRAW = 1 << 2,
  QER_NONSOLID = 1 << 3,
  QER_WATER = 1 << 4,
  QER_LAVA = 1 << 5,
  QER_FOG = 1 << 6,
  QER_CULL = 1 << 8,
  QER_AREAPORTAL = 1 << 9,
  QER_CLIP = 1 << 10,
  QER_BOTCLIP = 1 << 11,
};

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
typedef Vector3 Colour3;

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

  enum ECull
  {
    eCullNone,
    eCullBack,
  };

    /**
     * \brief
     * Return the editor image texture for this shader.
     */
    virtual TexturePtr getEditorImage() = 0;

    /**
     * \brief
     * Get the string name of this shader.
     */
    virtual std::string getName() const = 0;

  virtual bool IsInUse() const = 0;
  virtual void SetInUse(bool bInUse) = 0;
  // get the editor flags (QER_NOCARVE QER_TRANS)
  virtual int getFlags() const = 0;
  // get the transparency value
  virtual float getTrans() const = 0;
  // test if it's a true shader, or a default shader created to wrap around a texture
  virtual bool IsDefault() const = 0;
  // get the cull type
  virtual ECull getCull() = 0;
  // get shader file name (ie the file where this one is defined)
  virtual const char* getShaderFileName() const = 0;

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

  virtual const ShaderLayer* firstLayer() const = 0;

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

namespace shaders {

/**
 * greebo: Abstract visitor class for traversing shader lists.
 */
class ShaderVisitor
{
public:
	virtual void visit(const MaterialPtr& shader) = 0;
};

} // namespace shaders

typedef struct _GSList GSList;
typedef Callback1<const char*> ShaderNameCallback;

class ModuleObserver;

const std::string MODULE_SHADERSYSTEM("MaterialManager");

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

	virtual void foreachShaderName(const ShaderNameCallback& callback) = 0;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
	virtual void foreachShader(shaders::ShaderVisitor& visitor) = 0;

  // iterate over the list of active shaders (deprecated functions)
  virtual void beginActiveShadersIterator() = 0;
  virtual bool endActiveShadersIterator() = 0;
  virtual MaterialPtr dereferenceActiveShadersIterator() = 0;
  virtual void incrementActiveShadersIterator() = 0;

    // Set the callback to be invoked when the active shaders list has changed
    virtual void setActiveShadersChangedNotify(const Callback& notify) = 0;

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
};

inline MaterialManager& GlobalMaterialManager() {
	// Cache the reference locally
	static MaterialManager& _shaderSystem(
		*boost::static_pointer_cast<MaterialManager>(
			module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM)
		)
	);
	return _shaderSystem;
}

#define QERApp_ActiveShaders_IteratorBegin GlobalMaterialManager().beginActiveShadersIterator
#define QERApp_ActiveShaders_IteratorAtEnd GlobalMaterialManager().endActiveShadersIterator
#define QERApp_ActiveShaders_IteratorCurrent GlobalMaterialManager().dereferenceActiveShadersIterator
#define QERApp_ActiveShaders_IteratorIncrement GlobalMaterialManager().incrementActiveShadersIterator

#endif
