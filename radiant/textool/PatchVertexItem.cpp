#include "PatchVertexItem.h"

#include "math/aabb.h"

namespace selection {
	namespace textool {

PatchVertexItem::PatchVertexItem(PatchControl& patchControl) : 
	_patchControl(patchControl)
{}

AABB PatchVertexItem::getExtents() {
	AABB returnValue;
	
	returnValue.origin = Vector3(
		_patchControl.m_texcoord[0], _patchControl.m_texcoord[1], 0
	);
	returnValue.extents = Vector3(0.001f, 0.001f, 0);
	
	return returnValue;
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
