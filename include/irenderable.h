#pragma once

#include <memory>
#include "math/Vector3.h"

class MeshVertex;
class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

class OpenGLRenderable;
class Matrix4;
class IRenderEntity;
class RendererLight;
class Renderable;
class VolumeTest;

/**
 * \brief Class used during the front-end render pass.
 * 
 * Each node in the scenegraph is visited and is asked to get ready for rendering
 * by calling Renderable.onPreRender() method.
 * 
 * If a node's highlight flags (which combined with their parents flags) indicates
 * that the node needs to render special visual aids like selection overlays,
 * the node's Renderable::renderHighlights() method is invoked.
 */
class IRenderableCollector
{
public:
    virtual ~IRenderableCollector() {}

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

    virtual bool hasHighlightFlag(Highlight::Flags flags) const = 0;

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
