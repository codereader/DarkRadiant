#ifndef RENDERABLECURVE_H_
#define RENDERABLECURVE_H_

#include <vector>
#include "irenderable.h"
#include "render.h"

namespace entity {

class RenderableCurve : 
	public OpenGLRenderable
{
public:
	std::vector<PointVertex> m_vertices;
	
	void render(const RenderInfo& info) const {
		pointvertex_gl_array(&m_vertices.front());
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_vertices.size()));
	}
};

} // namespace entity

#endif /*RENDERABLECURVE_H_*/
