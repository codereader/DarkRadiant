#ifndef BRUSHTEXTUREDEFINTION_H_
#define BRUSHTEXTUREDEFINTION_H_

#include "math/matrix.h"

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
