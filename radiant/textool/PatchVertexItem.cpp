#include "PatchVertexItem.h"

#include "math/aabb.h"
#include "igl.h"

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
	glPointSize(5);
	glBegin(GL_POINTS);
	
	if (_selected) {
		glColor3f(1, 0.5f, 0);
	}
	else {
		glColor3f(1, 1, 1);
	}
	
	// Draw one single point at the given coords
	glVertex2f(_patchControl.m_texcoord[0], _patchControl.m_texcoord[1]);
	
	glEnd();
}

void PatchVertexItem::transformSelected(const Matrix4& matrix) {
	if (_selected) {
		transform(matrix);
	}
	// This object has no children, therefore the call needs not to be passed
}

void PatchVertexItem::transform(const Matrix4& matrix) {
	// Pick the translation components from the matrix and apply the translation
	_patchControl.m_texcoord += Vector2(matrix.tx(), matrix.ty());
}

bool PatchVertexItem::testSelect(const Rectangle& rectangle) {
	return rectangle.contains(_patchControl.m_texcoord);
}

TexToolItemVec PatchVertexItem::getSelectables(const Rectangle& rectangle) {
	// Return an empty array, this is handled by the PatchItem itself
	TexToolItemVec returnValue;
	return returnValue;
}

	} // namespace TexTool
} // namespace selection
