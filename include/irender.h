#pragma once

#include "imodule.h"
#include "ivolumetest.h"
#include "irenderview.h"
#include "iwindingrenderer.h"
#include "igeometryrenderer.h"
#include "isurfacerenderer.h"
#include <functional>
#include <vector>

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "render/Colour4.h"
#include "math/AABB.h"
#include "render/MeshVertex.h"

#include "ishaderlayer.h"
#include <sigc++/signal.h>

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
 * mapping. Also used by shader passes to signal that they care about lighting
 * (and need to be provided with a list of incident lights along with
 * renderable objects).
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
typedef std::shared_ptr<Shader> ShaderPtr;

struct RenderableGeometry;

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
    // Returns the name of this entity (mainly for debugging purposes)
    virtual std::string getEntityName() const = 0;

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

    /**
     * Returns the shader that is used to render coloured primitives like lines and points,
     * using the colour of the entity as defined by its entityDef, light colour, etc.
     * This shader will be applicable to both camera and ortho views, it can be used for
     * visualisations such as target lines and light boxes.
     */
    virtual const ShaderPtr& getColourShader() const = 0;

    /**
     * Returns the colour of this entity, that is used to display its
     * wireframe representation.
     */
    virtual Vector4 getEntityColour() const = 0;

    /**
     * Associates the given object with this entity and the given shader.
     * It will be processed during the following lighting mode rendering passes.
     */
    virtual void addRenderable(const render::IRenderableObject::Ptr& object, Shader* shader) = 0;

    /**
     * Removes the object from this entity.
     */
    virtual void removeRenderable(const render::IRenderableObject::Ptr& object) = 0;

    using ObjectVisitFunction = std::function<void(const render::IRenderableObject::Ptr&, Shader*)>;

    /**
     * Enumerate all entity object, unconditionally.
     */
    virtual void foreachRenderable(const ObjectVisitFunction& functor) = 0;

    /**
     * Enumerate all entity object (partially) intersecting with the given bounds.
     * The bounds are specified in world coordinates.
     */
    virtual void foreachRenderableTouchingBounds(const AABB& bounds, const ObjectVisitFunction& functor) = 0;
};
typedef std::shared_ptr<IRenderEntity> IRenderEntityPtr;
typedef std::weak_ptr<IRenderEntity> IRenderEntityWeakPtr;

/**
 * \brief Interface for a light source in the renderer.
 */
class RendererLight
{
public:
    virtual ~RendererLight() {}

    /**
     * \brief Return the render entity associated with this light
     *
     * The IRenderEntity is used to evaluate possible shader expressions in the
     * shader returned by getShader(). The light object itself may be its own
     * render entity (so getLightEntity() can just return *this).
     */
    virtual const IRenderEntity& getLightEntity() const = 0;

    /// Return the shader for this light
    virtual const ShaderPtr& getShader() const = 0;

    /**
     * \brief Return the world-space to light-texture-space transformation matrix.
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

    /**
     * \brief Return the AABB of the illuminated volume.
     *
     * This AABB represents the boundaries of the volume which are illuminated
     * by this light. Anything outside of this volume does not need to be
     * considered for shading by this light.
     *
     * Note that for omni lights, dragging the light center point outside of
     * the light volume does not expand the lightAABB() value, because the
     * light center only affects the direction of the light rays, not the size
     * of the illuminated volume.
     */
    virtual AABB lightAABB() const = 0;

    /**
     * \brief Return the light origin in world space.
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
typedef std::shared_ptr<RendererLight> RendererLightPtr;

/// Debug stream insertion for RendererLight
inline std::ostream& operator<< (std::ostream& os, const RendererLight& l)
{
    return os << "RendererLight(origin=" << math::pp(l.getLightOrigin())
              << ", lightAABB=" << l.lightAABB() << ")";
}

class Renderable;
typedef std::function<void(Renderable&)> RenderableCallback;

typedef std::function<void(const RendererLight&)> RendererLightCallback;

/**
 * \brief
 * Interface for objects which can render themselves in OpenGL.
 *
 * This interface is used for highlight rendering, after renderable objects have
 * first been submitted using the Renderable interface. The backend render()
 * function should contain the OpenGL calls necessary to submit vertex, normal
 * and texture-coordinate data.
 */
class OpenGLRenderable
{
public:
    virtual ~OpenGLRenderable() {}

    /**
     * \brief
     * Submit OpenGL render calls.
     */
    virtual void render() const = 0;
};

class Matrix4;
class Texture;

#include "math/Vector3.h"

class Material;
typedef std::shared_ptr<Material> MaterialPtr;

/**
 * A Shader represents a single material which can be rendered in OpenGL, which
 * may correspond to an actual material (Material), a raw colour or a special
 * GL shader.
 *
 * Importantly, a Shader also maintains its own list of OpenGLRenderable objects
 * which use it -- the actual rendering is performed by traversing a list of
 * Shaders and rendering the geometry attached to each one.
 */
class Shader :
    public render::IWindingRenderer,
    public render::IGeometryRenderer,
    public render::ISurfaceRenderer
{
public:
	// Observer interface to get notified on (un-)realisation
	class Observer
	{
	public:
		virtual void onShaderRealised() = 0;
		virtual void onShaderUnrealised() = 0;
	};

    virtual ~Shader() {}

    /// Return the name used to construct this shader
    virtual std::string getName() const = 0;

	/**
     * \brief
	 * Attach a renderable object to this Shader, which will be rendered using
	 * this Shader when the render backend is activated.
	 *
	 * \param renderable
	 * The OpenGLRenderable object to add.
	 *
	 * \param modelview
	 * The modelview transform for this object.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable,
							   const Matrix4& modelview) = 0;

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

	// Attach/detach an observer to this shader object.
	// In case the shader is already realised when attachObserver() is called,
	// the observer's onShaderRealised() method is immediately invoked.
	// The analogous holds for detachObserver(): if the shader is realised,
	// the observer's onShaderUnrealised() method is invoked before unregistering it.
	virtual void attachObserver(Observer& observer) = 0;
	virtual void detachObserver(Observer& observer) = 0;

	virtual bool isRealised() = 0;

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
typedef std::shared_ptr<Shader> ShaderPtr;

// Defines a piece of text that should be rendered in the scene
class IRenderableText
{
public:
    // The position in world space where the text needs to be rendered
    virtual const Vector3& getWorldPosition() = 0;

    // The text to be rendered. Returning an empty text will skip this renderable.
    virtual const std::string& getText() = 0;

    // The colour of the text
    virtual const Vector4& getColour() = 0;
};

// An ITextRenderer is able to draw one or more IRenderableText
// instances to the scene
class ITextRenderer
{
public:
    using Ptr = std::shared_ptr<ITextRenderer>;

    using Slot = std::uint64_t;
    static constexpr Slot InvalidSlot = std::numeric_limits<Slot>::max();

    // Registers the given text instance for rendering
    virtual Slot addText(IRenderableText& text) = 0;

    // Removes the text instance in the given Slot
    // The slot handle is invalidated by this operation and should be 
    // discarded by the client, by setting it to InvalidSlot
    virtual void removeText(Slot slot) = 0;
};

const char* const MODULE_RENDERSYSTEM("ShaderCache");

// Render view type enumeration, is used to select (or leave out)
// Shader objects during rendering
enum class RenderViewType
{
    Camera      = 1 << 0,
    OrthoView   = 1 << 1,
};

enum class BuiltInShaderType
{
    // A medium-sized point
    Point,

    // A bigger version of "Point"
    BigPoint,

    // Lines connecting the patch control points
    PatchLattice,

    // Line shader drawing above regular things
    WireframeOverlay,

    // Fill shader drawing above regular things
    FlatshadeOverlay,

    // Line shader used for drawing AAS area bounds
    AasAreaBounds,

    // Shader used for the boxes marking missing models
    MissingModel,

    // The polygon showing the clip plane intersecting brushes
    BrushClipPlane,

    // Line shader connecting the leak point trace
    PointTraceLines,

    // This is the shader drawing a coloured overlay
    // over faces/polys. Its colour is configurable,
    // and it has depth test activated (camera).
    ColouredPolygonOverlay,

    // This is the shader drawing a solid line to outline
    // a selected item. The first pass has its depth test
    // activated using GL_LESS, whereas the second pass
    // draws the hidden lines in stippled appearance
    // with its depth test using GL_GREATER (camera).
    HighlightedPolygonOutline,

    // Coloured line stipple overlay of selected items (ortho)
    WireframeSelectionOverlay,

    // Coloured line stipple overlay of selected grouped items (ortho)
    WireframeSelectionOverlayOfGroups,

    // Line shader used for drawing the three axes at entity origins
    Pivot,

    // Coloured merge action highlights (camera)
    CameraMergeActionOverlayAdd,
    CameraMergeActionOverlayRemove,
    CameraMergeActionOverlayChange,
    CameraMergeActionOverlayConflict,

    // Coloured merge action highlights (ortho)
    OrthoMergeActionOverlayAdd,
    OrthoMergeActionOverlayRemove,
    OrthoMergeActionOverlayChange,
    OrthoMergeActionOverlayConflict,
};

// Available types of colour shaders. These areused 
// to draw map items in the ortho view, or to render
// connection lines, light volumes in both camera and ortho.
enum class ColourShaderType
{
    // Fill shader, non-transparent
    CameraSolid,

    // Fill shader with transparency
    CameraTranslucent,

    // Solid lines used for ortho view rendering
    OrthoviewSolid,

    // Items drawn in both camera and ortho views
    CameraAndOrthoview,
};

class IRenderResult
{
public:
    using Ptr = std::shared_ptr<IRenderResult>;

    virtual ~IRenderResult() {}

    virtual std::string toString() = 0;
};

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
	 * @param name
	 * The name of the shader to capture.
	 *
	 * @returns
	 * Shader* object corresponding to the given material shader name.
	 */

	virtual ShaderPtr capture(const std::string& name) = 0;

    /**
     * Retrieves an ITextRenderer instance with the given font style and size.
     * This renderer will accept IRenderableText instances which define
     * the actual text, colour and position.
     */
    virtual ITextRenderer::Ptr captureTextRenderer(IGLFont::Style style, std::size_t size) = 0;

    /**
     * Acquire one of DarkRadiant's built-in shaders, used for drawing
     * things like connectors, lines, points and overlays.
     */
    virtual ShaderPtr capture(BuiltInShaderType type) = 0;

    /**
     * Capture a colour shader, there are various types available
     * for rendering in ortho views, camera or both.
     */
    virtual ShaderPtr capture(ColourShaderType type, const Colour4& colour) = 0;

    /**
     * Register the given entity to be considered during rendering.
     * If this entity is a light it will be automatically recognised 
     * and inserted into the set of lights.
     */
    virtual void addEntity(const IRenderEntityPtr& renderEntity) = 0;

    /**
     * Detaches this entity from this rendersystem, it won't be 
     * affected by any rendering after this call.
     */
    virtual void removeEntity(const IRenderEntityPtr& renderEntity) = 0;

    /**
     * Enumerates all known render entities in this system, invoking
     * the functor for each of them.
     */
    virtual void foreachEntity(const std::function<void(const IRenderEntityPtr&)>& functor) = 0;

    /**
     * Enumerates all known render lights in this system, invoking
     * the functor for each of them
     */
    virtual void foreachLight(const std::function<void(const RendererLightPtr&)>& functor) = 0;

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
     * * \param view
     * The view used to setup the projection, modelview and lighting calculations.
     */
    virtual IRenderResult::Ptr renderFullBrightScene(RenderViewType renderViewType,
                        RenderStateFlags globalFlagsMask,
                        const render::IRenderView& view) = 0;

    virtual void startFrame() = 0;
    virtual void endFrame() = 0;

    /**
     * Render the scene based on the light-entity interactions.
     * All the active lights and entities must have added themselves
     * to this rendersystem at this point, using addEntity().
     * 
     * \param globalFlagsMask
     * The mask of render flags which are permitted during this render pass. Any
     * render flag which is 0 in this mask will not be enabled during rendering,
     * even if the particular shader requests it.
     * 
     * \returns A result object which can be used to display a statistics summary.
     */
    virtual IRenderResult::Ptr renderLitScene(RenderStateFlags globalFlagsMask, 
        const render::IRenderView& view) = 0;

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

    /// Get the current shader program in use.
    virtual ShaderProgram getCurrentShaderProgram() const = 0;

    /// Set the shader program to use.
    virtual void setShaderProgram(ShaderProgram prog) = 0;

    virtual void attachRenderable(Renderable& renderable) = 0;
    virtual void detachRenderable(Renderable& renderable) = 0;
    virtual void forEachRenderable(const RenderableCallback& callback) const = 0;

  	// Initialises the OpenGL extensions
    virtual void extensionsInitialised() = 0;

    // Returns true if openGL supports ARB or GLSL lighting
    virtual bool shaderProgramsAvailable() const = 0;

    // Sets the flag whether shader programs are available.
    virtual void setShaderProgramsAvailable(bool available) = 0;

    // Activates or deactivates the merge render mode
    virtual void setMergeModeEnabled(bool enabled) = 0;

	// Subscription to get notified as soon as the openGL extensions have been initialised
	virtual sigc::signal<void> signal_extensionsInitialised() = 0;
};
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;
typedef std::weak_ptr<RenderSystem> RenderSystemWeakPtr;

/**
 * \brief
 * Global accessor method for the RenderSystem instance.
 */
inline RenderSystem& GlobalRenderSystem()
{
    static module::InstanceReference<RenderSystem> _reference(MODULE_RENDERSYSTEM);
    return _reference;
}
