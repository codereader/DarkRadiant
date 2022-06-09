#pragma once

/* greebo: This file contains the templated class definition of the three-component vector
 *
 * BasicVector3: A vector with three components of type <T>
 *
 * The BasicVector3 is equipped with the most important operators like *, *= and so on.
 *
 * Note: The most commonly used Vector3 is a BasicVector3<float>, this is also defined in this file
 *
 * Note: that the multiplication of a Vector3 with another one (Vector3*Vector3) does NOT
 * result in an inner product but in a component-wise scaling. Use the .dot() method to
 * execute an inner product of two vectors.
 */

#include <cmath>
#include <istream>
#include <ostream>
#include <sstream>

#include <float.h>
#include "math/pi.h"
#include "lrint.h"
#include "FloatTools.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

#undef Success // get rid of fuckwit X.h macro
#include <Eigen/Dense>

/// A 3-element vector of type T
template<typename T>
class BasicVector3
{
public:
    /// Eigen vector type to store a BasicVector3's data
    using Eigen_T = Eigen::Matrix<T, 3, 1>;

    // Public typedef to read the type of our elements
    using ElementType = T;

private:
    // Eigen vector for storage and calculations
    Eigen_T _v;

public:

    /// Initialise Vector with all zeroes.
    BasicVector3(): _v(0, 0, 0)
    {}

    /// Construct a BasicVector3 with the 3 provided components.
    BasicVector3(T x, T y, T z): _v(x, y, z)
    {}

    /// Construct directly from the underlying Eigen vector type
    BasicVector3(const Eigen_T& vec): _v(vec)
    {}

    /**
     * \brief Construct a BasicVector3 from a 3-element array.
     *
     * The array must be valid as no bounds checking is done.
     */
    BasicVector3(const T* array): _v(array[0], array[1], array[2])
    {}

    /// Construct from another BasicVector3 with a compatible element type
    template<typename U> BasicVector3(const BasicVector3<U>& other)
    : BasicVector3(static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()))
    {
    }

    /**
     * Named constructor, returning a vector on the unit sphere for the given spherical coordinates.
     */
    static BasicVector3<T> createForSpherical(T theta, T phi)
    {
        return BasicVector3<T>(
            cos(theta) * cos(phi),
            sin(theta) * cos(phi),
            sin(phi)
        );
    }

    /// Read-only access to the internal Eigen vector
    const Eigen_T& eigen() const { return _v; }

    /// Mutable access to the internal Eigen vector
    Eigen_T& eigen() { return _v; }

    /// Set all 3 components to the provided values.
    void set(T x, T y, T z)
    {
        _v = Eigen_T(x, y, z);
    }

    // Return mutable references to the vector components
    T& x() { return _v[0]; }
    T& y() { return _v[1]; }
    T& z() { return _v[2]; }

    // Return vector component values
    T x() const { return _v[0]; }
    T y() const { return _v[1]; }
    T z() const { return _v[2]; }

    /// Compare this BasicVector3 against another for equality.
    bool operator== (const BasicVector3& other) const {
        return (other.x() == x()
                && other.y() == y()
                && other.z() == z());
    }

    /// Compare this BasicVector3 against another for inequality.
    bool operator!= (const BasicVector3& other) const {
        return !(*this == other);
    }

    /// Return the componentwise negation of this vector
    BasicVector3<T> operator- () const
    {
        return BasicVector3<T>(-_v);
    }

    /// Return the Pythagorean length of this vector.
    double getLength() const
    {
        return _v.norm();
    }

    /// Return the squared length of this vector.
    T getLengthSquared() const
    {
        return _v.squaredNorm();
    }

    /**
     * \brief Normalise this vector in-place by scaling by the inverse of its
     * size.

     * \return The length of the vector before normalisation.
     */
    double normalise()
    {
        double length = getLength();
        _v.normalize();
        return length;
    }

    /// Return the result of normalising this vector
    BasicVector3<T> getNormalised() const
    {
        return BasicVector3<T>(_v.normalized());
    }

    /// Return dot product of this and another vector
    T dot(const BasicVector3<T>& other) const
    {
        return _v.dot(other._v);
    }

    /// Return the angle between <self> and <other>
    T angle(const BasicVector3<T>& other) const
    {
        // Get dot product of normalised vectors, ensuring it lies between -1
        // and 1.
        T dot = std::clamp(
            getNormalised().dot(other.getNormalised()), -1.0, 1.0
        );

        // Angle is the arccos of the dot product
        return acos(dot);
    }

    /// Return the cross product of this and another vector
    BasicVector3<T> cross(const BasicVector3<T>& other) const
    {
        return BasicVector3<T>(_v.cross(other._v));
    }

    /** Implicit cast to C-style array. This allows a Vector3 to be
     * passed directly to GL functions that expect an array (e.g.
     * glFloat3dv()). These functions implicitly provide operator[]
     * as well, since the C-style array provides this function.
     */

    operator const T* () const {
        return _v.data();
    }

    operator T* () {
        return _v.data();
    }

    /// Returns a "snapped" copy of this Vector, each component rounded to the given precision.
    BasicVector3<T> getSnapped(T snap) const
    {
        return BasicVector3<T>(float_snapped(x(), snap),
                               float_snapped(y(), snap),
                               float_snapped(z(), snap));
    }

    /// Snaps this vector to the given precision in place.
    void snap(T snap)
    {
        *this = getSnapped(snap);
    }
};

/// Multiply vector components with a scalar and return the result
template <
    typename T, typename S,
    typename = typename std::enable_if<std::is_arithmetic<S>::value, S>::type
>
BasicVector3<T> operator*(const BasicVector3<T>& v, S s)
{
    return BasicVector3<T>(v.eigen() * s);
}

/// Multiply vector components with a scalar and return the result
template <
    typename T, typename S,
    typename = typename std::enable_if<std::is_arithmetic<S>::value, S>::type
>
BasicVector3<T> operator*(S s, const BasicVector3<T>& v)
{
    return v * s;
}

/// Multiply vector components with a scalar and modify in place
template <typename T, typename S>
BasicVector3<T>& operator*=(BasicVector3<T>& v, S s)
{
    v.eigen() *= s;
    return v;
}

/// Divide vector by a scalar and return result
template<typename T, typename S>
BasicVector3<T> operator/ (const BasicVector3<T>& v, S s)
{
    return BasicVector3<T>(v.x() / s, v.y() / s, v.z() / s);
}

/// Divide vector by a scalar in place
template<typename T, typename S>
BasicVector3<T>& operator/= (BasicVector3<T>& v, S s)
{
    v = v / s;
    return v;
}

/// Divide a scalar by a vector and return result
template <
    typename S, typename T,
    typename = typename std::enable_if<std::is_arithmetic<S>::value, S>::type
>
BasicVector3<T> operator/(S s, const BasicVector3<T>& v)
{
    return BasicVector3<T>(s / v.x(), s / v.y(), s / v.z());
}

/// Divide a vector componentwise with another vector and return result
template <typename T>
BasicVector3<T> operator/(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.x() / v2.x(), v1.y() / v2.y(), v1.z() / v2.z());
}

/// Divide a vector componentwise with another vector, in place
template <typename T>
BasicVector3<T>& operator/=(BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1 = v1 / v2;
    return v1;
}

/// Componentwise addition of two vectors
template <typename T>
BasicVector3<T> operator+(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.eigen() + v2.eigen());
}

/// Componentwise vector addition in place
template <typename T>
BasicVector3<T>& operator+=(BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1.eigen() += v2.eigen();
    return v1;
}

/// Componentwise subtraction of two vectors
template <typename T>
BasicVector3<T> operator-(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.eigen() - v2.eigen());
}

/// Componentwise vector subtraction in place
template<typename T>
BasicVector3<T>& operator-= (BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1.eigen() -= v2.eigen();
    return v1;
}

/// Componentwise (Hadamard) product of two vectors
template <typename T>
BasicVector3<T> operator*(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.x() * v2.x(), v1.y() * v2.y(), v1.z() * v2.z());
}

/// Componentwise (Hadamard) product of two vectors, in place
template<typename T>
BasicVector3<T>& operator*= (BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1 = v1 * v2;
    return v1;
}

/// Stream insertion for BasicVector3
template<typename T>
inline std::ostream& operator<<(std::ostream& st, BasicVector3<T> vec)
{
    return st << vec.x() << " " << vec.y() << " " << vec.z();
}

/// Stream extraction for BasicVector3
template<typename T>
inline std::istream& operator>>(std::istream& st, BasicVector3<T>& vec)
{
    return st >> std::skipws >> vec.x() >> vec.y() >> vec.z();
}

namespace math
{

/// Epsilon equality test for BasicVector3
template <typename T>
inline bool isNear(const BasicVector3<T>& v1, const BasicVector3<T>& v2, double epsilon)
{
    BasicVector3<T> diff = v1 - v2;
    return std::abs(diff.x()) < epsilon && std::abs(diff.y()) < epsilon
        && std::abs(diff.z()) < epsilon;
}

/// Test if two vectors are parallel (in similar or opposite directions)
template<typename T>
bool isParallel(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    T angle = v1.angle(v2);
    return float_equal_epsilon(angle, 0.0, 0.001)
        || float_equal_epsilon(angle, PI, 0.001);
}

/// Return the midpoint of two vectors
template<typename T>
BasicVector3<T> midPoint(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return (v1 + v2) * 0.5;
}

/// Return human readable debug string (pretty print)
template<typename T> std::string pp(const BasicVector3<T>& v)
{
    std::stringstream ss;
    ss << "[" << v.x() << ", " << v.y() << ", " << v.z() << "]";
    return ss.str();
}

}

// ==========================================================================================

// A 3-element vector stored in double-precision floating-point.
typedef BasicVector3<double> Vector3;

// A 3-element vector (single-precision variant)
typedef BasicVector3<float> Vector3f;

// =============== Vector3 Constants ==================================================

const Vector3 g_vector3_identity(0, 0, 0);
const Vector3 g_vector3_axis_x(1, 0, 0);
const Vector3 g_vector3_axis_y(0, 1, 0);
const Vector3 g_vector3_axis_z(0, 0, 1);

const Vector3 g_vector3_axes[3] = { g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z };

#ifdef _MSC_VER
#pragma warning(pop)
#endif
