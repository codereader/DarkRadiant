#pragma once

#include "TexDef.h"

class Matrix4;

// Encapsulates the 2x3 matrix transforming world coordinates into texture space
struct TextureMatrix
{
	float coords[2][3];

	// Constructor
	TextureMatrix();

	// Construct the BP Definition out of the transformation matrix
	// Basically copies over the values from the according components
	TextureMatrix(const Matrix4& transform);

	// Construct a TextureMatrix out of "fake" shift scale rot definitions
	TextureMatrix(const TexDef& texdef);

	// shift a texture (texture adjustments) along it's current texture axes
	void shift(float s, float t);

	// Scales texture by the given float values (1.05 scales texture to 105%)
	void scale(float s, float t, std::size_t shaderWidth, std::size_t shaderHeight);

	// apply same rotation as the spinner button of the surface inspector
	void rotate(float angle, std::size_t shaderWidth, std::size_t shaderHeight);

	/* greebo: This removes the texture scaling from the
	 * coordinates. The resulting coordinates are absolute
	 * values within the shader image.
	 *
	 * An 128x256 texture with scaled coordinates 0.5,0.5
	 * would be translated into the coordinates 64,128,
	 * pointing to a defined pixel within the texture image.
	 */
	void applyShaderDimensions(std::size_t width, std::size_t height);

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
	void normalise(float width, float height);

	/* greebo: This returns the transformation matrix.
	 * As the member variables already ARE the matrix
	 * components, they are just copied into the right places.
	 */
	Matrix4 getTransform() const;
};

inline std::ostream& operator<<(std::ostream& st, const TextureMatrix& texdef)
{
	st << "<" << texdef.coords[0][0] << ", " << texdef.coords[0][1] << ", " << texdef.coords[0][2] << ">\n";
	st << "<" << texdef.coords[1][0] << ", " << texdef.coords[1][1] << ", " << texdef.coords[1][2] << ">";
	return st;
}
