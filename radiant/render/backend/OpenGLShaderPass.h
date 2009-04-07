#ifndef OPENGLSHADERPASS_H_
#define OPENGLSHADERPASS_H_

#include "math/Vector3.h"
#include "iglrender.h"

#include <vector>

/* FORWARD DECLS */
class Matrix4;
class OpenGLRenderable;
class RendererLight;

/**
 * @brief A single component pass of an OpenGL shader.
 *
 * Each OpenGLShader may contain multiple passes, which are rendered
 * independently. Each pass retains its own OpenGLState and a list of renderable
 * objects to be rendered in this pass.
 */
class OpenGLShaderPass
{
	// The state applied to this bucket
	OpenGLState _state;
	
	/*
	 * Representation of a transformed-and-lit renderable object. Stores a 
	 * single object, with its transform matrix and illuminating light source.
	 */
	struct TransformedRenderable {
    
    	// The renderable object
    	const OpenGLRenderable* renderable;

    	// The modelview transform for this renderable
    	const Matrix4* transform;

		// The light falling on this obejct    	
    	const RendererLight* light;

		// Default constructor
		TransformedRenderable(const OpenGLRenderable& r, 
							  const Matrix4& t, 
							  const RendererLight* l)
		: renderable(&r),
		  transform(&t),		   
		  light(l)
		{ }
	};

	// Vector of transformed renderables using this state
	typedef std::vector<TransformedRenderable> Renderables;
	Renderables _renderables;

private:

	// Apply own state to the "current" state object passed in as a reference,
	// in combination with the global state mask, as well as setting
    // relevant GL parameters directly.
	void applyState(OpenGLState& current,
                    unsigned int globalStateMask,
                    const Vector3& viewer);

	// Render all of our contained TransformedRenderables
	void renderAllContained(OpenGLState& current, 
						    const Vector3& viewer);

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

public:

	/**
	 * Add a renderable to this state bucket with the given object transform
	 * matrix and light.
	 */
	void addRenderable(const OpenGLRenderable& renderable, 
					   const Matrix4& modelview, 
					   const RendererLight* light = 0);

	/**
	 * Return the OpenGL state associated with this bucket.
	 */
	OpenGLState& state() {
		return _state;
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
				const Vector3& viewer);
};


#endif
