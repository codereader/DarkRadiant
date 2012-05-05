#pragma once

#include "math/Vector3.h"
#include "iglrender.h"

#include <vector>
#include <map>

/* FORWARD DECLS */
class Matrix4;
class OpenGLRenderable;
class RendererLight;

namespace render
{ 
    
class OpenGLShader;

/**
 * @brief A single component pass of an OpenGL shader.
 *
 * Each OpenGLShader may contain multiple passes, which are rendered
 * independently. Each pass retains its own OpenGLState and a list of renderable
 * objects to be rendered in this pass.
 */
class OpenGLShaderPass
{
	render::OpenGLShader& _owner;

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
    	const Matrix4* transform;

		// The light falling on this obejct
    	const RendererLight* light;

		// The entity attached to this renderable
		const IRenderEntity* entity;

		// Constructor
		TransformedRenderable(const OpenGLRenderable& r,
							  const Matrix4& t,
							  const RendererLight* l,
							  const IRenderEntity* e)
		: renderable(&r),
		  transform(&t),
		  light(l),
		  entity(e)
		{}
	};

	// Vector of transformed renderables using this state
	typedef std::vector<TransformedRenderable> Renderables;
	Renderables _renderablesWithoutEntity;
	
	// Renderables sorted by RenderEntity
	typedef std::map<const IRenderEntity*, Renderables> RenderablesByEntity;

	RenderablesByEntity _renderables;

private:

	// Apply own state to the "current" state object passed in as a reference,
	// in combination with the global state mask, as well as setting
    // relevant GL parameters directly.
	void applyState(OpenGLState& current,
                    unsigned int globalStateMask,
                    const Vector3& viewer,
					std::size_t time,
					const IRenderEntity* entity);

	// Returns true if the stage associated to this pass is active and should be rendered
	bool stateIsActive();

	void setupTextureMatrix(GLenum textureUnit, const ShaderLayerPtr& stage);

	// Render all of the given TransformedRenderables
	void renderAllContained(const Renderables& renderables,
							OpenGLState& current,
						    const Vector3& viewer,
							std::size_t time);

    /* Helper functions to enable/disable particular GL states */

    void setTexture0();

    void enableTexture2D();
    void disableTexture2D();

    void enableTextureCubeMap();
    void disableTextureCubeMap();

    void enableRenderBlend();
    void disableRenderBlend();

    // Apply all OpenGLState textures to texture units
    void applyAllTextures(OpenGLState& current, unsigned requiredState);

    // Set up the cube map texture matrix if necessary
    void setUpCubeMapAndTexGen(OpenGLState& current,
                               unsigned requiredState,
                               const Vector3& viewer);

    // Set up lighting calculation
    void setUpLightingCalculation(OpenGLState& current,
                                  const RendererLight* light,
                                  const Vector3& viewer,
                                  const Matrix4& objTransform,
								  std::size_t time);

public:

	OpenGLShaderPass(render::OpenGLShader& owner) :
		_owner(owner)
	{}

	/**
	 * Add a renderable to this state bucket with the given object transform
	 * matrix and light.
	 */
	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview,
					   const RendererLight* light = 0);

	/**
	 * Add a renderable to this state bucket with the given object transform
	 * matrix and light.
	 */
	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview,
					   const IRenderEntity& entity,
					   const RendererLight* light = 0);

	/**
	 * Return the OpenGL state associated with this bucket.
	 */
	OpenGLState& state()
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
     * \param current
     * The current OpenGL state variables.
     *
     * \param flagsMask
     * Mask of allowed render flags.
     *
     * \param viewer
     * Viewer location in world space.
     *
     */
	void render(OpenGLState& current,
				unsigned int flagsMask,
				const Vector3& viewer,
				std::size_t time);

	/**
	 * Returns true if this shaderpass doesn't have anything to render.
	 */
	bool empty() const
	{
		return _renderables.empty() && _renderablesWithoutEntity.empty();
	}

	friend std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self);
};

typedef boost::shared_ptr<OpenGLShaderPass> OpenGLShaderPassPtr;

// Stream insertion operator
std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self);

} // namespace render
