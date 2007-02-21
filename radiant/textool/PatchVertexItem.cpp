#include "PatchVertexItem.h"

#include "math/aabb.h"

namespace selection {
	namespace textool {

PatchVertexItem::PatchVertexItem(PatchControl& patchControl) : 
	_patchControl(patchControl)
{}

AABB PatchVertexItem::getExtents() {
	return AABB();
}

void PatchVertexItem::render() {
	
}

void PatchVertexItem::transform(const Matrix4& transform) {
	
}

bool PatchVertexItem::testSelect(const float s, const float& t) {
	return false;
}

	} // namespace TexTool
} // namespace selection
