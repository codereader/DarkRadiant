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
		_children.push_back(patchVertexItem);
	}
}

AABB PatchItem::getExtents() {
	AABB returnValue;
	
	// Cycle through all the children and include their AABB
	for (unsigned int i = 0; i < _children.size(); i++) {
		returnValue.includeAABB(_children[i]->getExtents());
	}
	
	return returnValue;
}

void PatchItem::render() {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->render();
	}
}

void PatchItem::transform(const Matrix4& transform) {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		_children[i]->transform(transform);
	}
}

bool PatchItem::testSelect(const Rectangle& rectangle) {
	// Cycle through all the children and ask them to render themselves
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Return true on the first selected child
		if (_children[i]->testSelect(rectangle)) {
			return true;
		}
	}
	
	// Nothing selectable, return false
	return false;
}

TexToolItemVec PatchItem::getSelectables(const Rectangle& rectangle) {
	TexToolItemVec returnVector;
	
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Return true on the first selected child
		if (_children[i]->testSelect(rectangle)) {
			returnVector.push_back(_children[i]);
		}
	}
	
	return returnVector;
}

	} // namespace TexTool
} // namespace selection
