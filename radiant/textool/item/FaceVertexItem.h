#ifndef FACE_VERTEX_ITEM_H_
#define FACE_VERTEX_ITEM_H_

#include "brush/Face.h"
#include "math/aabb.h"
#include "textool/TexToolItem.h"

namespace textool
{

class FaceVertexItem :
	public TexToolItem
{
	// The face this control is referring to
	Face& _sourceFace;

	WindingVertex& _windingVertex;

	// Saved texture coords
	Vector2 _saved;

public:
	// Constructor, allocates all child FacItems
	FaceVertexItem(Face& sourceFace, WindingVertex& windingVertex); 

    // destructor
	virtual ~FaceVertexItem() {}

	/** 
	 * greebo: Saves the undoMemento of this face,
	 * so that the operation can be undone later.
	 */
	virtual void beginTransformation();

	virtual void transform(const Matrix4& matrix);

	virtual bool testSelect(const Rectangle& rectangle);

	// RenderableItem implementation
	virtual void render();

private:
	Vector2 getTexCentroid();

	// Returns the bounds in UV space of the whole winding
	AABB getTexAABB();
}; 
	
} // namespace textool

#endif /* FACE_VERTEX_ITEM_H_ */
