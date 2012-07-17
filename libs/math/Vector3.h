#pragma once

/* greebo: This file contains the templated class definition of the three-component vector
 *
 * BasicVector3: A vector with three components of type <Element>
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

#include <float.h>
#include "math/pi.h"
#include "lrint.h"
#include "FloatTools.h"

/// A 3-element vector of type Element
template<typename Element>
class BasicVector3
{
    // The actual values of the vector, an array containing 3 values of type
    // Element
    Element _v[3];

public:
    // Public typedef to read the type of our elements
    typedef Element ElementType;

    /**
     * Default constructor. Initialise Vector with all zeroes.
     */
    BasicVector3()
    {
        _v[0] = 0;
        _v[1] = 0;
        _v[2] = 0;
    }

    /// Construct a BasicVector3 with the 3 provided components.
    BasicVector3(const Element& x_, const Element& y_, const Element& z_) {
        x() = x_;
        y() = y_;
        z() = z_;
    }

    /** Construct a BasicVector3 from a 3-element array. The array must be
     * valid as no bounds checking is done.
     */
    BasicVector3(const Element* array)
    {
        for (int i = 0; i < 3; ++i)
            _v[i] = array[i];
    }

    /**
     * Named constructor, returning a vector on the unit sphere for the given spherical coordinates.
     */
    static BasicVector3<Element> createForSpherical(Element theta, Element phi)
    {
        return BasicVector3<Element>(
            cos(theta) * cos(phi),
            sin(theta) * cos(phi),
            sin(phi)
        );
    }

    /** Set all 3 components to the provided values.
     */
    void set(const Element& x, const Element& y, const Element& z) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
    }

    /**
     * Check if this Vector is valid. A Vector is invalid if any of its
     * components are NaN.
     */
    bool isValid() const {
        return !isnan(_v[0]) && !isnan(_v[1]) && !isnan(_v[2]);
    }

    // Return NON-CONSTANT references to the vector components
    Element& x() { return _v[0]; }
    Element& y() { return _v[1]; }
    Element& z() { return _v[2]; }

    // Return CONSTANT references to the vector components
    const Element& x() const { return _v[0]; }
    const Element& y() const { return _v[1]; }
    const Element& z() const { return _v[2]; }

    /** Compare this BasicVector3 against another for equality.
     */
    bool operator== (const BasicVector3& other) const {
        return (other.x() == x()
                && other.y() == y()
                && other.z() == z());
    }

    /** Compare this BasicVector3 against another for inequality.
     */
    bool operator!= (const BasicVector3& other) const {
        return !(*this == other);
    }

    /*  Define the negation operator -
     *  All the vector's components are negated
     */
    BasicVector3<Element> operator- () const {
        return BasicVector3<Element>(
            -_v[0],
            -_v[1],
            -_v[2]
        );
    }

    /*  Define the addition operators + and += with any other BasicVector3 of type OtherElement
     *  The vectors are added to each other element-wise
     */
    template<typename OtherElement>
    BasicVector3<Element> operator+ (const BasicVector3<OtherElement>& other) const {
        return BasicVector3<Element>(
            _v[0] + static_cast<Element>(other.x()),
            _v[1] + static_cast<Element>(other.y()),
            _v[2] + static_cast<Element>(other.z())
        );
    }

    template<typename OtherElement>
    void operator+= (const BasicVector3<OtherElement>& other) {
        _v[0] += static_cast<Element>(other.x());
        _v[1] += static_cast<Element>(other.y());
        _v[2] += static_cast<Element>(other.z());
    }

    /*  Define the substraction operators - and -= with any other BasicVector3 of type OtherElement
     *  The vectors are substracted from each other element-wise
     */
    template<typename OtherElement>
    BasicVector3<Element> operator- (const BasicVector3<OtherElement>& other) const {
        return BasicVector3<Element>(
            _v[0] - static_cast<Element>(other.x()),
            _v[1] - static_cast<Element>(other.y()),
            _v[2] - static_cast<Element>(other.z())
        );
    }

    template<typename OtherElement>
    void operator-= (const BasicVector3<OtherElement>& other) {
        _v[0] -= static_cast<Element>(other.x());
        _v[1] -= static_cast<Element>(other.y());
        _v[2] -= static_cast<Element>(other.z());
    }

    /*  Define the multiplication operators * and *= with another Vector3 of type OtherElement
     *
     *  The vectors are multiplied element-wise
     *
     *  greebo: This is mathematically kind of senseless, as this is a mixture of
     *  a dot product and scalar multiplication. It can be used to scale each
     *  vector component by a different factor, so maybe this comes in handy.
     */
    template<typename OtherElement>
    BasicVector3<Element> operator* (const BasicVector3<OtherElement>& other) const {
        return BasicVector3<Element>(
            _v[0] * static_cast<Element>(other.x()),
            _v[1] * static_cast<Element>(other.y()),
            _v[2] * static_cast<Element>(other.z())
        );
    }

    template<typename OtherElement>
    void operator*= (const BasicVector3<OtherElement>& other) {
        _v[0] *= static_cast<Element>(other.x());
        _v[1] *= static_cast<Element>(other.y());
        _v[2] *= static_cast<Element>(other.z());
    }

    /*  Define the multiplications * and *= with a scalar
     */
    template<typename OtherElement>
    BasicVector3<Element> operator* (const OtherElement& other) const {
        Element factor = static_cast<Element>(other);
        return BasicVector3<Element>(
            _v[0] * factor,
            _v[1] * factor,
            _v[2] * factor
        );
    }

    template<typename OtherElement>
    void operator*= (const OtherElement& other) {
        Element factor = static_cast<Element>(other);
        _v[0] *= factor;
        _v[1] *= factor;
        _v[2] *= factor;
    }

    /*  Define the division operators / and /= with another Vector3 of type OtherElement
     *  The vectors are divided element-wise
     */
    template<typename OtherElement>
    BasicVector3<Element> operator/ (const BasicVector3<OtherElement>& other) const {
        return BasicVector3<Element>(
            _v[0] / static_cast<Element>(other.x()),
            _v[1] / static_cast<Element>(other.y()),
            _v[2] / static_cast<Element>(other.z())
        );
    }

    template<typename OtherElement>
    void operator/= (const BasicVector3<OtherElement>& other) {
        _v[0] /= static_cast<Element>(other.x());
        _v[1] /= static_cast<Element>(other.y());
        _v[2] /= static_cast<Element>(other.z());
    }

    /*  Define the scalar divisions / and /=
     */
    template<typename OtherElement>
    BasicVector3<Element> operator/ (const OtherElement& other) const {
        Element divisor = static_cast<Element>(other);
        return BasicVector3<Element>(
            _v[0] / divisor,
            _v[1] / divisor,
            _v[2] / divisor
        );
    }

    template<typename OtherElement>
    void operator/= (const OtherElement& other) {
        Element divisor = static_cast<Element>(other);
        _v[0] /= divisor;
        _v[1] /= divisor;
        _v[2] /= divisor;
    }

    /*
     * Mathematical operations on the BasicVector3
     */

    /** Return the length of this vector.
     *
     * @returns
     * The Pythagorean length of this vector.
     */
    float getLength() const {
        float lenSquared = getLengthSquared();
        return sqrt(lenSquared);
    }

    /** Return the squared length of this vector.
     */
    float getLengthSquared() const {
        float lenSquared = float(_v[0]) * float(_v[0]) +
                            float(_v[1]) * float(_v[1]) +
                            float(_v[2]) * float(_v[2]);
        return lenSquared;
    }

    /**
     * Return a new BasicVector3 equivalent to the normalised version of this
     * BasicVector3 (scaled by the inverse of its size)
     */
    BasicVector3<Element> getNormalised() const {
        return (*this)/getLength();
    }

    /**
     * Normalise this vector in-place by scaling by the inverse of its size.
     * Returns the length it had before normalisation.
     */
    float normalise()
    {
        float length = getLength();
        float inverseLength = 1/length;

        _v[0] *= inverseLength;
        _v[1] *= inverseLength;
        _v[2] *= inverseLength;
        
        return length;
    }

    // Returns a vector with the reciprocal values of each component
    BasicVector3<Element> getInversed() {
        return BasicVector3<Element>(
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
    Element dot(const BasicVector3<OtherT>& other) const {
        return  Element(_v[0] * other.x() +
                        _v[1] * other.y() +
                        _v[2] * other.z());
    }

    /* Returns the angle between <self> and <other>
     *
     * @returns
     * The angle as defined by the arccos( (a*b) / (|a|*|b|) )
     */
    template<typename OtherT>
    Element angle(const BasicVector3<OtherT>& other) const {
        BasicVector3<Element> aNormalised = getNormalised();
        BasicVector3<OtherT> otherNormalised = other.getNormalised();

        Element dot = aNormalised.dot(otherNormalised);

        // greebo: Sanity correction: Make sure the dot product
        // of two normalised vectors is not greater than 1
        if (dot > 1.0) {
            dot = 1;
        }

        return acos( dot );
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
    BasicVector3<Element> crossProduct(const BasicVector3<OtherT>& other) const {
        return BasicVector3<Element>(
            _v[1] * other.z() - _v[2] * other.y(),
            _v[2] * other.x() - _v[0] * other.z(),
            _v[0] * other.y() - _v[1] * other.x());
    }

    /** Implicit cast to C-style array. This allows a Vector3 to be
     * passed directly to GL functions that expect an array (e.g.
     * glFloat3dv()). These functions implicitly provide operator[]
     * as well, since the C-style array provides this function.
     */

    operator const Element* () const {
        return _v;
    }

    operator Element* () {
        return _v;
    }

    // Returns the maximum absolute value of the components
    Element max() const {
        return std::max(fabs(_v[0]), std::max(fabs(_v[1]), fabs(_v[2])));
    }

    // Returns the minimum absolute value of the components
    Element min() const {
        return std::min(fabs(_v[0]), std::min(fabs(_v[1]), fabs(_v[2])));
    }

    template<typename OtherT>
    bool isParallel(const BasicVector3<OtherT>& other) const
	{
        return float_equal_epsilon(angle(other), 0.0, 0.001) || 
			   float_equal_epsilon(angle(other), c_pi, 0.001);
    }

    // Swaps all components with the other vector
    template<typename OtherElement>
    void swap(BasicVector3<OtherElement>& other)
    {
        std::swap(x(), other.x());
        std::swap(y(), other.y());
        std::swap(z(), other.z());
    }

    // Returns the mid-point of this vector and the other one
    BasicVector3<Element> mid(const BasicVector3<Element>& other) const
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
    BasicVector3<Element> getSnapped() const
    {
        return BasicVector3<Element>(
            static_cast<Element>(float_to_integer(x())),
            static_cast<Element>(float_to_integer(y())),
            static_cast<Element>(float_to_integer(z()))
        );
    }

    /**
     * Snaps this vector to integer values in place.
     */
    void snap()
    {
        *this = getSnapped();
    }

    /**
     * Returns a "snapped" copy of this Vector, each component rounded to the given precision.
     */
    template<typename OtherElement>
    BasicVector3<Element> getSnapped(const OtherElement& snap) const
    {
        return BasicVector3<Element>(
            static_cast<Element>(float_snapped(x(), snap)),
            static_cast<Element>(float_snapped(y(), snap)),
            static_cast<Element>(float_snapped(z(), snap))
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
