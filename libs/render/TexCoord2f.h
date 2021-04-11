#pragma once

#include "math/Vector2.h"

/// \brief A 2-component texture-coordinate set.
class TexCoord2f :
	public Vector2
{
public:
	// Default constructor
	TexCoord2f() :
		Vector2(0,0)
	{}

	// constructor
	TexCoord2f(double s, double t) :
		Vector2(s, t)
	{}

	// Copy constructor
	TexCoord2f(const TexCoord2f& other) :
		Vector2(other.s(), other.t())
	{}

	// Copy constructor from Vector2
	TexCoord2f(const Vector2& other) :
		Vector2(other.x(), other.y())
	{}

	double& s() {
		return x();
	}
	const double& s() const {
		return x();
	}
	double& t() {
		return y();
	}
	const double& t() const {
		return y();
	}

	bool operator< (const TexCoord2f& other) const {
		if (s() != other.s()) {
			return s() < other.s();
		}
		if (t() != other.t()) {
			return t() < other.t();
		}
		return false;
	}
};

namespace math
{

/// Epsilon equality test for TexCoord2f
inline bool isNear(const TexCoord2f& v1, const TexCoord2f& v2, double epsilon)
{
    TexCoord2f diff = v1 - v2;
    return std::abs(diff.x()) < epsilon && std::abs(diff.y()) < epsilon;
}

}

