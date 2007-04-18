#include "TexDef.h"

#include <math.h>
#include "texturelib.h"
#include "math/Vector2.h"

// Constructor
TexDef::TexDef() {
	_shift[0] = 0;
	_shift[1] = 0;
	_rotate = 0;
	_scale[0] = 1;
	_scale[1] = 1;
}

// Constructs a TexDef out of the given transformation matrix plus width/height
TexDef::TexDef(double width, double height, const Matrix4& transform) {
	_scale[0] = static_cast<double>((1.0 / Vector2(transform[0], transform[4]).getLength()) / width);
	_scale[1] = static_cast<double>((1.0 / Vector2(transform[1], transform[5]).getLength()) / height);

	_rotate = static_cast<double>(-radians_to_degrees(arctangent_yx(-transform[4], transform[0])));

	if (_rotate == -180.0f) {
		_rotate = 180.0f;
	}

	_shift[0] = transform[12] * width;
	_shift[1] = transform[13] * height;

	// If the 2d cross-product of the x and y axes is positive, one of the axes has a negative scale.
	if (Vector2(transform[0], transform[4]).crossProduct(Vector2(transform[1], transform[5])) > 0) {
		if (_rotate >= 180.0f) {
			_rotate -= 180.0f;
			_scale[0] = -_scale[0];
		}
		else {
			_scale[1] = -_scale[1];
		}
	}
}

void TexDef::shift(double s, double t) {
	_shift[0] += s;
	_shift[1] += t;
}

void TexDef::scale(double s, double t) {
	_scale[0] += s;
	_scale[1] += t;
}

void TexDef::rotate(double angle) {
	_rotate += angle;
	_rotate = static_cast<double>(float_to_integer(_rotate) % 360);
}

// Checks the TexDef for insanely large values
bool TexDef::isSane() const {
	return fabs(_shift[0]) < (1 << 16)
		&& fabs(_shift[1]) < (1 << 16);
}

// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
// This function normalises shift values to the smallest positive congruent values.
void TexDef::normalise(double width, double height) {
	// it may be useful to also normalise the rotation here, if this function is used elsewhere.
	_shift[0] = float_mod(_shift[0], width);
	_shift[1] = float_mod(_shift[1], height);
}
	
/* Construct a transform in ST space from the texdef.
 * Transforms constructed from quake's texdef format 
 * are (-shift)*(1/scale)*(-rotate) with x translation sign flipped.
 * This would really make more sense if it was inverseof(shift*rotate*scale).. oh well.*/
Matrix4 TexDef::getTransform(double width, double height) const {
	Matrix4 transform;
	double inverse_scale[2];
  
		// transform to texdef shift/scale/rotate
	inverse_scale[0] = 1 / (_scale[0] * width);
	inverse_scale[1] = 1 / (_scale[1] * -height);
	transform[12] = _shift[0] / width;
	transform[13] = -_shift[1] / -height;
	
	double c = cos(degrees_to_radians(-_rotate));
	double s = sin(degrees_to_radians(-_rotate));
	
	transform[0] = c * inverse_scale[0];
	transform[1] = s * inverse_scale[1];
	transform[4] = -s * inverse_scale[0];
	transform[5] = c * inverse_scale[1];
	transform[2] = transform[3] = transform[6] = transform[7] = transform[8] = transform[9] = transform[11] = transform[14] = 0;
	transform[10] = transform[15] = 1;
	
	return transform;
}
