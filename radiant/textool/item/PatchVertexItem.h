#ifndef PATCHVERTEXITEM_H_
#define PATCHVERTEXITEM_H_

#include "textool/TexToolItem.h"
#include "ipatch.h"

namespace textool {

class PatchVertexItem :
	public TexToolItem
{
	// The reference to the "real" PatchControlVertex
	PatchControl& _patchControl;

public:
	PatchVertexItem(PatchControl& patchControl);

	// Gets the AABB of this object in texture space
	virtual AABB getExtents();

	// Renders this object's visual representation.
	virtual void render();

	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix);

	/** greebo: Snaps this patch texcoord to the grid.
	 */
	virtual void snapSelectedToGrid(float grid);

	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle);
	
	/** greebo: Moves the selected patch items to the specified coords.
	 */
	virtual void moveSelectedTo(const Vector2& targetCoords);

	/** greebo: Flips the control vertex (if selected) 
	 * 			about the given axis in texture space.
	 * 
	 *  @axis:  0 = s-axis flip, 1 = t-axis flip 			
	 */
	virtual void flipSelected(const int& axis);
};
	
} // namespace TexTool

#endif /*PATCHVERTEXITEM_H_*/
