#ifndef RENDERABLEWIREFRAME_H_
#define RENDERABLEWIREFRAME_H_

#include "render.h"

struct EdgeRenderIndices {
	RenderIndex first;
	RenderIndex second;

	EdgeRenderIndices()
		: first(0), second(0)
	{}
	
	EdgeRenderIndices(const RenderIndex _first, const RenderIndex _second)
		: first(_first), second(_second)
	{}
};

class RenderableWireframe : 
	public OpenGLRenderable 
{
public:
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_vertices->colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &m_vertices->vertex);
		glDrawElements(GL_LINES, GLsizei(m_size<<1), RenderIndexTypeID, m_faceVertex.data());
	}

	Array<EdgeRenderIndices> m_faceVertex;
	std::size_t m_size;
	const PointVertex* m_vertices;
};

#endif /*RENDERABLEWIREFRAME_H_*/
