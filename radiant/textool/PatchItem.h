#ifndef PATCHITEM_H_
#define PATCHITEM_H_

#include "patch/Patch.h"
#include "TexToolItem.h"

namespace selection {
	namespace textool {

class PatchItem :
	public TexToolItem
{
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
	void transform(const Matrix4& matrix);

	// Transforms this object if it's selected only
	void transformSelected(const Matrix4& matrix);

	// ========== Selectable implementation ================
	
	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle);
	
	/** greebo: Returns the list of selectables at the given coordinates. 
	 */
	virtual TexToolItemVec getSelectables(const Rectangle& rectangle);
	
	/** greebo: Saves the current undo state.
	 */
	virtual void beginTransformation();
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHITEM_H_*/
