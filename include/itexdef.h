#pragma once

namespace brush
{

/**
 * greebo: The texture definition structure containing the scale,
 * rotation and shift values of an applied texture.
 * At some places this is referred to as "fake" texture coordinates.
 */
struct ShiftScaleRotation
{
	double	_shift[2];
	double	_rotate;
	double	_scale[2];
};

}
