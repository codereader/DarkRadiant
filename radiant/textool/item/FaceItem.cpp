#include "FaceItem.h"

#include "math/FloatTools.h"
#include "brush/Face.h"
#include "brush/Winding.h"

namespace textool {

FaceItem::FaceItem(Face& sourceFace) : 
	_sourceFace(sourceFace),
	_winding(sourceFace.getWinding())
{}

AABB FaceItem::getExtents() {
	AABB returnValue;
	
	for (Winding::iterator i = _winding.begin(); i != _winding.end(); i++) {
		returnValue.includePoint(Vector3(i->texcoord[0], i->texcoord[1], 0));
	}
	
	return returnValue;
}

void FaceItem::render() {
	glEnable(GL_BLEND);
	glBlendColor(0,0,0, 0.3f);
	glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
	
	if (_selected) {
		glColor3f(1, 0.5f, 0);
	}
	else {
		glColor3f(1, 1, 1);
	}
	
	glBegin(GL_TRIANGLE_FAN);
	
	for (Winding::iterator i = _winding.begin(); i != _winding.end(); i++) {
		glVertex2f(i->texcoord[0], i->texcoord[1]);
	}
	
	glEnd();
	glDisable(GL_BLEND);
	
	glPointSize(5);
	glBegin(GL_POINTS);
	for (Winding::iterator i = _winding.begin(); i != _winding.end(); i++) {
		glVertex2f(i->texcoord[0], i->texcoord[1]);
	}
	
	glColor3f(1, 1, 1);
	Vector2 centroid = getCentroid();
	glVertex2f(centroid[0], centroid[1]);
	
	glEnd();
}

void FaceItem::transform(const Matrix4& matrix) {
	// Pick the translation components from the matrix and apply the translation
	Vector2 translation(matrix.tx(), matrix.ty());
	
	// Scale the translation with the shader image dimensions
	translation[0] *= _sourceFace.getShader().width();
	translation[1] *= _sourceFace.getShader().height();
	
	// Invert the s-translation, the ShiftTexDef does it inversely for some reason. 
	translation[0] *= -1;
	
	// Shift the texdef accordingly
	_sourceFace.ShiftTexdef(translation[0], translation[1]);
}

Vector2 FaceItem::getCentroid() const {
	Vector2 texCentroid;
	 
	for (Winding::iterator i = _winding.begin(); i != _winding.end(); i++) {
		texCentroid += i->texcoord;
	}
	
	// Take the average value of all the winding texcoords to retrieve the centroid
	texCentroid /= _winding.numpoints;
	
	return texCentroid;
}

bool FaceItem::testSelect(const Rectangle& rectangle) {
	// Check if the centroid is within the rectangle
	return rectangle.contains(getCentroid());
}

void FaceItem::snapSelectedToGrid(float grid) {
	if (_selected) {
		Vector2 centroid = getCentroid();
		
		Vector2 snapped(
			float_snapped(centroid[0], grid), 
			float_snapped(centroid[1], grid)
		);
		
		Vector3 translation;
		translation[0] = snapped[0] - centroid[0];
		translation[1] = snapped[1] - centroid[1];
		
		Matrix4 matrix = Matrix4::getTranslation(translation);
		
		// Do the transformation
		transform(matrix);
	}
}

void FaceItem::flipSelected(const int& axis) {
	if (_selected) {
		_sourceFace.flipTexture(axis);
	}
}

} // namespace TexTool
