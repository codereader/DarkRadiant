#pragma once

#include "TexDef.h"

class Matrix3;

// Encapsulates the 2x3 matrix transforming world coordinates into texture space
// 
// greebo: A brief note on why just 6 defined matrix components are enough here: 
// Normally projecting 3D coordinates might require a full 4x4 matrix such that every 
// XYZ component can be contributing. But in idTech engines the vertices are preprocessed
// using an "axis base" transformation as first step, which is rotating the world such that 
// the Z direction is aligned with the "direction" of the projection.
// 
// The full path from XYZ to UV is therefore: T x AB x XYZ = UV
// 
// The final texture matrix can just ignore the Z component of every vertex and 
// transform the remaining, meaningful XY components of the vector using a homegeneous 
// 3x3 matrix transformation. (The third component of the vector is used as 
// the W coordinate and is set to 1, to make it translatable by tx and ty):
// 
//  | xx yx tx |   | x |   | u |
//  | xy yy ty | X | y | = | v |
//  | 0  0  1  |   | 1 |   | 1 |
// 
// Therefore only 6 compontents are really needed to define the matrix.
// The trick to rotate the world space such that it is "oriented" along a plane changed 
// over time, from what I gathered reading the engine code.
// Q3-type engines seemed to pick one of the 6 axis-aligned planes (picking the one matching
// the face normal the most), whereas Doom 3 goes a step further and aligns the Z axis 
// of the world system to the normal of the face plane. That's why the values of shift/scale/rotation
// texdef defined in a Q3 map file will never look the same in idTech4 engines when placed on
// angled brush faces - there will always be a stretch in some direction, which the D3 engine
// completely works around by using the actual face normal - no stretching.
class TextureMatrix final
{
private:
	double coords[2][3];

public:
	TextureMatrix();

    // Copy-construct from the relevant components from the given transform 
    // (which is everything except the last row: xz() and yz() are 0, zz() is 1)
	TextureMatrix(const Matrix3& transform);

	// Construct a TextureMatrix out of "fake" shift scale rot definitions
	TextureMatrix(const TexDef& texdef);

	// shift a texture (texture adjustments) along it's current texture axes
	void shift(double s, double t);

	/* greebo: this converts absolute coordinates into
	 * relative ones, where everything is measured
	 * in multiples of the texture x/y dimensions. */
	void addScale(std::size_t width, std::size_t height);

	// compute a fake shift scale rot representation from the texture matrix
	// these shift scale rot values are to be understood in the local axis base
	// Note: this code looks similar to Texdef_fromTransform, but the algorithm is slightly different.
    ShiftScaleRotation getShiftScaleRotation() const;

	// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
	// This function normalises shift values to the smallest positive congruent values.
	void normalise(float width, float height);

    // Returns the Matrix3 form of this instance
    Matrix3 getMatrix3() const;

    // Checks if any of the matrix components are NaN or INF (in which case the matrix is not sane)
    bool isSane() const;

    friend std::ostream& operator<<(std::ostream& st, const TextureMatrix& texdef);
};

inline std::ostream& operator<<(std::ostream& st, const TextureMatrix& texdef)
{
	st << "<" << texdef.coords[0][0] << ", " << texdef.coords[0][1] << ", " << texdef.coords[0][2] << ">\n";
	st << "<" << texdef.coords[1][0] << ", " << texdef.coords[1][1] << ", " << texdef.coords[1][2] << ">";
	return st;
}
