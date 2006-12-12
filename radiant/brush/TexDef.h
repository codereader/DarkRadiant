#ifndef TEXDEF_H_
#define TEXDEF_H_

#include "itexdef.h"
#include <math.h>
#include "math/Vector2.h"
#include "math/matrix.h"

// greebo: Helper function
// handles degenerate cases, just in case library atan2 doesn't
inline double arctangent_yx(double y, double x) {
	if(fabs(x) > 1.0E-6) {
		return atan2(y, x);
	}
	else if(y > 0) {
		return c_half_pi;
	}
	else {
		return -c_half_pi;
	}
}

class TexDef : public GenericTextureDefinition {
public:
	// Constructor
	TexDef() {
		shift[0] = 0;
		shift[1] = 0;
		rotate = 0;
		scale[0] = 1;
		scale[1] = 1;
	}
	
	// Constructs a TexDef out of the given transformation matrix plus width/height
	TexDef(float width, float height, const Matrix4& transform) {
		scale[0] = static_cast<float>((1.0 / Vector2(transform[0], transform[4]).getLength()) / width);
		scale[1] = static_cast<float>((1.0 / Vector2(transform[1], transform[5]).getLength()) / height);

		rotate = static_cast<float>(-radians_to_degrees(arctangent_yx(-transform[4], transform[0])));

		if (rotate == -180.0f) {
			rotate = 180.0f;
		}

		shift[0] = transform[12] * width;
		shift[1] = transform[13] * height;

		// If the 2d cross-product of the x and y axes is positive, one of the axes has a negative scale.
		if (Vector2(transform[0], transform[4]).crossProduct(Vector2(transform[1], transform[5])) > 0) {
			if (rotate >= 180.0f) {
				rotate -= 180.0f;
				scale[0] = -scale[0];
			}
			else {
				scale[1] = -scale[1];
			}
		}
	}

	bool isSane() const {
		return fabs(shift[0]) < (1 << 16)
			&& fabs(shift[1]) < (1 << 16);
	}
	
	/* Construct a transform in ST space from the texdef.
	 * Transforms constructed from quake's texdef format 
	 * are (-shift)*(1/scale)*(-rotate) with x translation sign flipped.
	 * This would really make more sense if it was inverseof(shift*rotate*scale).. oh well.*/
	Matrix4 getTransform(float width, float height) const {
		Matrix4 transform;
		double inverse_scale[2];
  
		// transform to texdef shift/scale/rotate
		inverse_scale[0] = 1 / (scale[0] * width);
		inverse_scale[1] = 1 / (scale[1] * -height);
		transform[12] = shift[0] / width;
		transform[13] = -shift[1] / -height;
		
		double c = cos(degrees_to_radians(-rotate));
		double s = sin(degrees_to_radians(-rotate));
		
		transform[0] = static_cast<float>(c * inverse_scale[0]);
		transform[1] = static_cast<float>(s * inverse_scale[1]);
		transform[4] = static_cast<float>(-s * inverse_scale[0]);
		transform[5] = static_cast<float>(c * inverse_scale[1]);
		transform[2] = transform[3] = transform[6] = transform[7] = transform[8] = transform[9] = transform[11] = transform[14] = 0;
		transform[10] = transform[15] = 1;
		
		return transform;
	}
};

#endif /*TEXDEF_H_*/
