#include "RenderableCharacterBatch.h"

#include "render.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace gui
{

RenderableCharacterBatch::RenderableCharacterBatch()
{
	// Allocate a vertex buffer object
	glGenBuffersARB(1, &_vboData);

	GlobalOpenGL_debugAssertNoErrors();
}

RenderableCharacterBatch::~RenderableCharacterBatch()
{
	glDeleteBuffersARB(1, &_vboData);
}

void RenderableCharacterBatch::addGlyph(const TextChar& ch)
{
	_verts.push_back(ch.coords[0]);
	_verts.push_back(ch.coords[1]);
	_verts.push_back(ch.coords[2]);
	_verts.push_back(ch.coords[3]);
}

void RenderableCharacterBatch::compile()
{
	// Space needed for geometry
	std::size_t dataSize = sizeof(Vertex2D) * _verts.size();
	
	// Initialise the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vboData);

	// Allocate space for vertices
	glBufferData(GL_ARRAY_BUFFER, dataSize, NULL, GL_STATIC_DRAW);

	GlobalOpenGL_debugAssertNoErrors();

	// Upload the data
	glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, &_verts.front());

	GlobalOpenGL_debugAssertNoErrors();

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GlobalOpenGL_debugAssertNoErrors();
}

void RenderableCharacterBatch::render() const
{
	glBindBuffer(GL_ARRAY_BUFFER, _vboData);
	
	glTexCoordPointer(2, GL_DOUBLE, sizeof(Vertex2D), BUFFER_OFFSET(sizeof(double)*2));
	glVertexPointer(2, GL_DOUBLE, sizeof(Vertex2D), BUFFER_OFFSET(0));
		
	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_verts.size()));

	GlobalOpenGL_debugAssertNoErrors();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace
