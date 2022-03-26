#pragma once

#include "math/Matrix4.h"
#include "OpenGLState.h"

#include <vector>

/* FORWARD DECLS */
class OpenGLRenderable;
class RendererLight;

namespace render
{

class OpenGLShader;
class InteractionProgram;

/**
 * @brief A single component pass of an OpenGL shader.
 *
 * Each OpenGLShader may contain multiple passes, which are rendered
 * independently. Each pass retains its own OpenGLState and a list of renderable
 * objects to be rendered in this pass.
 */
class OpenGLShaderPass
{
protected:
	OpenGLShader& _owner;

	// The state applied to this bucket
	OpenGLState _glState;

	/*
	 * Representation of a transformed-and-lit renderable object. Stores a
	 * single object, with its transform matrix and illuminating light source.
	 */
	struct TransformedRenderable
	{
    	// The renderable object
    	const OpenGLRenderable* renderable;

    	// The modelview transform for this renderable
    	const Matrix4 transform;

		// Constructor
		TransformedRenderable(const OpenGLRenderable& r,
							  const Matrix4& t)
		: renderable(&r),
		  transform(t)
		{}
	};

	// Vector of transformed renderables using this state
    std::vector<TransformedRenderable> _transformedRenderables;

protected:

	// Render all of the given TransformedRenderables
	void drawRenderables(OpenGLState& current);

public:

	OpenGLShaderPass(OpenGLShader& owner) :
		_owner(owner)
	{}

    OpenGLShader& getShader()
    {
        return _owner;
    }

    // Returns true if the stage associated to this pass is active and should be rendered
    bool stateIsActive();

	/**
     * \brief
     * Add a renderable to this shader pass the given object transform matrix.
	 */
	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& modelview);

	/**
	 * Return the OpenGL state associated with this bucket.
	 */
	OpenGLState& state()
    {
		return _glState;
	}

	const OpenGLState& state() const
	{
		return _glState;
	}

	OpenGLState* statePtr()
	{
		return &_glState;
	}

	/**
	 * \brief
     * Render the renderables attached to this shader pass.
     *
     * \param view
     * The render view used to cull surfaces
     */
	void submitSurfaces(const VolumeTest& view);

    /**
     * \brief
     * Render the renderables attached to this shader pass.
     * Their geometry might not be stored in the central buffer objects.
     */
    void submitRenderables(OpenGLState& current);

	/**
	 * Returns true if this shaderpass doesn't have anything to render.
	 */
    bool empty();

    // True if this shader pass has OpenGLRenderables attached
    bool hasRenderables() const;

    // Clear out all renderable references accumulated during this frame
    void clearRenderables();

    // Whether this shader pass is suitable for the give view type
    bool isApplicableTo(RenderViewType renderViewType) const;

    // Evaluates all stages and invokes applyState
    void evaluateStagesAndApplyState(OpenGLState& current, unsigned int globalStateMask,
        std::size_t time, const IRenderEntity* entity);

    // Apply own state to the "current" state object passed in as a reference,
    // in combination with the global state mask, as well as setting
    // relevant GL parameters directly.
    void applyState(OpenGLState& current, unsigned int globalStateMask);

    // Evaluates the time- and entity-dependent expressions in the shader stages
    void evaluateShaderStages(std::size_t time, const IRenderEntity* entity);

	friend std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self);
};

typedef std::shared_ptr<OpenGLShaderPass> OpenGLShaderPassPtr;

// Stream insertion operator
std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self);

} // namespace render
