/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#if !defined(INCLUDED_IRENDER_H)
#define INCLUDED_IRENDER_H

#include "imodule.h"
#include "generic/callbackfwd.h"

#include "math/Vector3.h"

#include "ShaderLayer.h"

// Rendering states to sort by.
// Higher bits have the most effect - slowest state changes should be highest.

const unsigned int RENDER_DEFAULT = 0;
const unsigned int RENDER_LINESTIPPLE = 1 << 0; // glEnable(GL_LINE_STIPPLE)
const unsigned int RENDER_LINESMOOTH = 1 << 1; // glEnable(GL_LINE_SMOOTH)
const unsigned int RENDER_POLYGONSTIPPLE = 1 << 2; // glEnable(GL_POLYGON_STIPPLE)
const unsigned int RENDER_POLYGONSMOOTH = 1 << 3; // glEnable(GL_POLYGON_SMOOTH)
const unsigned int RENDER_ALPHATEST = 1 << 4; // glEnable(GL_ALPHA_TEST)
const unsigned int RENDER_DEPTHTEST = 1 << 5; // glEnable(GL_DEPTH_TEST)
const unsigned int RENDER_DEPTHWRITE = 1 << 6; // glDepthMask(GL_TRUE)
const unsigned int RENDER_COLOURWRITE = 1 << 7; // glColorMask(GL_TRUE; GL_TRUE; GL_TRUE; GL_TRUE)
const unsigned int RENDER_CULLFACE = 1 << 8; // glglEnable(GL_CULL_FACE)
const unsigned int RENDER_SCALED = 1 << 9; // glEnable(GL_NORMALIZE)
const unsigned int RENDER_SMOOTH = 1 << 10; // glShadeModel
const unsigned int RENDER_LIGHTING = 1 << 11; // glEnable(GL_LIGHTING)
const unsigned int RENDER_BLEND = 1 << 12; // glEnable(GL_BLEND)
const unsigned int RENDER_OFFSETLINE = 1 << 13; // glEnable(GL_POLYGON_OFFSET_LINE)
const unsigned int RENDER_FILL = 1 << 14; // glPolygonMode
const unsigned int RENDER_COLOURARRAY = 1 << 15; // glEnableClientState(GL_COLOR_ARRAY)
const unsigned int RENDER_COLOURCHANGE = 1 << 16; // render() is allowed to call glColor*()
const unsigned int RENDER_MATERIAL_VCOL = 1 << 17; // material requests per-vertex colour
const unsigned int RENDER_VCOL_INVERT = 1 << 18; // vertex colours should be inverted
const unsigned int RENDER_TEXTURE_2D = 1 << 19; // glEnable(GL_TEXTURE_2D)
const unsigned int RENDER_TEXTURE_CUBEMAP = 1 << 20; // glEnable(GL_TEXTURE_CUBE_MAP)
const unsigned int RENDER_BUMP = 1 << 21;
const unsigned int RENDER_PROGRAM = 1 << 22;
const unsigned int RENDER_SCREEN = 1 << 23;
const unsigned int RENDER_OVERRIDE = 1 << 24;
typedef unsigned int RenderStateFlags;


class AABB;
class Matrix4;

template<typename Element> class BasicVector3;

class Shader;

/**
 * \brief
 * Interface for a light source in the renderer.
 */
class RendererLight
{
public:
	virtual boost::shared_ptr<Shader> getShader() const = 0;

    /**
     * \brief
     * Return the origin of the light volume in world space.
     *
     * This corresponds to the "origin" key of the light object, i.e. the center
     * of the bounding box for an omni light and the tip of the pyramid for a
     * projected light.
     */
    virtual Vector3 worldOrigin() const = 0;

    /**
     * \brief
     * Return the world-space to light-texture-space transformation matrix.
     *
     * The light texture space is a box, with coordinates [0..1] on each
     * dimension, representing the texture (UV) coordinates of the light falloff
     * textures that will be applied to rendered fragments within the light
     * volume.
     *
     * The matrix returned by this method transforms coordinates in world space
     * into coordinates in light-texture space.
     */
    virtual Matrix4 getLightTextureTransformation() const = 0;

	virtual bool testAABB(const AABB& other) const = 0;

    /**
     * \brief
     * Return the light origin in world space.
     *
     * The light origin is the point from which the light rays are considered to
     * be projected, i.e. the direction from which bump maps will be illuminated
     * and shadows (if they existed) would be cast.
     *
     * For an omindirectional light, this origin is determined from the
     * "light_center" keyvalue in combination with the bounding box itself,
     * whereas for a projected light it is always equal to the tip of the
     * pyramid (the same as worldOrigin()).
     */
	virtual Vector3 getLightOrigin() const = 0;

	virtual const Vector3& colour() const = 0;
};
typedef boost::shared_ptr<RendererLight> RendererLightPtr;

/**
 * \brief
 * Interface for an object which can test its intersection with a RendererLight.
 *
 * Objects which implement this interface define a testLight() function which
 * determines whether the given light intersects the object. They also provide
 * methods to allow the renderer to provide the list of lights which will be
 * illuminating the object, subsequent to the intersection test.
 *
 * \todo
 * This interface seems to exist because of the design decision that lit objects
 * should maintain a list of lights which illuminate them. This is a poor
 * design because this should be the responsibility of the renderer. When the
 * renderer is refactored to process the scene light-by-light this class will
 * not be necessary.
 */
class LightCullable
{
public:
	virtual bool testLight(const RendererLight& light) const = 0;
	virtual void insertLight(const RendererLight& light) {}
	virtual void clearLights() {}
};
typedef boost::shared_ptr<LightCullable> LightCullablePtr;

class Renderable;
typedef Callback1<const Renderable&> RenderableCallback;

typedef Callback1<const RendererLight&> RendererLightCallback;

class LightList
{
public:
  virtual void evaluateLights() const = 0;
  virtual void lightsChanged() const = 0;
  virtual void forEachLight(const RendererLightCallback& callback) const = 0;
};

const int c_attr_TexCoord0 = 1;
const int c_attr_Tangent = 3;
const int c_attr_Binormal = 4;

/**
 * \brief
 * Data object passed to the backend OpenGLRenderable::render() method
 * containing information about the render pass which may be of use to
 * renderable objects, including the render flags and various
 * matrices/coordinates.
 */
class RenderInfo
{
    // Render flags
    RenderStateFlags _flags;

    // Viewer location in 3D space
    Vector3 _viewerLocation;

    // Cube map mode
    ShaderLayer::CubeMapMode _cubeMapMode;

public:

    /**
     * \brief
     * Constructor.
     */
    RenderInfo(RenderStateFlags flags = RENDER_DEFAULT,
               const Vector3& viewer = Vector3(0, 0, 0),
               ShaderLayer::CubeMapMode cubeMode = ShaderLayer::CUBE_MAP_NONE)
    : _flags(flags),
      _viewerLocation(viewer),
      _cubeMapMode(cubeMode)
    { }

    /**
     * \brief
     * Check if a flag is set
     */
    bool checkFlag(unsigned flag) const
    {
        return (_flags & flag) != 0;
    }

    /**
     * \brief
     * Get the entire flag bitfield.
     */
    RenderStateFlags getFlags() const
    {
        return _flags;
    }

    /**
     * \brief
     * Get the viewer location.
     */
    const Vector3& getViewerLocation() const
    {
        return _viewerLocation;
    }

    /**
     * \brief
     * Get the cube map mode.
     */
    ShaderLayer::CubeMapMode getCubeMapMode() const
    {
        return _cubeMapMode;
    }

};

/**
 * \brief
 * Interface for objects which can render themselves in OpenGL.
 *
 * This interface is used by the render backend, after renderable objects have
 * first been submitted using the Renderable interface. The backend render()
 * function should contain the OpenGL calls necessary to submit vertex, normal
 * and texture-coordinate data. 
 *
 * No GL state changes should occur in render(), other than those specifically
 * allowed by the render flags (such as glColor() if RENDER_COLOURWRITE is set).
 */
class OpenGLRenderable
{
public:

    /**
     * \brief
     * Submit OpenGL render calls.
     */
    virtual void render(const RenderInfo& info) const = 0;
};

class Matrix4;
class Texture;
class ModuleObserver;

#include "math/Vector3.h"

class Material;
typedef boost::shared_ptr<Material> MaterialPtr;

/**
 * A Shader represents a single material which can be rendered in OpenGL, which
 * may correspond to an actual material (Material), a raw colour or a special 
 * GL shader.
 * 
 * Importantly, a Shader also maintains its own list of OpenGLRenderable objects
 * which use it -- the actual rendering is performed by traversing a list of
 * Shaders and rendering the geometry attached to each one.
 */
class Shader
{
public:
	
	/**
	 * Attach a renderable object to this Shader, which will be rendered using
	 * this Shader when the render backend is activated.
	 * 
	 * @param renderable
	 * The OpenGLRenderable object to add.
	 * 
	 * @param modelview
	 * The modelview transform for this object.
	 * 
	 * @param lights
	 * A LightList containing all of the lights which should illuminate this
	 * object.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable, 
							   const Matrix4& modelview, 
							   const LightList* lights = 0) = 0;
  
  virtual void incrementUsed() = 0;
  virtual void decrementUsed() = 0;
  virtual void attach(ModuleObserver& observer) = 0;
  virtual void detach(ModuleObserver& observer) = 0;
  
	/** Retrieve the contained Material from this object.
	 * 
	 * @returns
	 * An Material subclass with information about the shader definition
	 */
	 
	virtual MaterialPtr getMaterial() const = 0;
  
  virtual unsigned int getFlags() const = 0;
};

/**
 * Shared pointer typedef for Shader.
 */
typedef boost::shared_ptr<Shader> ShaderPtr;

const std::string MODULE_RENDERSYSTEM("ShaderCache");

/**
 * \brief
 * The main interface for the backend renderer.
 */
class RenderSystem
: public RegisterableModule
{
public:

	/** 
     * \brief
     * Capture the given shader, increasing its reference count and
	 * returning a pointer to the Shader object.
     *
     * The object must be freed after use by calling release().
	 * 
	 * @param name
	 * The name of the shader to capture.
	 * 
	 * @returns
	 * Shader* object corresponding to the given material shader name.
	 */
	 
	virtual ShaderPtr capture(const std::string& name) = 0;

    /**
     * \brief
     * Main render method.
     *
     * This method traverses all of the OpenGLRenderable objects that have been
     * submitted to Shader instances, and invokes their render() method to draw
     * their geometry.
     *
     * \param globalFlagsMask
     * The mask of render flags which are permitted during this render pass. Any
     * render flag which is 0 in this mask will not be enabled during rendering,
     * even if the particular shader requests it.
     *
     * \param modelview
     * The modelview transformation matrix to apply before rendering.
     *
     * \param projection
     * The view projection matrix to apply before rendering.
     *
     * \param viewer
     * Location of the viewer in world space.
     */
    virtual void render(RenderStateFlags globalFlagsMask,
                        const Matrix4& modelview,
                        const Matrix4& projection,
                        const Vector3& viewer = Vector3(0, 0, 0)) = 0;

  virtual void realise() = 0;
  virtual void unrealise() = 0;

	/**
	 * Test if lighting mode is supported, depending on OpenGL extensions.
	 */
	virtual bool lightingSupported() const = 0;
	
	/**
	 * Test if lighting mode is ENABLED.
	 */
	virtual bool lightingEnabled() const = 0;
	
	// Enable/disable lighting mode
	virtual void setLightingEnabled(bool enabled) = 0;

  virtual const LightList& attach(LightCullable& cullable) = 0;
  virtual void detach(LightCullable& cullable) = 0;
  virtual void changed(LightCullable& cullable) = 0;

    /**
     * \brief
     * Attach a light source to the renderer.
     */
    virtual void attachLight(RendererLight& light) = 0;

    /**
     * \brief
     * Detach a light source from the renderer.
     */
    virtual void detachLight(RendererLight& light) = 0;

    /**
     * \brief
     * Indicate that the given light source has been modified.
     */
    virtual void lightChanged(RendererLight& light) = 0;

  virtual void attachRenderable(const Renderable& renderable) = 0;
  virtual void detachRenderable(const Renderable& renderable) = 0;
  virtual void forEachRenderable(const RenderableCallback& callback) const = 0;
  
  	// Initialises the OpenGL extensions 
    virtual void extensionsInitialised() = 0;
};

/**
 * \brief
 * Global accessor method for the RenderSystem instance.
 */
inline RenderSystem& GlobalRenderSystem() 
{
	// Cache the reference locally
	static RenderSystem& _instance(
		*boost::static_pointer_cast<RenderSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEM)
		)
	);
	return _instance;
}

#endif
