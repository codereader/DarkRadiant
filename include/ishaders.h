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

#include <ostream>

class Texture; 	// defined in texturelib.h
typedef boost::shared_ptr<Texture> TexturePtr;

class Image;

/* greebo: A TextureConstructor creates an actual bitmap image
 * that can be used to perform an OpenGL bind.
 * 
 * The image can either be loaded from a disk file (simple)
 * or the result of a MapExpression hierarchy (addnormals and such)
 */
class TextureConstructor
{
public:
	virtual ImagePtr construct() = 0;
};
typedef boost::shared_ptr<TextureConstructor> TextureConstructorPtr;

// Forward declaration
namespace shaders {
	
class IMapExpression;
typedef boost::shared_ptr<IMapExpression> MapExpressionPtr;

/* greebo: The TextureManager keeps track of all the Textures that are
 * bound in OpenGL. It is responsible for loading/unloading the textures
 * on demand and/or retrieving the pointers to these textures.
 */
class IGLTextureManager
{
public:
	
	/**
	 * Retrieves the pointer to the Texture object named by the textureKey 
	 * string. If the texture is already bound in OpenGL the pointer to the 
	 * existing Texture is returned.
	 * 
	 * @param textureKey
	 * Name of the texture to look up.
	 * 
	 * @param constructor
	 * TextureConstructor object which will be used to populate and bind this
	 * texture if it is not found in the cache. 
	 */
	virtual TexturePtr getBinding(MapExpressionPtr mapExp) = 0;
	
	/** greebo: This loads a texture directly from the disk using the
	 * 			specified <fullPath>.
	 * 
	 * @fullPath: The absolute path to the file (no VFS paths).
	 * @moduleNames: The module names used to invoke the correct imageloader.
	 * 				 This usually defaults to "BMP".
	 */
	virtual TexturePtr getBinding(const std::string& fullPath,
								  const std::string& moduleNames) = 0;
};

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
  QER_ALPHATEST = 1 << 7,
  QER_CULL = 1 << 8,
  QER_AREAPORTAL = 1 << 9,
  QER_CLIP = 1 << 10,
  QER_BOTCLIP = 1 << 11,
};

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
typedef Vector3 Colour3;

enum BlendFactor {
	BLEND_ZERO,
	BLEND_ONE,
	BLEND_SRC_COLOUR,
	BLEND_ONE_MINUS_SRC_COLOUR,
	BLEND_SRC_ALPHA,
	BLEND_ONE_MINUS_SRC_ALPHA,
	BLEND_DST_COLOUR,
	BLEND_ONE_MINUS_DST_COLOUR,
	BLEND_DST_ALPHA,
	BLEND_ONE_MINUS_DST_ALPHA,
	BLEND_SRC_ALPHA_SATURATE
};

class BlendFunc
{
public:
  BlendFunc(BlendFactor src, BlendFactor dst) : m_src(src), m_dst(dst)
  {
  }
  BlendFactor m_src;
  BlendFactor m_dst;
};

class ShaderLayer
{
public:
  virtual TexturePtr texture() const = 0;
  virtual BlendFunc blendFunc() const = 0;
  virtual bool clampToBorder() const = 0;
  virtual float alphaTest() const = 0;
};

typedef Callback1<const ShaderLayer&> ShaderLayerCallback;

class IShader
{
public:
  enum EAlphaFunc
  {
    eAlways,
    eEqual,
    eLess,
    eGreater,
    eLEqual,
    eGEqual,
  };
  enum ECull
  {
    eCullNone,
    eCullBack,
  };

  // get/set the qtexture_t* Radiant uses to represent this shader object
  virtual TexturePtr getTexture() = 0;
  virtual TexturePtr getDiffuse() = 0;
  virtual TexturePtr getBump() = 0;
  virtual TexturePtr getSpecular() = 0;
  // get shader name
  virtual const char* getName() const = 0;
  virtual bool IsInUse() const = 0;
  virtual void SetInUse(bool bInUse) = 0;
  // get the editor flags (QER_NOCARVE QER_TRANS)
  virtual int getFlags() const = 0;
  // get the transparency value
  virtual float getTrans() const = 0;
  // test if it's a true shader, or a default shader created to wrap around a texture
  virtual bool IsDefault() const = 0;
  // get the alphaFunc
  virtual void getAlphaFunc(EAlphaFunc *func, float *ref) = 0;
  virtual BlendFunc getBlendFunc() const = 0;
  // get the cull type
  virtual ECull getCull() = 0;
  // get shader file name (ie the file where this one is defined)
  virtual const char* getShaderFileName() const = 0;

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
  virtual void forEachLayer(const ShaderLayerCallback& layer) const = 0;

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

typedef boost::shared_ptr<IShader> IShaderPtr;

/**
 * Stream insertion of IShader for debugging purposes.
 */
inline
std::ostream& operator<< (std::ostream& os, const IShader& shader) {
	os << "IShader { name = " << shader.getName()
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
	virtual void visit(const IShaderPtr& shader) = 0;
};

} // namespace shaders

typedef struct _GSList GSList;
typedef Callback1<const char*> ShaderNameCallback;

class ModuleObserver;

const std::string MODULE_SHADERSYSTEM("ShaderSystem");

class ShaderSystem :
	public RegisterableModule
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
	 * IShaderPtr shared ptr corresponding to the named shader object.
	 */
	virtual IShaderPtr getShaderForName(const std::string& name) = 0;

	virtual void foreachShaderName(const ShaderNameCallback& callback) = 0;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
	virtual void foreachShader(shaders::ShaderVisitor& visitor) = 0;

  // iterate over the list of active shaders (deprecated functions)
  virtual void beginActiveShadersIterator() = 0;
  virtual bool endActiveShadersIterator() = 0;
  virtual IShaderPtr dereferenceActiveShadersIterator() = 0;
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

inline ShaderSystem& GlobalShaderSystem() {
	// Cache the reference locally
	static ShaderSystem& _shaderSystem(
		*boost::static_pointer_cast<ShaderSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM)
		)
	);
	return _shaderSystem;
}

#define QERApp_Shader_ForName GlobalShaderSystem().getShaderForName
#define QERApp_ActiveShaders_IteratorBegin GlobalShaderSystem().beginActiveShadersIterator
#define QERApp_ActiveShaders_IteratorAtEnd GlobalShaderSystem().endActiveShadersIterator
#define QERApp_ActiveShaders_IteratorCurrent GlobalShaderSystem().dereferenceActiveShadersIterator
#define QERApp_ActiveShaders_IteratorIncrement GlobalShaderSystem().incrementActiveShadersIterator

#endif
