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

/// A 3-element vector of type T
template<typename T>
class BasicVector3
{
    // The actual values of the vector, an array containing 3 values of type
    // T
    T _v[3];

public:
    // Public typedef to read the type of our elements
    typedef T ElementType;

    /// Initialise Vector with all zeroes.
    BasicVector3()
    {
        _v[0] = 0;
        _v[1] = 0;
        _v[2] = 0;
    }

    /// Construct a BasicVector3 with the 3 provided components.
    BasicVector3(const T& x_, const T& y_, const T& z_) {
        x() = x_;
        y() = y_;
        z() = z_;
    }

    /**
     * \brief Construct a BasicVector3 from a 3-element array.
     *
     * The array must be valid as no bounds checking is done.
     */
    BasicVector3(const T* array)
    {
        for (int i = 0; i < 3; ++i)
            _v[i] = array[i];
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

    /// Set all 3 components to the provided values.
    void set(const T& x, const T& y, const T& z) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
    }

    // Return NON-CONSTANT references to the vector components
    T& x() { return _v[0]; }
    T& y() { return _v[1]; }
    T& z() { return _v[2]; }

    // Return CONSTANT references to the vector components
    const T& x() const { return _v[0]; }
    const T& y() const { return _v[1]; }
    const T& z() const { return _v[2]; }

    /// Return human readable debug string (pretty print)
    std::string pp() const
    {
        std::stringstream ss;
        ss << "[" << x() << ", " << y() << ", " << z() << "]";
        return ss.str();
    }

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
        return BasicVector3<T>(-_v[0], -_v[1], -_v[2]);
    }

    /*  Define the division operators / and /= with another Vector3 of type OtherElement
     *  The vectors are divided element-wise
     */
    template<typename OtherElement>
    BasicVector3<T> operator/ (const BasicVector3<OtherElement>& other) const {
        return BasicVector3<T>(
            _v[0] / static_cast<T>(other.x()),
            _v[1] / static_cast<T>(other.y()),
            _v[2] / static_cast<T>(other.z())
        );
    }

    template<typename OtherElement>
	BasicVector3<T>& operator/= (const BasicVector3<OtherElement>& other) {
        _v[0] /= static_cast<T>(other.x());
        _v[1] /= static_cast<T>(other.y());
        _v[2] /= static_cast<T>(other.z());
		return *this;
    }

    /*  Define the scalar divisions / and /=
     */
    template<typename OtherElement>
    BasicVector3<T> operator/ (const OtherElement& other) const {
        T divisor = static_cast<T>(other);
        return BasicVector3<T>(
            _v[0] / divisor,
            _v[1] / divisor,
            _v[2] / divisor
        );
    }

    template<typename OtherElement>
	BasicVector3<T>& operator/= (const OtherElement& other) {
        T divisor = static_cast<T>(other);
        _v[0] /= divisor;
        _v[1] /= divisor;
        _v[2] /= divisor;
		return *this;
    }

    /// Return the Pythagorean length of this vector.
    float getLength() const {
        float lenSquared = getLengthSquared();
        return sqrt(lenSquared);
    }

    /// Return the squared length of this vector.
    float getLengthSquared() const {
        float lenSquared = float(_v[0]) * float(_v[0]) +
                            float(_v[1]) * float(_v[1]) +
                            float(_v[2]) * float(_v[2]);
        return lenSquared;
    }

    /**
     * \brief Normalise this vector in-place by scaling by the inverse of its
     * size.

     * \return The length of the vector before normalisation.
     */
    T normalise()
    {
        T length = getLength();
        T inverseLength = 1/length;

        _v[0] *= inverseLength;
        _v[1] *= inverseLength;
        _v[2] *= inverseLength;

        return length;
    }

    /// Return the result of normalising this vector
    BasicVector3<T> getNormalised() const
    {
        BasicVector3<T> copy = *this;
        copy.normalise();
        return copy;
    }

    // Returns a vector with the reciprocal values of each component
    BasicVector3<T> getInversed() {
        return BasicVector3<T>(
            1.0f / _v[0],
            1.0f / _v[1],
            1.0f / _v[2]
        );
    }

    /* Scalar product this vector with another Vector3,
     * returning the projection of <self> onto <other>
     *
     * @param other
     * The Vector3 to dot-product with this Vector3.
     *
     * @returns
     * The inner product (a scalar): a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
     */

    template<typename OtherT>
    T dot(const BasicVector3<OtherT>& other) const {
        return  T(_v[0] * other.x() +
                        _v[1] * other.y() +
                        _v[2] * other.z());
    }

    /* Returns the angle between <self> and <other>
     *
     * @returns
     * The angle as defined by the arccos( (a*b) / (|a|*|b|) )
     */
    template<typename OtherT>
    T angle(const BasicVector3<OtherT>& other) const
    {
        BasicVector3<T> aNormalised = getNormalised();
        BasicVector3<OtherT> otherNormalised = other.getNormalised();

        T dot = aNormalised.dot(otherNormalised);

        // greebo: Sanity correction: Make sure the dot product
        // of two normalised vectors is not greater than 1
        if (dot > 1.0)
        {
            dot = 1;
        }
        else if (dot < -1.0)
        {
            dot = -1.0;
        }

        return acos(dot);
    }

    /* Cross-product this vector with another Vector3, returning the result
     * in a new Vector3.
     *
     * @param other
     * The Vector3 to cross-product with this Vector3.
     *
     * @returns
     * The cross-product of the two vectors.
     */

    template<typename OtherT>
    BasicVector3<T> crossProduct(const BasicVector3<OtherT>& other) const {
        return BasicVector3<T>(
            _v[1] * other.z() - _v[2] * other.y(),
            _v[2] * other.x() - _v[0] * other.z(),
            _v[0] * other.y() - _v[1] * other.x());
    }

    /** Implicit cast to C-style array. This allows a Vector3 to be
     * passed directly to GL functions that expect an array (e.g.
     * glFloat3dv()). These functions implicitly provide operator[]
     * as well, since the C-style array provides this function.
     */

    operator const T* () const {
        return _v;
    }

    operator T* () {
        return _v;
    }

    template<typename OtherT>
    bool isParallel(const BasicVector3<OtherT>& other) const
	{
        return float_equal_epsilon(angle(other), 0.0, 0.001) ||
			   float_equal_epsilon(angle(other), math::PI, 0.001);
    }

    // Returns the mid-point of this vector and the other one
    BasicVector3<T> mid(const BasicVector3<T>& other) const
    {
        return (*this + other) * 0.5f;
    }

    // Returns true if this vector is equal to the other one, considering the given tolerance.
    template<typename OtherElement, typename Epsilon>
    bool isEqual(const BasicVector3<OtherElement>& other, Epsilon epsilon) const
    {
        return float_equal_epsilon(x(), other.x(), epsilon) &&
               float_equal_epsilon(y(), other.y(), epsilon) &&
               float_equal_epsilon(z(), other.z(), epsilon);
    }

    /**
     * Returns a "snapped" copy of this Vector, each component rounded to integers.
     */
    BasicVector3<T> getSnapped() const
    {
        return BasicVector3<T>(
            static_cast<T>(float_to_integer(x())),
            static_cast<T>(float_to_integer(y())),
            static_cast<T>(float_to_integer(z()))
        );
    }

    /**
     * Returns a "snapped" copy of this Vector, each component rounded to the given precision.
     */
    template<typename OtherElement>
    BasicVector3<T> getSnapped(const OtherElement& snap) const
    {
        return BasicVector3<T>(
            static_cast<T>(float_snapped(x(), snap)),
            static_cast<T>(float_snapped(y(), snap)),
            static_cast<T>(float_snapped(z(), snap))
        );
    }

    /**
     * Snaps this vector to the given precision in place.
     */
    template<typename OtherElement>
    void snap(const OtherElement& snap)
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
    T factor = static_cast<T>(s);
    return BasicVector3<T>(v.x() * factor, v.y() * factor, v.z() * factor);
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
    v = v * s;
    return v;
}

/// Componentwise addition of two vectors
template <typename T>
BasicVector3<T> operator+(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.x() + v2.x(), v1.y() + v2.y(), v1.z() + v2.z());
}

/// Componentwise vector addition in place
template <typename T>
BasicVector3<T>& operator+=(BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1 = v1 + v2;
    return v1;
}

/// Componentwise subtraction of two vectors
template <typename T>
BasicVector3<T> operator-(const BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    return BasicVector3<T>(v1.x() - v2.x(), v1.y() - v2.y(), v1.z() - v2.z());
}

/// Componentwise vector subtraction in place
template<typename T>
BasicVector3<T>& operator-= (BasicVector3<T>& v1, const BasicVector3<T>& v2)
{
    v1 = v1 - v2;
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
