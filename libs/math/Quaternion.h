#pragma once

/// \file
/// \brief Quaternion data types and related operations.

#include "Vector3.h"
#include "Vector4.h"

// Forward declaration, include Matrix4.h for definition
class Matrix4;

/// \brief A quaternion stored in single-precision floating-point.
class Quaternion :
	public Vector4
{
public:
	Quaternion() :
		Vector4()
	{}

	Quaternion(const Vector4& vector) :
		Vector4(vector)
	{}

	// Construct a Quaternion out of the 4 arguments
	Quaternion(ElementType x_, ElementType y_, ElementType z_, ElementType w_) :
		Vector4(x_, y_, z_, w_)
	{}

	// Construct a Quaternion out of a Vector3 plus a fourth argument
  	Quaternion(const Vector3& other, ElementType w_) :
		Vector4(other, w_)
	{}

	/**
	 * Returns the identity quaternion (named constructor)
	 */
	static const Quaternion& Identity();

	/**
	 * Constructs a quaternion which rotates between two points on the unit-sphere (from and to).
	 */
	static Quaternion createForUnitVectors(const Vector3& from, const Vector3& to);

	/**
	 * Constructs a rotation quaternion for the given euler angles.
	 * Each component of the given vector refers to an angle (in degrees)
	 * of one of the x-/y-/z-axis.
	 */
	static Quaternion createForEulerXYZDegrees(const Vector3& eulerXYZ);

	/**
	 * Constructs a quat for the given axis and angle
	 */
	static Quaternion createForAxisAngle(const Vector3& axis, double angle);

	/**
	 * Constructs a rotation quaternion about a given axis.
	 */
	static Quaternion createForX(double angle);
	static Quaternion createForY(double angle);
	static Quaternion createForZ(double angle);

	/**
	 * Returns this quaternion multiplied by the other one.
	 */
	Quaternion getMultipliedBy(const Quaternion& other) const;

	/**
	 * Multiplies this quaternion by the other one in place.
	 * Equivalent to: *this = getMultipliedBy(other);
	 */
	void multiplyBy(const Quaternion& other);

	/**
	 * Multiplies this quaternion by the other one in place.
	 * Equivalent to: *this = other.getMultipliedBy(*this);
	 */
	void preMultiplyBy(const Quaternion& other);

	/**
	 * Returns the inverse of this quaternion.
	 */
	Quaternion getInverse() const;

	/**
	 * Conjugates/inverts this quaternion in place.
	 */
	void conjugate();

	/**
	 * Returns a normalised copy of this quaternion.
	 */
	Quaternion getNormalised() const;

	/**
	 * Normalise this quaternion in-place.
	 */
	void normalise();

	/**
	 * Returns the given point as transformed by this quaternion
	 */
	template<typename ElementType>
    BasicVector3<ElementType> transformPoint(const BasicVector3<ElementType>& point) const
    {
        double xx = x() * x();
        double yy = y() * y();
        double zz = z() * z();
        double ww = w() * w();

        double xy2 = x() * y() * 2;
        double xz2 = x() * z() * 2;
        double xw2 = x() * w() * 2;
        double yz2 = y() * z() * 2;
        double yw2 = y() * w() * 2;
        double zw2 = z() * w() * 2;

        return BasicVector3<ElementType>(
            static_cast<ElementType>(ww * point.x() + yw2 * point.z() - zw2 * point.y() + xx * point.x() + xy2 * point.y() + xz2 * point.z() - zz * point.x() - yy * point.x()),
            static_cast<ElementType>(xy2 * point.x() + yy * point.y() + yz2 * point.z() + zw2 * point.x() - zz * point.y() + ww * point.y() - xw2 * point.z() - xx * point.y()),
            static_cast<ElementType>(xz2 * point.x() + yz2 * point.y() + zz * point.z() - yw2 * point.x() - yy * point.z() + xw2 * point.y() - xx * point.z() + ww * point.z())
        );
    }
};

inline const Quaternion& Quaternion::Identity()
{
	static Quaternion _identity(0, 0, 0, 1);
	return _identity;
}

inline Quaternion Quaternion::createForUnitVectors(const Vector3& from, const Vector3& to)
{
	return Quaternion(from.cross(to), from.dot(to));
}

inline Quaternion Quaternion::createForEulerXYZDegrees(const Vector3& eulerXYZ)
{
	double cx = cos(degrees_to_radians(eulerXYZ[0] * 0.5f));
	double sx = sin(degrees_to_radians(eulerXYZ[0] * 0.5f));
	double cy = cos(degrees_to_radians(eulerXYZ[1] * 0.5f));
	double sy = sin(degrees_to_radians(eulerXYZ[1] * 0.5f));
	double cz = cos(degrees_to_radians(eulerXYZ[2] * 0.5f));
	double sz = sin(degrees_to_radians(eulerXYZ[2] * 0.5f));

	return Quaternion(
		cz * cy * sx - sz * sy * cx,
		cz * sy * cx + sz * cy * sx,
		sz * cy * cx - cz * sy * sx,
		cz * cy * cx + sz * sy * sx
	);
}

inline Quaternion Quaternion::createForAxisAngle(const Vector3& axis, double angle)
{
	angle *= 0.5f;
	double sa = sin(angle);
	return Quaternion(axis[0] * sa, axis[1] * sa, axis[2] * sa, cos(angle));
}

inline Quaternion Quaternion::createForX(double angle)
{
	angle *= 0.5f;
	return Quaternion(sin(angle), 0, 0, cos(angle));
}

inline Quaternion Quaternion::createForY(double angle)
{
	angle *= 0.5f;
	return Quaternion(0, sin(angle), 0, cos(angle));
}

inline Quaternion Quaternion::createForZ(double angle)
{
	angle *= 0.5f;
	return Quaternion(0, 0, sin(angle), cos(angle));
}

inline Quaternion Quaternion::getMultipliedBy(const Quaternion& other) const
{
	return Quaternion(
		w() * other.x() + x() * other.w() + y() * other.z() - z() * other.y(),
		w() * other.y() + y() * other.w() + z() * other.x() - x() * other.z(),
		w() * other.z() + z() * other.w() + x() * other.y() - y() * other.x(),
		w() * other.w() - x() * other.x() - y() * other.y() - z() * other.z()
	);
}

inline void Quaternion::multiplyBy(const Quaternion& other)
{
	*this = getMultipliedBy(other);
}

inline void Quaternion::preMultiplyBy(const Quaternion& other)
{
	*this = other.getMultipliedBy(*this);
}

inline Quaternion Quaternion::getInverse() const
{
	return Quaternion(-getVector3(), w());
}

inline void Quaternion::conjugate()
{
	*this = getInverse();
}

inline Quaternion Quaternion::getNormalised() const
{
	const double n = 1.0f / sqrt(x() * x() + y() * y() + z() * z() + w() * w());

	return Quaternion(x() * n, y() * n, z() * n, w() * n);
}

inline void Quaternion::normalise()
{
	*this = getNormalised();
}

const double c_half_sqrt2 = 0.70710678118654752440084436210485;
const float c_half_sqrt2f = static_cast<float>(c_half_sqrt2);

/// Stream insertion for Quaternion
inline std::ostream& operator<< (std::ostream& s, const Quaternion& q)
{
    return s << "Quaternion(x=" << q.x() << ", y=" << q.y() << ", z=" << q.z()
             << ", w=" << q.w() << ")";
}