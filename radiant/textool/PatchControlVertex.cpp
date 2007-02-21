#include "PatchControlVertex.h"

#include "math/aabb.h"

namespace selection {
	namespace textool {

// Adds the given TexToolItem as child of this object
void PatchControlVertex::addChild(TexToolItemPtr child) {
	_children.push_back(child);
}

/** greebo: Returns the vector reference of this object's children.
 */
TexToolItemVec& PatchControlVertex::getChildren() {
	return _children;
}

// Gets the AABB of this object in texture space
AABB PatchControlVertex::getExtents() {
	return AABB();
}

// ========== Renderable implementation ================

// Renders this object's visual representation.
void PatchControlVertex::render() {
	
}

// ========== Transformable implementation ================

// Transforms this object with the given transformation matrix
void PatchControlVertex::transform(const Matrix4& transform) {
	
}

// ========== Selectable implementation ================

/** greebo: Returns true if the object can be selected at the given coords.
 */
bool PatchControlVertex::testSelect(const float s, const float& t) {
	return false;
}

	} // namespace TexTool
} // namespace selection
