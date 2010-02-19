#ifndef RenderableCharacterBatch_h__
#define RenderableCharacterBatch_h__

#include "igl.h"
#include "irender.h"
#include <boost/shared_ptr.hpp>

#include "TextParts.h"

namespace gui
{

/**
 * A container holding the render information of a bunch of characters,
 * all referencing the same font texture. The geometry of the characters
 * is arranged into a vertex buffer object on compile().
 */
class RenderableCharacterBatch :
	public OpenGLRenderable
{
private:
	// The local vertex buffer
	typedef std::vector<Vertex2D> Vertices;
	Vertices _verts;

	// Vertex buffer objects
	GLuint _vboData;

public:
	RenderableCharacterBatch();

	~RenderableCharacterBatch();

	// Add a single glyph to this batch
	void addGlyph(const TextChar& ch);

	void compile();

	// OpenGLRenderable implementation
	void render(const RenderInfo& info) const;
};
typedef boost::shared_ptr<RenderableCharacterBatch> RenderableCharacterBatchPtr;

} // namespace

#endif // RenderableCharacterBatch_h__
