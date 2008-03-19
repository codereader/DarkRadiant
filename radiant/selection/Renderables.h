#ifndef RENDERABLES_H_
#define RENDERABLES_H_

#include "irenderable.h"

/* greebo: This contains the renderables (rectangles, arrows, circles, semicircles) to represent
 * the manipulators of the selected items
 */

// helper class for rendering a circle
struct RenderableCircle : public OpenGLRenderable {
	Array<PointVertex> _vertices;

	RenderableCircle(std::size_t size) : _vertices(size) {}
    
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &_vertices.data()->colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &_vertices.data()->vertex);
		glDrawArrays(GL_LINE_LOOP, 0, GLsizei(_vertices.size()));
	}
	
    void setColour(const Colour4b& colour) {
		for (Array<PointVertex>::iterator i = _vertices.begin(); i != _vertices.end(); ++i) {
			(*i).colour = colour;
		}
	}
};

// helper class for rendering a semi-circle
struct RenderableSemiCircle : public OpenGLRenderable {
	Array<PointVertex> _vertices;

	RenderableSemiCircle(std::size_t size) : _vertices(size) {}
    
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &_vertices.data()->colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &_vertices.data()->vertex);
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(_vertices.size()));
	}
	
	void setColour(const Colour4b& colour) {
		for (Array<PointVertex>::iterator i = _vertices.begin(); i != _vertices.end(); ++i) {
			(*i).colour = colour;
		}
	}
};

// Helper class for rendering an arrow
struct RenderableArrowLine : public OpenGLRenderable {
	PointVertex _line[2];

	RenderableArrowLine() {}
    
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &_line[0].colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &_line[0].vertex);
		glDrawArrays(GL_LINES, 0, 2);
	}
	
	void setColour(const Colour4b& colour) {
		_line[0].colour = colour;
		_line[1].colour = colour;
	}
};
  
// Helper class for rendering an arrow
struct RenderableArrowHead : public OpenGLRenderable  {
	Array<FlatShadedVertex> _vertices;

	RenderableArrowHead(std::size_t size): _vertices(size) {}
    
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(FlatShadedVertex), &_vertices.data()->colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(FlatShadedVertex), &_vertices.data()->vertex);
		glNormalPointer(GL_DOUBLE, sizeof(FlatShadedVertex), &_vertices.data()->normal);
		glDrawArrays(GL_TRIANGLES, 0, GLsizei(_vertices.size()));
	}
	
	void setColour(const Colour4b& colour) {
		for(Array<FlatShadedVertex>::iterator i = _vertices.begin(); i != _vertices.end(); ++i) {
			(*i).colour = colour;
		}
	}
};

// Helper class for rendering a quadratic
struct RenderableQuad : public OpenGLRenderable {
	PointVertex _quad[4];
	
	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &_quad[0].colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &_quad[0].vertex);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}

	void setColour(const Colour4b& colour) {
		_quad[0].colour = colour;
		_quad[1].colour = colour;
		_quad[2].colour = colour;
		_quad[3].colour = colour;
	}
};

// Helper class for rendering an arrow
struct RenderableArrow : public OpenGLRenderable {
	PointVertex _line[2];

	void render(RenderStateFlags state) const {
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &_line[0].colour);
		glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &_line[0].vertex);
		glDrawArrays(GL_LINES, 0, 2);
	}
	
	void setColour(const Colour4b& colour) {
		_line[0].colour = colour;
		_line[1].colour = colour;
	}
};

#endif /*RENDERABLES_H_*/
