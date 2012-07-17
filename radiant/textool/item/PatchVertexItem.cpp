#include "PatchVertexItem.h"

#include "math/AABB.h"
#include "math/Matrix4.h"
#include "igl.h"

namespace textool {

PatchVertexItem::PatchVertexItem(PatchControl& patchControl) :
	_patchControl(patchControl)
{}

AABB PatchVertexItem::getExtents() {
	AABB returnValue;

	returnValue.origin = Vector3(
		_patchControl.texcoord[0], _patchControl.texcoord[1], 0
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
	glVertex2d(_patchControl.texcoord[0], _patchControl.texcoord[1]);

	glEnd();
}

void PatchVertexItem::transform(const Matrix4& matrix) {
	// Pick the translation components from the matrix and apply the translation
	_patchControl.texcoord += Vector2(matrix.tx(), matrix.ty());
}

bool PatchVertexItem::testSelect(const Rectangle& rectangle) {
	return rectangle.contains(_patchControl.texcoord);
}

void PatchVertexItem::snapSelectedToGrid(float grid)
{
	if (_selected)
	{
		_patchControl.texcoord[0] = float_snapped(_patchControl.texcoord[0], grid);
		_patchControl.texcoord[1] = float_snapped(_patchControl.texcoord[1], grid);
	}
}

void PatchVertexItem::moveSelectedTo(const Vector2& targetCoords) {
	if (_selected) {
		_patchControl.texcoord = targetCoords;
	}
}

void PatchVertexItem::flipSelected(const int& axis) {
	if (_selected) {
		_patchControl.texcoord[axis] = -_patchControl.texcoord[axis];
	}
}

} // namespace TexTool
