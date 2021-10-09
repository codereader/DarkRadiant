#include "TextureMatrix.h"

#include "texturelib.h"
#include "math/pi.h"
#include "math/Vector2.h"
#include "math/Matrix3.h"

TextureMatrix::TextureMatrix()
{
	_coords[0][0] = 2.0f;
	_coords[0][1] = 0.f;
	_coords[0][2] = 0.f;
	_coords[1][0] = 0.f;
	_coords[1][1] = 2.0f;
	_coords[1][2] = 0.f;
}

TextureMatrix::TextureMatrix(const Matrix3& transform)
{
    _coords[0][0] = transform.xx();
    _coords[0][1] = transform.yx();
    _coords[0][2] = transform.zx();
    _coords[1][0] = transform.xy();
    _coords[1][1] = transform.yy();
    _coords[1][2] = transform.zy();
}

TextureMatrix::TextureMatrix(const ShiftScaleRotation& ssr)
{
	auto r = degrees_to_radians(-ssr.rotate);
	auto c = cos(r);
	auto s = sin(r);

	auto x = 1.0 / ssr.scale[0];
	auto y = 1.0 / ssr.scale[1];

	_coords[0][0] = x * c;
	_coords[1][0] = x * s;
	_coords[0][1] = y * -s;
	_coords[1][1] = y * c;
	_coords[0][2] = -ssr.shift[0];
	_coords[1][2] = ssr.shift[1];
}

void TextureMatrix::shift(double s, double t)
{
	// x and y are geometric values, which we must compute as ST increments
	// this depends on the texture size and the pixel/texel ratio
	// as a ratio against texture size
	// the scale of the texture is not relevant here (we work directly on a transformation from the base vectors)
	_coords[0][2] -= s;
	_coords[1][2] += t;
}

void TextureMatrix::addScale(std::size_t width, std::size_t height)
{
	_coords[0][0] /= width;
	_coords[0][1] /= width;
	_coords[0][2] /= width;
	_coords[1][0] /= height;
	_coords[1][1] /= height;
	_coords[1][2] /= height;
}

ShiftScaleRotation TextureMatrix::getShiftScaleRotation() const
{
	ShiftScaleRotation ssr;

	ssr.scale[0] = 1.0 / Vector2(_coords[0][0], _coords[1][0]).getLength();
	ssr.scale[1] = 1.0 / Vector2(_coords[0][1], _coords[1][1]).getLength();

	ssr.rotate = -radians_to_degrees(arctangent_yx(_coords[1][0], _coords[0][0]));

	ssr.shift[0] = -_coords[0][2];
	ssr.shift[1] = _coords[1][2];

	// determine whether or not an axis is flipped using a 2d cross-product
	auto cross = Vector2(_coords[0][0], _coords[0][1]).crossProduct(Vector2(_coords[1][0], _coords[1][1]));

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

void TextureMatrix::normalise(float width, float height)
{
	_coords[0][2] = float_mod(_coords[0][2], width);
	_coords[1][2] = float_mod(_coords[1][2], height);
}

Matrix3 TextureMatrix::getMatrix3() const
{
    return Matrix3::byRows(
        _coords[0][0], _coords[0][1], _coords[0][2],
        _coords[1][0], _coords[1][1], _coords[1][2],
        0, 0, 1
    );
}

bool TextureMatrix::isSane() const
{
    return !std::isnan(_coords[0][0]) && !std::isinf(_coords[0][0]) &&
           !std::isnan(_coords[0][1]) && !std::isinf(_coords[0][1]) &&
           !std::isnan(_coords[0][2]) && !std::isinf(_coords[0][2]) &&
           !std::isnan(_coords[1][0]) && !std::isinf(_coords[1][0]) &&
           !std::isnan(_coords[1][1]) && !std::isinf(_coords[1][1]) &&
           !std::isnan(_coords[1][2]) && !std::isinf(_coords[1][2]);
}
