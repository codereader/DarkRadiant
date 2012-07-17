#pragma once

#include "render.h"
#include "render/VertexNCb.h"

/* greebo: This contains the renderables (rectangles, arrows, circles, semicircles) to represent
 * the manipulators of the selected items
 */

// helper class for rendering a circle
class RenderableCircle :
	public RenderablePointVector
{
public:
	// Pass the amount of points to render
	RenderableCircle(std::size_t size) :
		RenderablePointVector(GL_LINE_LOOP, size)
	{}
};

// helper class for rendering a semi-circle
class RenderableSemiCircle :
	public RenderablePointVector
{
public:
	// Pass the amount of points to render
	RenderableSemiCircle(std::size_t size) :
		RenderablePointVector(GL_LINE_STRIP, size)
	{}
};

// Helper class for rendering an arrow (only the line part)
class RenderableArrowLine :
	public RenderablePointVector
{
public:
	// Constructor instantiates a renderable array of size 2
	RenderableArrowLine() :
		RenderablePointVector(GL_LINES, 2)
	{}
};

// Helper class for rendering an arrow (only the head part)
class RenderableArrowHead :
	public OpenGLRenderable
{
public:
	typedef std::vector<VertexNCb> FlatShadedVertices;
	FlatShadedVertices _vertices;

	RenderableArrowHead(std::size_t size) :
		_vertices(size)
	{}

	void render(const RenderInfo& info) const
	{
        glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexNCb), &_vertices.front().colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(VertexNCb), &_vertices.front().vertex);
		glNormalPointer(GL_DOUBLE, sizeof(VertexNCb), &_vertices.front().normal);
		glDrawArrays(GL_TRIANGLES, 0, GLsizei(_vertices.size()));
	}

	void setColour(const Colour4b& colour)
	{
		for (FlatShadedVertices::iterator i = _vertices.begin(); i != _vertices.end(); ++i)
		{
			i->colour = colour;
		}
	}
};

// Helper class for rendering a quadratic
class RenderableQuad :
	public RenderablePointVector
{
public:
	RenderableQuad() :
		RenderablePointVector(GL_LINE_LOOP, 4)
	{}
};
