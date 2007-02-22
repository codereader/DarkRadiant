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
	virtual AABB getExtents();

	// Renders this object's visual representation.
	virtual void render();

	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix);

	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const Rectangle& rectangle);

};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHVERTEXITEM_H_*/
