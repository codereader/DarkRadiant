#include "PatchItem.h"

#include "patch/Patch.h"
#include "PatchVertexItem.h"

namespace selection {
	namespace textool {

PatchItem::PatchItem(Patch& sourcePatch) : 
	_sourcePatch(sourcePatch)
{
	// Add all the patch control vertices as children to this class
	for (PatchControlIter i = _sourcePatch.begin(); i != _sourcePatch.end(); i++) {
		// Allocate a new vertex children on the heap
		TexToolItemPtr patchVertexItem(
			new PatchVertexItem(*i)
		);
		
		// Add it to the children of this patch
		addChild(patchVertexItem);
	}
}

AABB PatchItem::getExtents() {
	return AABB();
}

void PatchItem::render() {
	
}

void PatchItem::transform(const Matrix4& transform) {
	
}

bool PatchItem::testSelect(const float s, const float& t) {
	return false;
}

	} // namespace TexTool
} // namespace selection
