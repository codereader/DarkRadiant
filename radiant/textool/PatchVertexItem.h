#ifndef PATCHVERTEXITEM_H_
#define PATCHVERTEXITEM_H_

#include "TexToolItem.h"
#include "ipatch.h"

namespace selection {
	namespace textool {

class PatchVertexItem :
	public TexToolItem
{
	// The reference to the "real" PatchControlVertex
	PatchControl& _patchControl;

public:
	PatchVertexItem(PatchControl& patchControl);

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
	
	/** greebo: Returns the list of possible TexToolItems that can be
	 * selected within the given rectangle.
	 */
	virtual TexToolItemVec getSelectables(const Rectangle& rectangle);
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHVERTEXITEM_H_*/
