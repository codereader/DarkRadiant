#include "TextureMatrix.h"

#include "texturelib.h"
#include "math/Vector2.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"

// Constructor with empty arguments
TextureMatrix::TextureMatrix()
{
	coords[0][0] = 2.0f;
	coords[0][1] = 0.f;
	coords[0][2] = 0.f;
	coords[1][0] = 0.f;
	coords[1][1] = 2.0f;
	coords[1][2] = 0.f;
}

TextureMatrix::TextureMatrix(const Matrix3& transform)
{
    coords[0][0] = transform.xx();
    coords[0][1] = transform.yx();
    coords[0][2] = transform.zx();
    coords[1][0] = transform.xy();
    coords[1][1] = transform.yy();
    coords[1][2] = transform.zy();
}

// Construct a TextureMatrix out of "fake" shift scale rot definitions
TextureMatrix::TextureMatrix(const TexDef& texdef)
{
	auto r = degrees_to_radians(-texdef.getRotation());
	auto c = cos(r);
	auto s = sin(r);

	auto scale = texdef.getScale();
	auto x = 1.0 / scale[0];
	auto y = 1.0 / scale[1];

	auto shift = texdef.getShift();
	coords[0][0] = x * c;
	coords[1][0] = x * s;
	coords[0][1] = y * -s;
	coords[1][1] = y * c;
	coords[0][2] = -shift[0];
	coords[1][2] = shift[1];
}

// shift a texture (texture adjustments) along it's current texture axes
void TextureMatrix::shift(double s, double t)
{
	// x and y are geometric values, which we must compute as ST increments
	// this depends on the texture size and the pixel/texel ratio
	// as a ratio against texture size
	// the scale of the texture is not relevant here (we work directly on a transformation from the base vectors)
	coords[0][2] -= s;
	coords[1][2] += t;
}

/* greebo: This removes the texture scaling from the
 * coordinates. The resulting coordinates are absolute
 * values within the shader image.
 *
 * An 128x256 texture with scaled coordinates 0.5,0.5
 * would be translated into the coordinates 64,128,
 * pointing to a defined pixel within the texture image.
 */
void TextureMatrix::applyShaderDimensions(std::size_t width, std::size_t height) {
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
void TextureMatrix::addScale(std::size_t width, std::size_t height) {
	coords[0][0] /= width;
	coords[0][1] /= width;
	coords[0][2] /= width;
	coords[1][0] /= height;
	coords[1][1] /= height;
	coords[1][2] /= height;
}

ShiftScaleRotation TextureMatrix::getShiftScaleRotation() const
{
	ShiftScaleRotation ssr;

	ssr.scale[0] = 1.0 / Vector2(coords[0][0], coords[1][0]).getLength();
	ssr.scale[1] = 1.0 / Vector2(coords[0][1], coords[1][1]).getLength();

	ssr.rotate = -radians_to_degrees(arctangent_yx(coords[1][0], coords[0][0]));

	ssr.shift[0] = -coords[0][2];
	ssr.shift[1] = coords[1][2];

	// determine whether or not an axis is flipped using a 2d cross-product
	auto cross = Vector2(coords[0][0], coords[0][1]).crossProduct(Vector2(coords[1][0], coords[1][1]));

	if (cross < 0)
	{
		// This is a bit of a compromise when using BPs--since we don't know *which* axis was flipped,
		// we pick one (rather arbitrarily) using the following convention: If the X-axis is between
		// 0 and 180, we assume it's the Y-axis that flipped, otherwise we assume it's the X-axis and
		// subtract out 180 degrees to compensate.
		if (ssr.rotate >= 180.0)
		{
		    ssr.rotate -= 180.0;
		    ssr.scale[0] = -ssr.scale[0];
		}
		else 
        {
		    ssr.scale[1] = -ssr.scale[1];
		}
	}

	return ssr;
}

// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
// This function normalises shift values to the smallest positive congruent values.
void TextureMatrix::normalise(float width, float height) {
	coords[0][2] = float_mod(coords[0][2], width);
	coords[1][2] = float_mod(coords[1][2], height);
}

/* greebo: This returns the transformation matrix.
 * As the member variables already ARE the matrix
 * components, they are just copied into the right places.
 */
Matrix4 TextureMatrix::getTransform() const
{
	// Initialise the return value with the identity matrix
	Matrix4 transform = Matrix4::getIdentity();

	// Just copy the member variables to the according matrix components
	transform.xx() = coords[0][0];
	transform.yx() = coords[0][1];
	transform.tx() = coords[0][2];
	transform.xy() = coords[1][0];
	transform.yy() = coords[1][1];
	transform.ty() = coords[1][2];

	return transform;
}

Matrix3 TextureMatrix::getMatrix3() const
{
    return Matrix3::byRows(
        coords[0][0], coords[0][1], coords[0][2],
        coords[1][0], coords[1][1], coords[1][2],
        0, 0, 1
    );
}

bool TextureMatrix::isSane() const
{
    return !std::isnan(coords[0][0]) && !std::isinf(coords[0][0]) &&
           !std::isnan(coords[0][1]) && !std::isinf(coords[0][1]) &&
           !std::isnan(coords[0][2]) && !std::isinf(coords[0][2]) &&
           !std::isnan(coords[1][0]) && !std::isinf(coords[1][0]) &&
           !std::isnan(coords[1][1]) && !std::isinf(coords[1][1]) &&
           !std::isnan(coords[1][2]) && !std::isinf(coords[1][2]);
}
