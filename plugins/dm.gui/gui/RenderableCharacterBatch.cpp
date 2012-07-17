#include "RenderableCharacterBatch.h"

#include "render.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace gui
{

RenderableCharacterBatch::RenderableCharacterBatch()
{
#ifdef RENDERABLE_CHARACTER_BATCH_USE_VBO
	// Allocate a vertex buffer object
	glGenBuffersARB(1, &_vboData);

	GlobalOpenGL().assertNoErrors();
#endif
}

RenderableCharacterBatch::~RenderableCharacterBatch()
{
#ifdef RENDERABLE_CHARACTER_BATCH_USE_VBO
	glDeleteBuffersARB(1, &_vboData);
#endif
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
#ifdef RENDERABLE_CHARACTER_BATCH_USE_VBO
	// Space needed for geometry
	std::size_t dataSize = sizeof(Vertex2D) * _verts.size();

	// Initialise the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, _vboData);

	// Allocate space for vertices
	glBufferData(GL_ARRAY_BUFFER, dataSize, NULL, GL_STATIC_DRAW);

	GlobalOpenGL().assertNoErrors();

	// Upload the data
	glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, &_verts.front());

	GlobalOpenGL().assertNoErrors();

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GlobalOpenGL().assertNoErrors();
#endif
}

void RenderableCharacterBatch::render() const
{
#ifdef RENDERABLE_CHARACTER_BATCH_USE_VBO
	// Bind the VBO buffer and submit the draw call
	glBindBuffer(GL_ARRAY_BUFFER, _vboData);

	glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_DOUBLE, sizeof(Vertex2D), BUFFER_OFFSET(sizeof(double)*2));
	glVertexPointer(2, GL_DOUBLE, sizeof(Vertex2D), BUFFER_OFFSET(0));

	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_verts.size()));

	GlobalOpenGL().assertNoErrors();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
	// Regular array draw call
	glVertexPointer(2, GL_DOUBLE, sizeof(Vertex2D), &(_verts.front().vertex));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_DOUBLE, sizeof(Vertex2D), &(_verts.front().texcoord));

	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_verts.size()));

	GlobalOpenGL().assertNoErrors();
#endif
}

} // namespace
