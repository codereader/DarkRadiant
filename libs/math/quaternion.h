#pragma once

/// \file
/// \brief Quaternion data types and related operations.

#include "math/Matrix4.h"

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
  	 * Construct a Quaternion by parsing the space-separated string.
  	 */
  	Quaternion(const std::string& str) :
		Vector4(str)
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
	static Quaternion createForAxisAngle(const Vector3& axis, float angle);

	/** 
	 * Constructs a rotation quaternion about a given axis.
	 */
	static Quaternion createForX(float angle);
	static Quaternion createForY(float angle);
	static Quaternion createForZ(float angle);

	/**
	 * Returns this quaternion multiplied by the other one.
	 */
	Quaternion getMultipliedBy(const Quaternion& other) const;

	/**
	 * Multiplies this quaternion by the other one in place.
	 */
	void multiplyBy(const Quaternion& other);

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
};

inline const Quaternion& Quaternion::Identity()
{
	static Quaternion _identity(0, 0, 0, 1);
	return _identity;
}

inline Quaternion Quaternion::createForUnitVectors(const Vector3& from, const Vector3& to)
{
	return Quaternion(from.crossProduct(to), from.dot(to));
}

inline Quaternion Quaternion::createForEulerXYZDegrees(const Vector3& eulerXYZ)
{
	float cx = cos(degrees_to_radians(eulerXYZ[0] * 0.5f));
	float sx = sin(degrees_to_radians(eulerXYZ[0] * 0.5f));
	float cy = cos(degrees_to_radians(eulerXYZ[1] * 0.5f));
	float sy = sin(degrees_to_radians(eulerXYZ[1] * 0.5f));
	float cz = cos(degrees_to_radians(eulerXYZ[2] * 0.5f));
	float sz = sin(degrees_to_radians(eulerXYZ[2] * 0.5f));

	return Quaternion(
		cz * cy * sx - sz * sy * cx,
		cz * sy * cx + sz * cy * sx,
		sz * cy * cx - cz * sy * sx,
		cz * cy * cx + sz * sy * sx
	);
}

inline Quaternion Quaternion::createForAxisAngle(const Vector3& axis, float angle)
{
	angle *= 0.5f;
	float sa = sin(angle);
	return Quaternion(axis[0] * sa, axis[1] * sa, axis[2] * sa, cos(angle));
}

inline Quaternion Quaternion::createForX(float angle)
{
	angle *= 0.5f;
	return Quaternion(sin(angle), 0, 0, cos(angle));
}

inline Quaternion Quaternion::createForY(float angle)
{
	angle *= 0.5f;
	return Quaternion(0, sin(angle), 0, cos(angle));
}

inline Quaternion Quaternion::createForZ(float angle)
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
	const float n = 1.0f / (x() * x() + y() * y() + z() * z() + w() * w());

	return Quaternion(x() * n, y() * n, z() * n, w() * n);
}

inline void Quaternion::normalise()
{
	*this = getNormalised();
}

const double c_half_sqrt2 = 0.70710678118654752440084436210485;
const float c_half_sqrt2f = static_cast<float>(c_half_sqrt2);


Quaternion quaternion_for_matrix4_rotation(const Matrix4& matrix4)
{
  Matrix4 transposed = matrix4.getTransposed();

  float trace = transposed[0] + transposed[5] + transposed[10] + 1.0f;

  if(trace > 0.0001)
  {
    float S = 0.5f / sqrt(trace);
    return Quaternion(
      (transposed[9] - transposed[6]) * S,
      (transposed[2] - transposed[8]) * S,
      (transposed[4] - transposed[1]) * S,
      0.25f / S
    );
  }

  if(transposed[0] >= transposed[5] && transposed[0] >= transposed[10])
  {
    float S = 2.0f * sqrt(1.0f + transposed[0] - transposed[5] - transposed[10]);
    return Quaternion(
      0.25f / S,
      (transposed[1] + transposed[4]) / S,
      (transposed[2] + transposed[8]) / S,
      (transposed[6] + transposed[9]) / S
    );
  }

  if(transposed[5] >= transposed[0] && transposed[5] >= transposed[10])
  {
    float S = 2.0f * sqrt(1.0f + transposed[5] - transposed[0] - transposed[10]);
    return Quaternion(
      (transposed[1] + transposed[4]) / S,
      0.25f / S,
      (transposed[6] + transposed[9]) / S,
      (transposed[2] + transposed[8]) / S
    );
  }

  float S = 2.0f * sqrt(1.0f + transposed[10] - transposed[0] - transposed[5]);
  return Quaternion(
    (transposed[2] + transposed[8]) / S,
    (transposed[6] + transposed[9]) / S,
    0.25f / S,
    (transposed[1] + transposed[4]) / S
  );
}

/// \brief Returns \p self concatenated with the rotation transform produced by \p rotation.
/// The concatenated rotation occurs before \p self.
inline Matrix4 matrix4_rotated_by_quaternion(const Matrix4& self, const Quaternion& rotation)
{
	return self.getMultipliedBy(Matrix4::getRotation(rotation));
}

/// \brief Concatenates \p self with the rotation transform produced by \p rotation.
/// The concatenated rotation occurs before \p self.
inline void matrix4_rotate_by_quaternion(Matrix4& self, const Quaternion& rotation)
{
  self = matrix4_rotated_by_quaternion(self, rotation);
}

/// \brief Rotates \p self by \p rotation, using \p pivotpoint.
inline void matrix4_pivoted_rotate_by_quaternion(Matrix4& self, const Quaternion& rotation, const Vector3& pivotpoint)
{
  self.translateBy(pivotpoint);
  matrix4_rotate_by_quaternion(self, rotation);
  self.translateBy(-pivotpoint);
}

inline Vector3 quaternion_transformed_point(const Quaternion& quaternion, const Vector3& point)
{
  float xx = quaternion.x() * quaternion.x();
  float yy = quaternion.y() * quaternion.y();
  float zz = quaternion.z() * quaternion.z();
  float ww = quaternion.w() * quaternion.w();

  float xy2 = quaternion.x() * quaternion.y() * 2;
  float xz2 = quaternion.x() * quaternion.z() * 2;
  float xw2 = quaternion.x() * quaternion.w() * 2;
  float yz2 = quaternion.y() * quaternion.z() * 2;
  float yw2 = quaternion.y() * quaternion.w() * 2;
  float zw2 = quaternion.z() * quaternion.w() * 2;

	return Vector3(
    ww * point.x() + yw2 * point.z() - zw2 * point.y() + xx * point.x() + xy2 * point.y() + xz2 * point.z() - zz * point.x() - yy * point.x(),
    xy2 * point.x() + yy * point.y() + yz2 * point.z() + zw2 * point.x() - zz * point.y() + ww * point.y() - xw2 * point.z() - xx * point.y(),
    xz2 * point.x() + yz2 * point.y() + zz * point.z() - yw2 * point.x() - yy * point.z() + xw2 * point.y() - xx * point.z() + ww * point.z()
  );
}

/// \brief Constructs a pure-rotation transform from \p axis and \p angle (radians).
inline Matrix4 matrix4_rotation_for_axisangle(const Vector3& axis, float angle)
{
	return Matrix4::getRotation(Quaternion::createForAxisAngle(axis, angle));
}

/// \brief Rotates \p self about \p axis by \p angle.
inline void matrix4_rotate_by_axisangle(Matrix4& self, const Vector3& axis, float angle)
{
  self.multiplyBy(matrix4_rotation_for_axisangle(axis, angle));
}

/// \brief Rotates \p self about \p axis by \p angle using \p pivotpoint.
inline void matrix4_pivoted_rotate_by_axisangle(Matrix4& self, const Vector3& axis, float angle, const Vector3& pivotpoint)
{
  self.translateBy(pivotpoint);
  matrix4_rotate_by_axisangle(self, axis, angle);
  self.translateBy(-pivotpoint);
}
