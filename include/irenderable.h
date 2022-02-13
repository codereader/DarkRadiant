#pragma once

#include <memory>
#include "math/Vector3.h"

class ArbitraryMeshVertex;
class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

class OpenGLRenderable;
class Matrix4;
class IRenderEntity;
class RendererLight;
class LitObject;
class Renderable;
class VolumeTest;

/**
 * \brief Class which accepts OpenGLRenderable objects during the first pass of
 * rendering.
 *
 * Each Renderable in the scenegraph is passed a reference to an
 * IRenderableCollector, to which the Renderable submits its OpenGLRenderable(s)
 * for later rendering. A single Renderable may submit more than one
 * OpenGLRenderable, with different options each time -- for instance a
 * Renderable model class may submit each of its material surfaces separately
 * with different shaders.
 */
class IRenderableCollector
{
public:
    virtual ~IRenderableCollector() {}

    // Process the given renderable object
    virtual void processRenderable(Renderable& renderable, const VolumeTest& volume) = 0;

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
     * Submits a renderable object that is used for highlighting an object.
     * Depending on the view, this might be a coloured, transparent overlay
     * or a wireframe outline.
     */
    virtual void addHighlightRenderable(const OpenGLRenderable& renderable, const Matrix4& localToWorld) = 0;

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
};

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

    /** 
     * Front-end rendering / preparation phase. The node prepares for rendering
     * by attaching their geometry or surface data to the necessary shader instances.
     **/
    virtual void onPreRender(const VolumeTest& volume) = 0;

    // Submit renderable geometry for highlighting the object
    virtual void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) = 0;

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
