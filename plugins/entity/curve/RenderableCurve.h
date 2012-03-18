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
	std::vector<VertexCb> m_vertices;

	void render(const RenderInfo& info) const
    {
        if (info.checkFlag(RENDER_VERTEX_COLOUR))
        {
            glEnableClientState(GL_COLOR_ARRAY);
        }
		pointvertex_gl_array(&m_vertices.front());
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_vertices.size()));
	}
};

} // namespace entity

#endif /*RENDERABLECURVE_H_*/
