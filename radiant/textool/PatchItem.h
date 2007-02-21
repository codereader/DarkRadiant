#ifndef PATCHITEM_H_
#define PATCHITEM_H_

#include "patch/Patch.h"
#include "TexToolItem.h"

namespace selection {
	namespace textool {

class PatchItem :
	public TexToolItem
{
	// The list of children of this object
	TexToolItemVec _children;

	// The patch this control is referring to
	Patch& _sourcePatch;

public:
	PatchItem(Patch& sourcePatch); 

	// Gets the AABB of this object in texture space
	AABB getExtents();

	// ========== Renderable implementation ================
	
	// Renders this object's visual representation.
	void render();

	// ========== Transformable implementation ================
	
	// Transforms this object with the given transformation matrix
	void transform(const Matrix4& transform);

	// ========== Selectable implementation ================
	
	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const float s, const float& t);
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHITEM_H_*/
