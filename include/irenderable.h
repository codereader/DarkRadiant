#pragma once

#include <memory>

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

/**
 * \brief
 * Class which accepts OpenGLRenderable objects during the first pass of
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
    * Submit an OpenGLRenderable object for rendering using the given shader.
    *
    * \param shader
    * The Shader object this Renderable will be attached to.
    *
    * \param renderable
    * The renderable object to submit.
    *
    * \param world
    * The local to world transform that should be applied to this object when
    * it is rendered.
    *
    * \param lights
    * Optional LightSources containing lights illuminating this Renderable.
    *
    * \param entity
    * Optional IRenderEntity exposing parameters which affect the rendering of
    * this Renderable.
    */
    virtual void addRenderable(Shader& shader,
                               const OpenGLRenderable& renderable,
                               const Matrix4& world,
                               const LightSources* lights = nullptr,
                               const IRenderEntity* entity = nullptr) = 0;

    /**
     * \brief
     * Submit a renderable object to be illuminated by scene lights.
     *
     * This method allows renderable geometry to be submitted under the control
     * of a LitObject which will determine whether and how the renderable is
     * illuminated by scene lights. Each objected submitted with this method
     * will be considered for lighting by the lights which are submitted to the
     * same RenderableCollector using addLight().
     *
     * Most of the parameters have identical meanings to those in
     * addRenderable().
     *
     * \param litObject
     * A LitObject determining lighting interactions for this renderable. This
     * may or may not be the same actual object as the OpenGLRenderable,
     * depending on how the object tree is set up. If a single LitObject
     * contains multiple renderables, a separate call to this method must be
     * made for each renderable (with the same litObject parameter).
     *
     */
    virtual void addLitRenderable(Shader& shader,
                                  OpenGLRenderable& renderable,
                                  const Matrix4& localToWorld,
                                  const LitObject& litObject,
                                  const IRenderEntity* entity = nullptr) = 0;

    /**
     * \brief
     * Submit a light source for the render operation.
     *
     * This is the entry point for lights into the render front-end. Each light
     * in the scene graph must be submitted through this method in order to
     * provide light for the final render. If the render is in wireframe mode,
     * light sources can still be submitted but they will not have any effect.
     */
    virtual void addLight(const RendererLight& light) = 0;

    /**
     * \brief
     * Determine if this RenderableCollector can accept renderables for full
     * materials rendering, or just wireframe rendering.
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
            NoHighlight = 0,
            Faces       = 1 << 0, /// Highlight faces of subsequently-submitted objects, if supported
            Primitives  = 1 << 1, /// Highlight primitives of subsequently-submitted objects, if supported
            GroupMember = 1 << 2, /// Highlight as member of group, if supported
        };
    };

    virtual void setHighlightFlag(Highlight::Flags flags, bool enabled) = 0;
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
