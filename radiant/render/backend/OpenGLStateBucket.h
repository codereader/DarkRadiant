#ifndef OPENGLSTATEBUCKET_H_
#define OPENGLSTATEBUCKET_H_

#include "math/Vector3.h"
#include "iglrender.h"

#include <vector>

/* FORWARD DECLS */
class Matrix4;
class OpenGLRenderable;
class RendererLight;

/// \brief A container of Renderable references.
/// May contain the same Renderable multiple times, with different transforms.
class OpenGLStateBucket
{
private:
  struct RenderTransform
  {
    const Matrix4* m_transform;
    const OpenGLRenderable *m_renderable;
    const RendererLight* m_light;

    RenderTransform(const OpenGLRenderable& renderable, const Matrix4& transform, const RendererLight* light)
      : m_transform(&transform), m_renderable(&renderable), m_light(light)
    {
    }
  };

  typedef std::vector<RenderTransform> Renderables;

private:

	OpenGLState m_state;
	Renderables m_renderables;

private:

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
					   const RendererLight* light = 0) 
	{
    	m_renderables.push_back(RenderTransform(renderable, modelview, light));
	}

	/**
	 * Return the OpenGL state associated with this bucket.
	 */
	OpenGLState& state() {
		return m_state;
	}

	/**
	 * Render the contents of this bucket.
	 */
	void render(OpenGLState& current, 
				unsigned int globalstate, 
				const Vector3& viewer);
};


#endif /*OPENGLSTATEBUCKET_H_*/
