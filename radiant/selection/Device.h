#pragma once

#include "math/Vector2.h"
#include <algorithm>

// Maps the range [0..size] to [-1..+1]
inline float screen_normalised(Vector2::ElementType pos, std::size_t size)
{
	return ((2.0f * pos) / size) - 1.0f;
}

inline Vector2 window_to_normalised_device(const Vector2& window, std::size_t width, std::size_t height)
{
	return Vector2(screen_normalised(window.x(), width), screen_normalised(height - 1 - window.y(), height));
}

// Constrains the value pos to the range [-1..+1]
inline Vector2::ElementType device_constrained(Vector2::ElementType pos)
{
	return std::min(1.0, std::max(-1.0, pos));
}

// See device_constrained(), this cuts off the Vector's components at -1 or +1
inline Vector2 device_constrained(const Vector2& device)
{
	return Vector2(device_constrained(device.x()), device_constrained(device.y()));
}
