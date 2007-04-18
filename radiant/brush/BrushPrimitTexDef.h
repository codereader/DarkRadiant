#ifndef BRUSHTEXTUREDEFINTION_H_
#define BRUSHTEXTUREDEFINTION_H_

#include "math/matrix.h"
#include "TexDef.h"

struct BrushPrimitTexDef {
	double coords[2][3];	
	
	// Constructor
	BrushPrimitTexDef();
	
	// Construct the BP Definition out of the transformation matrix
	// Basically copies over the values from the according components
	BrushPrimitTexDef(const Matrix4& transform);
	
	// Construct a BrushPrimitTexDef out of "fake" shift scale rot definitions
	BrushPrimitTexDef(const TexDef& texdef);
	
	// shift a texture (texture adjustments) along it's current texture axes
	void shift(double s, double t);
	
	// apply same scale as the spinner button of the surface inspector
	void scale(double s, double t);
	
	// apply same rotation as the spinner button of the surface inspector
	void rotate(double angle);
	
	/* greebo: This removes the texture scaling from the
	 * coordinates. The resulting coordinates are absolute
	 * values within the shader image.
	 * 
	 * An 128x256 texture with scaled coordinates 0.5,0.5
	 * would be translated into the coordinates 64,128,
	 * pointing to a defined pixel within the texture image.
	 */
	void removeScale(std::size_t width, std::size_t height);
	
	/* greebo: this converts absolute coordinates into
	 * relative ones, where everything is measured
	 * in multiples of the texture x/y dimensions. */
	void addScale(std::size_t width, std::size_t height);
	
	// compute a fake shift scale rot representation from the texture matrix
	// these shift scale rot values are to be understood in the local axis base
	// Note: this code looks similar to Texdef_fromTransform, but the algorithm is slightly different.
	TexDef getFakeTexCoords() const;
	
	// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
	// This function normalises shift values to the smallest positive congruent values.
	void normalise(double width, double height);

	/* greebo: This returns the transformation matrix.
	 * As the member variables already ARE the matrix
	 * components, they are just copied into the right places.
	 */
	Matrix4 getTransform() const;
};

inline std::ostream& operator<<(std::ostream& st, const BrushPrimitTexDef& texdef) {
	st << "<" << texdef.coords[0][0] << ", " << texdef.coords[0][1] << ", " << texdef.coords[0][2] << ">\n";
	st << "<" << texdef.coords[1][0] << ", " << texdef.coords[1][1] << ", " << texdef.coords[1][2] << ">";
	return st;
}

#endif /*BRUSHTEXTUREDEFINTION_H_*/
