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
	// in combination with the global state parameter
	void applyState(OpenGLState& current, unsigned int globalState);

	// Flush out the renderables in the vector and draw them to screen
	void flushRenderables(OpenGLState& current, 
						  unsigned int globalstate, 
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
	 * Render the contents of this bucket.
	 */
	void render(OpenGLState& current, 
				unsigned int globalstate, 
				const Vector3& viewer);
};


#endif
