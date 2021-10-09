#include "TexDef.h"

#include <math.h>
#include "texturelib.h"
#include "math/Vector2.h"

// Constructor
TexDef::TexDef()
{
	_shift[0] = 0;
	_shift[1] = 0;
	_rotate = 0;
	_scale[0] = 1;
	_scale[1] = 1;
}

// Constructs a TexDef out of the given transformation matrix plus width/height
TexDef::TexDef(double width, double height, const Matrix4& transform)
{
	_scale[0] = static_cast<double>((1.0 / Vector2(transform[0], transform[4]).getLength()) / width);
	_scale[1] = static_cast<double>((1.0 / Vector2(transform[1], transform[5]).getLength()) / height);

	_rotate = static_cast<double>(-radians_to_degrees(arctangent_yx(-transform[4], transform[0])));

	if (_rotate == -180.0f)
	{
		_rotate = 180.0f;
	}

	_shift[0] = transform[12] * width;
	_shift[1] = transform[13] * height;

	// If the 2d cross-product of the x and y axes is positive, one of the axes has a negative scale.
	if (Vector2(transform[0], transform[4]).crossProduct(Vector2(transform[1], transform[5])) > 0)
	{
		if (_rotate >= 180.0f)
		{
			_rotate -= 180.0f;
			_scale[0] = -_scale[0];
		}
		else
		{
			_scale[1] = -_scale[1];
		}
	}
}

double TexDef::getRotation() const
{
	return _rotate;
}

void TexDef::setRotation(double rotation)
{
	_rotate = rotation;
}

Vector2 TexDef::getShift() const
{
	return Vector2(_shift);
}

void TexDef::setShift(const Vector2& shift)
{
    _shift[0] = shift.x();
    _shift[1] = shift.y();
}

Vector2 TexDef::getScale() const
{
	return Vector2(_scale);
}

void TexDef::setScale(const Vector2& scale)
{
	_scale[0] = scale[0];
	_scale[1] = scale[1];
}

void TexDef::shift(double s, double t)
{
	_shift[0] += s;
	_shift[1] += t;
}

void TexDef::scale(double s, double t)
{
	_scale[0] += s;
	_scale[1] += t;
}

void TexDef::rotate(double angle)
{
	_rotate += angle;
	_rotate = static_cast<double>(float_to_integer(_rotate) % 360);
}

// Checks the TexDef for insanely large values
bool TexDef::isSane() const
{
	return fabs(_shift[0]) < (1 << 16)
		&& fabs(_shift[1]) < (1 << 16);
}

// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
// This function normalises shift values to the smallest positive congruent values.
void TexDef::normalise(double width, double height)
{
	// it may be useful to also normalise the rotation here, if this function is used elsewhere.
	_shift[0] = float_mod(_shift[0], width);
	_shift[1] = float_mod(_shift[1], height);
}

ShiftScaleRotation TexDef::toShiftScaleRotation() const
{
	ShiftScaleRotation result;

	result.shift[0] = _shift[0];
	result.shift[1] = _shift[1];
	result.rotate = _rotate;
	result.scale[0] = _scale[0];
	result.scale[1] = _scale[1];

	return result;
}

TexDef TexDef::CreateFromShiftScaleRotation(const ShiftScaleRotation& scr)
{
	TexDef texDef;

	texDef._shift[0] = scr.shift[0];
	texDef._shift[1] = scr.shift[1];
	texDef._rotate = scr.rotate;
	texDef._scale[0] = scr.scale[0];
	texDef._scale[1] = scr.scale[1];

	return texDef;
}

