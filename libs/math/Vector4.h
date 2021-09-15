#pragma once

#include <sstream>

/* greebo: This file contains the templated class definition of the three-component vector
 *
 * BasicVector4: A vector with three components of type <Element>
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

/// A 4-element vector of type <Element>
template<typename Element>
class BasicVector4
{
    // The components of this vector
    Element _v[4];

public:
    // Public typedef to read the type of our elements
    typedef Element ElementType;

    // Constructor (no arguments)
    BasicVector4() {
        _v[0] = 0;
        _v[1] = 0;
        _v[2] = 0;
        _v[3] = 0;
    }

    /**
     * \brief Construct a BasicVector4 out of 4 explicit values.
     *
     * If the W coordinate is unspecified it will default to 1.
     */
    BasicVector4(Element x_, Element y_, Element z_, Element w_ = 1)
    {
        _v[0] = x_;
        _v[1] = y_;
        _v[2] = z_;
        _v[3] = w_;
    }

    // Construct a BasicVector4 out of a Vector3 plus a W value (default 1)
    BasicVector4(const BasicVector3<Element>& other, Element w_ = 1)
    {
        _v[0] = other.x();
        _v[1] = other.y();
        _v[2] = other.z();
        _v[3] = w_;
    }

    // Return non-constant references to the components
    Element& x() { return _v[0]; }
    Element& y() { return _v[1]; }
    Element& z() { return _v[2]; }
    Element& w() { return _v[3]; }

    // Return constant references to the components
    const Element& x() const { return _v[0]; }
    const Element& y() const { return _v[1]; }
    const Element& z() const { return _v[2]; }
    const Element& w() const { return _v[3]; }

    /**
     * \brief Return a readable (pretty-printed) string representation of the
     * vector.
     *
     * We need a dedicated function for this because the standard operator<< is
     * already used for serialisation to the less readable space-separated text
     * format.
     */
    std::string pp() const
    {
        std::stringstream ss;
        ss << "(" << x() << ", " << y() << ", " << z() << ", " << w() << ")";
        return ss.str();
    }

    /// Dot product this BasicVector4 with another vector
    Element dot(const BasicVector4<Element>& other) const {
        return x() * other.x()
             + y() * other.y()
             + z() * other.z()
             + w() * other.w();
    }

    /** Project this homogeneous Vector4 into a Cartesian Vector3
     * by dividing by w.
     *
     * @returns
     * A Vector3 representing the Cartesian equivalent of this
     * homogeneous vector.
     */
    BasicVector3<Element> getProjected() {
        return BasicVector3<Element>(
            _v[0] / _v[3],
            _v[1] / _v[3],
            _v[2] / _v[3]);
    }

    /// Cast to const raw array
    operator const Element* () const { return _v; }

    // Cast to non-const raw array
    operator Element* () { return _v; }

    /*  Cast this Vector4 onto a Vector3, both const and non-const
     */
    BasicVector3<Element>& getVector3() {
        return *reinterpret_cast<BasicVector3<Element>*>(_v);
    }

    const BasicVector3<Element>& getVector3() const {
        return *reinterpret_cast<const BasicVector3<Element>*>(_v);
    }
}; // BasicVector4

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

}