#pragma once

#include <memory>
#include "math/Vector3.h"

class ArbitraryMeshVertex;
class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

class OpenGLRenderable;
class LightSources;
class Matrix4;
class IRenderEntity;
class RendererLight;
class LitObject;

#ifdef RENDERABLE_GEOMETRY
// Contains the vertex and index data to render geometry of the given type
struct RenderableGeometry
{
    enum class Type
    {
        Triangles,
        Quads,
        Polygons,
    };

    // The primitive type which will be translated to openGL enums
    virtual Type getType() const = 0;

    // Data as needed by glDrawArrays

    virtual const Vector3& getFirstVertex() = 0;
    virtual std::size_t getVertexStride() = 0;
    virtual const unsigned int& getFirstIndex() = 0;
    virtual std::size_t getNumIndices() = 0;
};
#endif

/**
 * \brief Class which accepts OpenGLRenderable objects during the first pass of
 * rendering.
 *
 * Each Renderable in the scenegraph is passed a reference to a
 * RenderableCollector, to which the Renderable submits its OpenGLRenderable(s)
 * for later rendering. A single Renderable may submit more than one
 * OpenGLRenderable, with different options each time -- for instance a
 * Renderable model class may submit each of its material surfaces separately
 * with different shaders.
 */
class RenderableCollector
{
public:
    virtual ~RenderableCollector() {}

    /**
     * \brief Submit a renderable object.
     *
     * This method allows renderable geometry to be submitted under the control
     * of a LitObject which will determine whether and how the renderable is
     * illuminated by scene lights. Each objected submitted with this method
     * will be considered for lighting by the lights which are submitted to the
     * same RenderableCollector using addLight().
     *
     * Objects may be submitted without a LitObject if they are not affected by
     * scene lights.
     *
     * \param shader
     * The Shader object this Renderable will be attached to.
     *
     * \param renderable
     * The renderable object to submit.
     *
     * \param localToWorld
     * The local to world transform that should be applied to this object when
     * it is rendered.
     *
     * \param entity
     * Optional IRenderEntity exposing parameters which affect the rendering of
     * this Renderable.
     *
     * \param litObject
     * Optional LitObject determining lighting interactions for this
     * renderable. This may or may not be the same actual object as the
     * OpenGLRenderable, depending on how the object class hierarchy is set up.
     * If a single LitObject contains multiple renderables, a separate call to
     * this method must be made for each renderable (with the same litObject
     * parameter).
     */
    virtual void addRenderable(Shader& shader,
                               const OpenGLRenderable& renderable,
                               const Matrix4& localToWorld,
                               const LitObject* litObject = nullptr,
                               const IRenderEntity* entity = nullptr) = 0;

    /**
     * \brief Submit a light source for the render operation.
     *
     * This is the entry point for lights into the render front-end. Each light
     * in the scene graph must be submitted through this method in order to
     * provide light for the final render. If the render is in wireframe mode,
     * light sources can still be submitted but they will not have any effect.
     */
    virtual void addLight(const RendererLight& light) = 0;

    /**
     * \brief Determine if this RenderableCollector can accept renderables for
     * full materials rendering, or just wireframe rendering.
     *
     * \return
     * true if full materials are supported, false if only wireframe rendering
     * is supported.
     */
    virtual bool supportsFullMaterials() const = 0;

    struct Highlight
    {
        enum Flags
        {
            NoHighlight         = 0,
            Faces               = 1 << 0, /// Highlight faces of subsequently-submitted objects, if supported
            Primitives          = 1 << 1, /// Highlight primitives of subsequently-submitted objects, if supported
            GroupMember         = 1 << 2, /// Highlight as member of group, if supported
            MergeAction         = 1 << 3, /// Highlight as merge action (combined with the flags below)
            MergeActionAdd      = 1 << 4, /// Highlight as merge action that is adding something
            MergeActionRemove   = 1 << 5, /// Highlight as merge action that is removing something
            MergeActionChange   = 1 << 6, /// Highlight as merge action that is changing something
            MergeActionConflict = 1 << 7, /// Highlight as merge action conflict
        };
    };

    virtual void setHighlightFlag(Highlight::Flags flags, bool enabled) = 0;

    // Returns true if the current set of highlight flags is not empty
    virtual bool hasHighlightFlags() const = 0;

#ifdef RENDERABLE_GEOMETRY
    // Submits renderable geometry to the collector, it will only rendered in the current frame
    // Flags are a combination of Highlight::Flags
    virtual void addGeometry(RenderableGeometry& geometry, std::size_t flags)
    {}
#endif
};

class VolumeTest;

/**
 * \brief
 * Main interface for Renderable scene objects.
 *
 * All objects which wish to be rendered need to implement this interface.
 * During the scenegraph traversal for rendering, each Renderable object is
 * passed a RenderableCollector object which it can use to submit its geometry
 * and state parameters.
 */
class Renderable
{
public:
    /// Destroy the Renderable
    virtual ~Renderable() {}

    /**
     * Sets the rendersystem this renderable is attached to. This is necessary
     * for this object to request Materials/Shaders for rendering.
     */
    virtual void setRenderSystem(const RenderSystemPtr& renderSystem) = 0;

    // Called in preparation of rendering this node
    virtual void onPreRender(const VolumeTest& volume)
    {}

    /// Submit renderable geometry when rendering in Solid mode.
    virtual void renderSolid(RenderableCollector& collector,
                             const VolumeTest& volume) const = 0;

    /// Submit renderable geometry when rendering in Wireframe mode.
    virtual void renderWireframe(RenderableCollector& collector,
                                 const VolumeTest& volume) const = 0;

    virtual void renderComponents(RenderableCollector&, const VolumeTest&) const
    { }

    virtual void viewChanged() const
    { }

    struct Highlight
    {
        enum Flags
        {
            NoHighlight = 0,
            Selected    = 1 << 0,
            GroupMember = 1 << 1,
        };
    };

    /**
     * Returns information about whether the renderer should highlight this node and how.
     */
    virtual std::size_t getHighlightFlags() = 0;
};
typedef std::shared_ptr<Renderable> RenderablePtr;
