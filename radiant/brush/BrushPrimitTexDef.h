#ifndef BRUSHTEXTUREDEFINTION_H_
#define BRUSHTEXTUREDEFINTION_H_

#include "math/matrix.h"
#include "brush/TexDef.h"

struct BrushPrimitTexDef {
	float coords[2][3];	
	
	// Constructor
	BrushPrimitTexDef() {
		coords[0][0] = 2.0f;
		coords[0][1] = 0.f;
		coords[0][2] = 0.f;
		coords[1][0] = 0.f;
		coords[1][1] = 2.0f;
		coords[1][2] = 0.f;
	}
	
	// Construct the BP Definition out of the transformation matrix
	// Basically copies over the values from the according components
	BrushPrimitTexDef(const Matrix4& transform) {
		coords[0][0] = transform.xx();
		coords[0][1] = transform.yx();
		coords[0][2] = transform.tx();
		coords[1][0] = transform.xy();
		coords[1][1] = transform.yy();
		coords[1][2] = transform.ty();
	}
	
	/* greebo: This removes the texture scaling from the
	 * coordinates. The resulting coordinates are absolute
	 * values within the shader image.
	 * 
	 * An 128x256 texture with scaled coordinates 0.5,0.5
	 * would be translated into the coordinates 64,128,
	 * pointing to a defined pixel within the texture image.
	 */
	void removeScale(std::size_t width, std::size_t height) {
		coords[0][0] *= width;
		coords[0][1] *= width;
		coords[0][2] *= width;
		coords[1][0] *= height;
		coords[1][1] *= height;
		coords[1][2] *= height;
	}
	
	/* greebo: this converts absolute coordinates into
	 * relative ones, where everything is measured
	 * in multiples of the texture x/y dimensions. */
	void addScale(std::size_t width, std::size_t height) {
		coords[0][0] /= width;
		coords[0][1] /= width;
		coords[0][2] /= width;
		coords[1][0] /= height;
		coords[1][1] /= height;
		coords[1][2] /= height;
	}
	
	// compute a fake shift scale rot representation from the texture matrix
	// these shift scale rot values are to be understood in the local axis base
	// Note: this code looks similar to Texdef_fromTransform, but the algorithm is slightly different.
	TexDef getFakeTexCoords() const {
		TexDef texdef;
	
		texdef._scale[0] = static_cast<float>(1.0 / Vector2(coords[0][0], coords[1][0]).getLength());
		texdef._scale[1] = static_cast<float>(1.0 / Vector2(coords[0][1], coords[1][1]).getLength());
		
		texdef._rotate = -static_cast<float>(radians_to_degrees(arctangent_yx(coords[1][0], coords[0][0])));
		
		texdef._shift[0] = -coords[0][2];
		texdef._shift[1] = coords[1][2];
		
		// determine whether or not an axis is flipped using a 2d cross-product
		double cross = Vector2(coords[0][0], coords[0][1]).crossProduct(Vector2(coords[1][0], coords[1][1]));
		
		if (cross < 0) {
			// This is a bit of a compromise when using BPs--since we don't know *which* axis was flipped,
			// we pick one (rather arbitrarily) using the following convention: If the X-axis is between
			// 0 and 180, we assume it's the Y-axis that flipped, otherwise we assume it's the X-axis and
			// subtract out 180 degrees to compensate.
			if (texdef._rotate >= 180.0f) {
			      texdef._rotate -= 180.0f;
			      texdef._scale[0] = -texdef._scale[0];
			}
			else {
			      texdef._scale[1] = -texdef._scale[1];
			}
		}
		
		return texdef;
	}
	
	// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
	// This function normalises shift values to the smallest positive congruent values.
	void normalise(float width, float height) {
		coords[0][2] = float_mod(coords[0][2], width);
		coords[1][2] = float_mod(coords[1][2], height);
	}

	/* greebo: This returns the transformation matrix.
	 * As the member variables already ARE the matrix
	 * components, they are just copied into the right places.
	 */
	Matrix4 getTransform() const {
		// Initialise the return value with the identity matrix
		Matrix4 transform = g_matrix4_identity;
		
		// Just copy the member variables to the according matrix components
		transform.xx() = coords[0][0];
		transform.yx() = coords[0][1];
		transform.tx() = coords[0][2];
		transform.xy() = coords[1][0];
		transform.yy() = coords[1][1];
		transform.ty() = coords[1][2];
		
		return transform;
	}
};

#endif /*BRUSHTEXTUREDEFINTION_H_*/
