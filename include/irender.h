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
#pragma once

#include "imodule.h"
#include <boost/function/function_fwd.hpp>

#include "math/Vector3.h"
#include <boost/weak_ptr.hpp>

#include "ShaderLayer.h"

/**
 * \file
 * Interfaces for the back-end renderer.
 */

/**
 * \name Global render flags
 *
 * These flags control which capabilities of the renderer are used throughout
 * the render process. They have a four stage lifecycle:
 *
 * 1. The flags are initially SET in the Shader implementation, describing the
 * features that the particular Shader would like to use for rendering its
 * renderables. For example, a shader pass performing a blend will set
 * RENDER_BLEND as one of its flags.
 *
 * 2. The flags are MASKED by another set of flags provided to a
 * RenderableCollector before it traverses the scene graph, in order to control
 * which shader-specified flags can actually be used for that render pass. For
 * example, the XYRenderer renders in wireframe mode only, so it does not enable
 * RENDER_FILL in its mask, while the CamRenderer does.
 *
 * 3. The flags may be used to set or change OpenGL state in the shader pass
 * implementation. For example, if RENDER_BLEND is set, then glEnable(GL_BLEND)
 * will be called before the associated shader's renderables are rendered. Some
 * flags map directly to glEnable parameters, while others (such as
 * RENDER_PROGRAM) specify more complex changes. Some flags do not enable any GL
 * features at all.
 *
 * 4. The flags are passed as a parameter to the OpenGLRenderable::render()
 * method, allowing individual objects to modify their behaviour accordingly.
 * For example, objects may decide whether or not to submit colour information
 * to OpenGL based on the value of the RENDER_VERTEX_COLOUR flag.
 */
///@{
const unsigned RENDER_DEFAULT = 0;
const unsigned RENDER_LINESTIPPLE = 1 << 0; // glEnable(GL_LINE_STIPPLE)
const unsigned RENDER_POLYGONSTIPPLE = 1 << 2; // glEnable(GL_POLYGON_STIPPLE)
const unsigned RENDER_ALPHATEST = 1 << 4; // glEnable(GL_ALPHA_TEST)
const unsigned RENDER_DEPTHTEST = 1 << 5; // glEnable(GL_DEPTH_TEST)
const unsigned RENDER_DEPTHWRITE = 1 << 6; // glDepthMask(GL_TRUE)

/// Colour buffer writing disabled with glColorMask
const unsigned RENDER_MASKCOLOUR = 1 << 7;

const unsigned RENDER_CULLFACE = 1 << 8; // glglEnable(GL_CULL_FACE)
const unsigned RENDER_SCALED = 1 << 9; // glEnable(GL_NORMALIZE)
const unsigned RENDER_SMOOTH = 1 << 10; // glShadeModel
const unsigned RENDER_LIGHTING = 1 << 11; // glEnable(GL_LIGHTING)
const unsigned RENDER_BLEND = 1 << 12; // glEnable(GL_BLEND)
const unsigned RENDER_OFFSETLINE = 1 << 13; // glEnable(GL_POLYGON_OFFSET_LINE)

/// Objects will be rendered as filled polygons (not wireframe).
const unsigned RENDER_FILL = 1 << 14;

/**
 * If enabled, mesh objects (geometry that does not consist only of GL_POINTS)
 * should submit vertex colour information. If disabled, mesh objects must not
 * change glColor during rendering.
 *
 * Does not affect GL state.
 */
const unsigned RENDER_VERTEX_COLOUR = 1 << 15;

/**
 * If enabled, point geometry may submit colours for each point. If disabled,
 * point geometry must not change colour during rendering.
 *
 * Does not affect GL state.
 */
const unsigned RENDER_POINT_COLOUR = 1 << 16;

/// GL_TEXTURE_2D will be enabled during rendering.
const unsigned RENDER_TEXTURE_2D = 1 << 17;

/**
 * Cube map rendering (in camera space) is enabled. The renderer will enable
 * GL_TEXTURE_CUBE_MAP, and set up the texture matrix such that the viewer
 * location is the origin.  Objects should submit their vertex coordinates as
 * texture coordinates, which will result in the correct cube map alignment.
 */
const unsigned RENDER_TEXTURE_CUBEMAP = 1 << 18;

/**
 * Normal map information will be used during rendering. If enabled, objects
 * should submit normal/tangent/bitangent vertex attributes to enable normal
 * mapping.
 */
const unsigned RENDER_BUMP = 1 << 19;

/// A vertex and fragment shader program will be used during rendering.
const unsigned RENDER_PROGRAM = 1 << 20;

const unsigned RENDER_OVERRIDE = 1 << 21;
typedef unsigned RenderStateFlags;
///@}

class AABB;
class Matrix4;

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;

class Shader;
typedef boost::shared_ptr<Shader> ShaderPtr;

/**
 * A RenderEntity represents a map entity as seen by the renderer.
 * It provides up to 12 numbered parameters to the renderer:
 * parm0, parm1 ... parm11.
 *
 * A few of the entity parms are hardwired to things like render colour
 * as defined through the entity's _color keyvalue, some are set through
 * scripting, spawmargs or gameplay code.
 */
class IRenderEntity
{
public:

	/**
	 * Get the value of this entity's shader parm with the given index.
	 */
	virtual float getShaderParm(int parmNum) const = 0;

	/**
	 * Entities can specify directions, which are used for particle emission for instance.
	 */
	virtual const Vector3& getDirection() const = 0;

	/**
	 * Returns the wireframe shader for this entity - child primitives need this for rendering.
	 */
	virtual const ShaderPtr& getWireShader() const = 0;
};
typedef boost::shared_ptr<IRenderEntity> IRenderEntityPtr;

/**
 * \brief
 * Interface for a light source in the renderer.
 */
class RendererLight :
	public virtual IRenderEntity
{
public:
    virtual ~RendererLight() {}
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

    /// Return true if this light intersects the given AABB
	virtual bool intersectsAABB(const AABB& aabb) const = 0;

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
};
typedef boost::shared_ptr<RendererLight> RendererLightPtr;

inline std::ostream& operator<< (std::ostream& os, const RendererLight& l)
{
    return os << "RendererLight { worldOrigin = " << l.worldOrigin() << " }";
}

/**
 * \brief
 * Interface for an object which can test its intersection with a RendererLight.
 *
 * Objects which implement this interface define a intersectsLight() function
 * which determines whether the given light intersects the object. They also
 * provide methods to allow the renderer to provide the list of lights which
 * will be illuminating the object, subsequent to the intersection test.
 *
 * \todo
 * This interface seems to exist because of the design decision that lit objects
 * should maintain a list of lights which illuminate them. This is a poor
 * design because this should be the responsibility of the renderer. When the
 * renderer is refactored to process the scene light-by-light this class will
 * not be necessary.
 */
class LitObject
{
public:
    virtual ~LitObject() {}

    /// Test if the given light intersects the LitObject
    virtual bool intersectsLight(const RendererLight& light) const = 0;

    /// Add a light to the set of lights which do intersect this object
    virtual void insertLight(const RendererLight& light) {}

    /// Clear out all lights in the set of lights intersecting this object
    virtual void clearLights() {}
};
typedef boost::shared_ptr<LitObject> LitObjectPtr;

class Renderable;
typedef boost::function<void(const Renderable&)> RenderableCallback;

typedef boost::function<void(const RendererLight&)> RendererLightCallback;

/**
 * \brief
 * A list of lights which may intersect an object
 *
 * A LightList is responsible for calculating which lights intersect a
 * particular object. Although there is nothing exposed in the interface, the
 * LightList holds a reference to a single lit object, and it is the
 * intersection with this object which is calculated.
 *
 * \internal
 * This interface doesn't really make any sense, and its purpose is not clear.
 * It seems to be basically a set of callback functions which need to be invoked
 * at the right time during the render process, but these shouldn't really be
 * the responsibility of anything outside the renderer itself.
 *
 * As of 2011-01-09/r6927 the calling sequence seems to be as follows:
 *
 * 1. Illuminated object (e.g. patch, brush) adds itself to the RenderSystem
 * with attachLitObject() at construction.
 * 2. attachLitObject() returns a reference to a (newly-created) LightList which
 * manages the lights intersecting this lit object. The lit object stores this
 * reference internally, while the LightList implementation also stores a
 * reference to the LitObject.
 * 3. When the lit object's renderSolid() method is invoked to set up a render,
 * it invokes LightList::calculateIntersectingLights() on the stored LightList
 * reference.
 * 4. calculateIntersectingLights() first checks to see if the lights need
 * updating, which is true if EITHER this LightList's setDirty() method OR the
 * RenderSystem's lightChanged() has been called since the last calculation. If
 * no update is needed, it returns.
 * 5. If an update IS needed, the LightList iterates over all lights in the
 * scene, and tests if each one intersects its associated lit object (which is
 * the one that just invoked calculateIntersectingLights(), although nothing
 * enforces this). This intersection test is performed by passing the light to
 * the LitObject::intersectsLight() method.
 * 6. For each light which passes the intersection test, the LightList both adds
 * it to its internal list of "active" (i.e. intersecting) lights for its
 * object, and passes it to the object's insertLight() method. Some object
 * classes then use insertLight() to populate another internal LightList subject
 * to additional (internal) intersection tests, but this is not required.
 * 7. At this point, calculateIntersectingLights() has finished, and returns
 * control to its calling renderSolid() method.
 * 8. The renderSolid() method (or another method it calls) passes a LightList
 * to the RenderableCollector with setLights(). The light list it passes may be
 * the original list returned from attachLitObject(), or the additional internal
 * list populated in step 6.
 * 9. The RenderableCollector state machine stores the LightList as the
 * "current" light list.
 * 10. Any subsequent renderables submitted with
 * RenderableCollector::addRenderable() are associated with the current
 * LightList passed in the previous step, and passed to the current Shader.
 * 11. The OpenGLShader accepts the renderable and LightList, and adds them to
 * its internal OpenGLShaderPasses: once only if RENDER_BUMP is not active, not
 * at all if RENDER_BUMP is active but the LightList is NULL, or once for each
 * light in the LightList otherwise.
 * 12. The OpenGLShaderPass now contains a list of TransformedRenderable
 * structures, each associating a single renderable with a single light.
 * Multiple TransformedRenderable will exist for the same renderable if there
 * were multiple lights illuminating it.
 */
class LightList
{
public:
    virtual ~LightList() {}

    /// Trigger the LightList to recalculate which lights intersect its object
    virtual void calculateIntersectingLights() const = 0;

    /// Set the dirty flag, informing the LightList that an update is required
    virtual void setDirty() = 0;

    /// Invoke a callback on all contained lights.
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

    /// Default constructor
    RenderInfo(RenderStateFlags flags = RENDER_DEFAULT,
               const Vector3& viewer = Vector3(0, 0, 0),
               ShaderLayer::CubeMapMode cubeMode = ShaderLayer::CUBE_MAP_NONE)
    : _flags(flags),
      _viewerLocation(viewer),
      _cubeMapMode(cubeMode)
    { }

    /// Check if a flag is set
    bool checkFlag(unsigned flag) const
    {
        return (_flags & flag) != 0;
    }

    /// Get the entire flag bitfield.
    RenderStateFlags getFlags() const
    {
        return _flags;
    }

    /// Get the viewer location.
    const Vector3& getViewerLocation() const
    {
        return _viewerLocation;
    }

    /// Get the cube map mode.
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
 * allowed by the render flags.
 */
class OpenGLRenderable
{
public:
    virtual ~OpenGLRenderable() {}

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
    virtual ~Shader() {}

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

	/**
	 * Like above, but taking an additional IRenderEntity argument.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable,
							   const Matrix4& modelview,
							   const IRenderEntity& entity,
							   const LightList* lights = 0) = 0;

    /**
     * \brief
     * Control the visibility of this shader.
     *
     * A shader that is not visible will perform no rendering and ignore any
     * renderables submitted to it with addRenderable().
     */
    virtual void setVisible(bool visible) = 0;

    /// Query if this shader is visible
    virtual bool isVisible() const = 0;

    virtual void incrementUsed() = 0;
    virtual void decrementUsed() = 0;
    virtual void attach(ModuleObserver& observer) = 0;
    virtual void detach(ModuleObserver& observer) = 0;

	/**
     * \brief Retrieve the Material that was used to construct this shader (if
     * any).
	 *
	 * \return
	 * An Material subclass with information about the shader definition
	 */

	virtual const MaterialPtr& getMaterial() const = 0;

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
	 * Get the current render time in milliseconds.
	 */
	virtual std::size_t getTime() const = 0;

	/**
	 * Set the render time in milliseconds.
	 */
	virtual void setTime(std::size_t milliSeconds) = 0;

    /* SHADER PROGRAMS */

    /// Available GL programs used for backend rendering.
    enum ShaderProgram
    {
        /// No shader program (normal GL fixed-function pipeline)
        SHADER_PROGRAM_NONE,

        /// Lighting interaction shader
        SHADER_PROGRAM_INTERACTION
    };

    /// Test if shader programs are available on the current system.
    virtual bool shaderProgramsAvailable() const = 0;

    /// Get the current shader program in use.
    virtual ShaderProgram getCurrentShaderProgram() const = 0;

    /// Set the shader program to use.
    virtual void setShaderProgram(ShaderProgram prog) = 0;

    /* LIGHT MANAGEMENT */

    /**
     * \brief
     * Add a lit object to the renderer.
     *
     * The renderer will create and return a reference to a LightList associated
     * with this particular LitObject. The lit object can use the public
     * LightList interface to trigger a recalculation of light intersections, or
     * to set the dirty flag indicating to the LightList that a recalculation is
     * necessary.
     *
     * \internal
     * When the LightList implementation performs the intersection calculation,
     * it will use the LitObject's intersectsLight method to do so, and if the
     * intersection is detected, the insertLight method will be invoked on the
     * LitObject. This means that (1) the renderer stores a LightList for each
     * object, (2) the object itself has a reference to the LightList owned by
     * the renderer, and (3) the LitObject interface allows the object to
     * maintain ANOTHER list of intersecting lights, added with insertLight().
     * This seems like a lot of indirection, but it might have something to do
     * with allowing objects with multiple sub-components to submit only a
     * subset of lights for each component.
     *
     * \param object
     * The lit object to add.
     *
     * \return
     * A reference to a LightList which manages the lights that intersect the
     * submitted object.
     */
    virtual LightList& attachLitObject(LitObject& object) = 0;

    virtual void detachLitObject(LitObject& cullable) = 0;

    virtual void litObjectChanged(LitObject& cullable) = 0;

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
typedef boost::shared_ptr<RenderSystem> RenderSystemPtr;
typedef boost::weak_ptr<RenderSystem> RenderSystemWeakPtr;

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
