#include "TexDef.h"

#include <math.h>
#include "texturelib.h"
#include "math/Vector2.h"

TexDef::TexDef(const ShiftScaleRotation& ssr) :
    _def(ssr)
{}

// Constructs a TexDef out of the given transformation matrix plus width/height
TexDef::TexDef(double width, double height, const Matrix4& transform)
{
	_def.scale[0] = static_cast<double>((1.0 / Vector2(transform[0], transform[4]).getLength()) / width);
	_def.scale[1] = static_cast<double>((1.0 / Vector2(transform[1], transform[5]).getLength()) / height);

	_def.rotate = static_cast<double>(-radians_to_degrees(arctangent_yx(-transform[4], transform[0])));

	if (_def.rotate == -180.0f)
	{
		_def.rotate = 180.0f;
	}

	_def.shift[0] = transform[12] * width;
	_def.shift[1] = transform[13] * height;

	// If the 2d cross-product of the x and y axes is positive, one of the axes has a negative scale.
	if (Vector2(transform[0], transform[4]).crossProduct(Vector2(transform[1], transform[5])) > 0)
	{
		if (_def.rotate >= 180.0f)
		{
			_def.rotate -= 180.0f;
			_def.scale[0] = -_def.scale[0];
		}
		else
		{
			_def.scale[1] = -_def.scale[1];
		}
	}
}

double TexDef::getRotation() const
{
	return _def.rotate;
}

void TexDef::setRotation(double rotation)
{
	_def.rotate = rotation;
}

Vector2 TexDef::getShift() const
{
	return Vector2(_def.shift);
}

void TexDef::setShift(const Vector2& shift)
{
    _def.shift[0] = shift.x();
    _def.shift[1] = shift.y();
}

Vector2 TexDef::getScale() const
{
	return Vector2(_def.scale);
}

void TexDef::setScale(const Vector2& scale)
{
	_def.scale[0] = scale[0];
	_def.scale[1] = scale[1];
}

void TexDef::shift(double s, double t)
{
	_def.shift[0] += s;
	_def.shift[1] += t;
}

void TexDef::scale(double s, double t)
{
	_def.scale[0] += s;
	_def.scale[1] += t;
}

void TexDef::rotate(double angle)
{
	_def.rotate += angle;
	_def.rotate = static_cast<double>(float_to_integer(_def.rotate) % 360);
}

// Checks the TexDef for insanely large values
bool TexDef::isSane() const
{
	return fabs(_def.shift[0]) < (1 << 16)
		&& fabs(_def.shift[1]) < (1 << 16);
}

// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
// This function normalises shift values to the smallest positive congruent values.
void TexDef::normalise(double width, double height)
{
	// it may be useful to also normalise the rotation here, if this function is used elsewhere.
	_def.shift[0] = float_mod(_def.shift[0], width);
	_def.shift[1] = float_mod(_def.shift[1], height);
}
