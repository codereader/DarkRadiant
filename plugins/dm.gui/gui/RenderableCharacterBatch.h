#ifndef RenderableCharacterBatch_h__
#define RenderableCharacterBatch_h__

#include "igl.h"
#include "irender.h"
#include <memory>

#include "TextParts.h"

// Define this to use VBOs instead of regular vertex arrays
//#define RENDERABLE_CHARACTER_BATCH_USE_VBO 1

namespace gui
{

/**
 * A container holding the render information of a bunch of characters,
 * all referencing the same font texture. The geometry of the characters
 * is arranged into a vertex buffer object on compile().
 */
class RenderableCharacterBatch
{
private:
	// The local vertex buffer
	typedef std::vector<Vertex2D> Vertices;
	Vertices _verts;

#ifdef RENDERABLE_CHARACTER_BATCH_USE_VBO
	// Vertex buffer objects
	GLuint _vboData;
#endif

public:
	RenderableCharacterBatch();

	~RenderableCharacterBatch();

	// Add a single glyph to this batch
	void addGlyph(const TextChar& ch);

	void compile();

	void render() const;
};
typedef std::shared_ptr<RenderableCharacterBatch> RenderableCharacterBatchPtr;

} // namespace

#endif // RenderableCharacterBatch_h__
