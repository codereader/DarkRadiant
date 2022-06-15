#pragma once

#include <sstream>

/* greebo: This file contains the templated class definition of the three-component vector
 *
 * BasicVector4: A vector with three components of type <T>
 *
 * The BasicVector4 is equipped with the most important operators like *, *= and so on.
 *
 * Note: The most commonly used Vector4 is a BasicVector4<float>, this is also defined in this file
 *
 * Note: that the multiplication of a Vector4 with another one (Vector4*Vector4) does NOT
 * result in an inner product but in a component-wise scaling. Use the .dot() method to
 * execute an inner product of two vectors.
 */

#include "lrint.h"
#include "FloatTools.h"
#include "Vector3.h"

/// A 4-element vector of type <T>
template<typename T>
class BasicVector4
{
public:
    /// Eigen vector type to store data
    using Eigen_T = Eigen::Matrix<T, 4, 1>;

    /// Public typedef to read the type of our elements
    using ElementType = T;

private:
    // Eigen vector for storage and calculations
    Eigen_T _v;

public:

    /// Default construct a zero vector
    BasicVector4(): _v(0, 0, 0, 0)
    {}

    /**
     * \brief Construct a BasicVector4 out of 4 explicit values.
     *
     * If the W coordinate is unspecified it will default to 1.
     */
    BasicVector4(T x_, T y_, T z_, T w_ = 1)
    : _v(x_, y_, z_, w_)
    {}

    /// Construct from another BasicVector4 with a compatible element type
    template<typename U> BasicVector4(const BasicVector4<U>& other)
    : BasicVector4(static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()), static_cast<T>(other.w()))
    {}

    /// Construct from a BasicVector3 of compatible element type, plus an optional W value
    template <typename U, typename W = float>
    BasicVector4(const BasicVector3<U>& other, W w_ = 1.0f)
    {
        _v[0] = static_cast<T>(other.x());
        _v[1] = static_cast<T>(other.y());
        _v[2] = static_cast<T>(other.z());
        _v[3] = static_cast<T>(w_);
    }

    /// Construct directly from Eigen type
    BasicVector4(const Eigen_T& vec): _v(vec)
    {}

    /// Return the underlying Eigen vector
    const Eigen_T& eigen() const { return _v; }

    // Return non-constant references to the components
    T& x() { return _v[0]; }
    T& y() { return _v[1]; }
    T& z() { return _v[2]; }
    T& w() { return _v[3]; }

    // Return constant references to the components
    const T& x() const { return _v[0]; }
    const T& y() const { return _v[1]; }
    const T& z() const { return _v[2]; }
    const T& w() const { return _v[3]; }

    /// Dot product this BasicVector4 with another vector
    T dot(const BasicVector4<T>& other) const {
        return x() * other.x()
             + y() * other.y()
             + z() * other.z()
             + w() * other.w();
    }

    /// Truncate this Vector4 into a Vector3 (no division by W)
    BasicVector3<T> getVector3() const {
        return BasicVector3<T>(x(), y(), z());
    }

    /// Project homogeneous BasicVector4 into 3 dimensions by dividing by W
    BasicVector3<T> getProjected() const
    {
        return getVector3() / w();
    }

    /// Cast to const raw array
    operator const T* () const { return _v.data(); }

    /// Cast to non-const raw array
    operator T* () { return _v.data(); }
};

/// Equality comparison for BasicVector4
template<typename T>
bool operator== (const BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    return (v1.x() == v2.x()
            && v1.y() == v2.y()
            && v1.z() == v2.z()
            && v1.w() == v2.w());
}

/// Inequality comparison for BasicVector4
template<typename T>
bool operator!= (const BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    return !(v1 == v2);
}

/// Componentwise addition of two vectors
template <typename T>
BasicVector4<T> operator+(const BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    return BasicVector4<T>(v1.x() + v2.x(),
                           v1.y() + v2.y(),
                           v1.z() + v2.z(),
                           v1.w() + v2.w());
}

template <typename T>
BasicVector4<T>& operator+=(BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    v1.x() += v2.x();
    v1.y() += v2.y();
    v1.z() += v2.z();
    v1.w() += v2.w();
    return v1;
}

/// Componentwise subtraction of two vectors
template<typename T>
BasicVector4<T> operator- (const BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    return BasicVector4<T>(v1.x() - v2.x(),
                           v1.y() - v2.y(),
                           v1.z() - v2.z(),
                           v1.w() - v2.w());
}

template<typename T>
BasicVector4<T>& operator-= (BasicVector4<T>& v1, const BasicVector4<T>& v2)
{
    v1.x() -= v2.x();
    v1.y() -= v2.y();
    v1.z() -= v2.z();
    v1.w() -= v2.w();
    return v1;
}

/// Multiply BasicVector4 with a scalar
template <
    typename T, typename S,
    typename = typename std::enable_if<std::is_arithmetic<S>::value, S>::type
>
BasicVector4<T> operator*(const BasicVector4<T>& v, S s)
{
    return BasicVector4<T>(v.x() * s, v.y() * s, v.z() * s, v.w() * s);
}

/// Multiply BasicVector4 with a scalar
template <
    typename T, typename S,
    typename = typename std::enable_if<std::is_arithmetic<S>::value, S>::type
>
BasicVector4<T> operator*(S s, const BasicVector4<T>& v)
{
    return v * s;
}

/// Multiply BasicVector4 with a scalar and modify in place
template<typename T, typename S>
BasicVector4<T>& operator*= (BasicVector4<T>& v, S s)
{
    v.x() *= s;
    v.y() *= s;
    v.z() *= s;
    v.w() *= s;
    return v;
}

/// Divide and assign BasicVector4 by a scalar
template<typename T, typename S>
BasicVector4<T>& operator/= (BasicVector4<T>& v, S s)
{
    v.x() /= s;
    v.y() /= s;
    v.z() /= s;
    v.w() /= s;
    return v;
}

/// Divide a BasicVector4 by a scalar
template<typename T, typename S>
BasicVector4<T> operator/ (const BasicVector4<T>& v, S s)
{
    auto result = v;
    result /= s;
    return result;
}

/// Stream insertion for BasicVector4
template<typename T>
inline std::ostream& operator<<(std::ostream& st, BasicVector4<T> vec)
{
    return st << vec.x() << " " << vec.y() << " " << vec.z() << " " << vec.w();
}

/// Stream extraction for BasicVector4
template<typename T>
inline std::istream& operator>>(std::istream& st, BasicVector4<T>& vec)
{
    return st >> std::skipws >> vec.x() >> vec.y() >> vec.z() >> vec.w();
}

// A 4-element vector stored in double-precision floating-point.
typedef BasicVector4<double> Vector4;

// A 4-element vector stored in single-precision floating-point.
typedef BasicVector4<float> Vector4f;

namespace math
{

/// Epsilon equality test for BasicVector4
template <typename T>
inline bool isNear(const BasicVector4<T>& v1, const BasicVector4<T>& v2, double epsilon)
{
    BasicVector4<T> diff = v1 - v2;
    return std::abs(diff.x()) < epsilon && std::abs(diff.y()) < epsilon
        && std::abs(diff.z()) < epsilon && std::abs(diff.w()) < epsilon;
}

/**
 * \brief Return a readable (pretty-printed) string representation of a
 * BasicVector4.
 *
 * We need a dedicated function for this because the standard operator<< is
 * already used for serialisation to the less readable space-separated text
 * format.
 */
template<typename T> std::string pp(const BasicVector4<T>& v)
{
    std::stringstream ss;
    ss << "(" << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << ")";
    return ss.str();
}

}