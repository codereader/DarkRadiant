#ifndef RENDERABLEWIREFRAME_H_
#define RENDERABLEWIREFRAME_H_

#include "render.h"

struct EdgeRenderIndices
{
	RenderIndex first;
	RenderIndex second;

	EdgeRenderIndices()
		: first(0), second(0)
	{}

	EdgeRenderIndices(const RenderIndex _first, const RenderIndex _second)
		: first(_first), second(_second)
	{}
};

#if 0
class RenderableWireframe :
	public OpenGLRenderable
{
public:
	void render(const RenderInfo& info) const
	{
		if (m_size == 0) return;

        if (info.checkFlag(RENDER_VERTEX_COLOUR))
        {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(
                4, GL_UNSIGNED_BYTE, sizeof(VertexCb), &m_vertices->colour
            );
        }

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexCb), &m_vertices->vertex);
		glDrawElements(GL_LINES, GLsizei(m_size<<1), RenderIndexTypeID, &m_faceVertex.front());

		if (info.checkFlag(RENDER_VERTEX_COLOUR))
		{
			glDisableClientState(GL_COLOR_ARRAY);
		}
	}

	std::vector<EdgeRenderIndices> m_faceVertex;
	std::size_t m_size;
	const VertexCb* m_vertices;

	virtual ~RenderableWireframe() {}
};
#endif

#endif /*RENDERABLEWIREFRAME_H_*/
