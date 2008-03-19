#ifndef VECTOR3_H_
#define VECTOR3_H_

/* greebo: This file contains the templated class definition of the three-component vector
 * 
 * BasicVector3: A vector with three components of type <Element>
 * 
 * The BasicVector3 is equipped with the most important operators like *, *= and so on.
 * 
 * Note: The most commonly used Vector3 is a BasicVector3<double>, this is also defined in this file 
 *  
 * Note: that the multiplication of a Vector3 with another one (Vector3*Vector3) does NOT
 * result in an inner product but in a component-wise scaling. Use the .dot() method to
 * execute an inner product of two vectors. 
 */

#include <sstream>
#include <string>
#include <cmath>
#include <float.h>
#include "math/pi.h"
#include "lrint.h"

#include <boost/format.hpp>

// BasicVector3: A 3-element vector of type Element
template<typename Element>
class BasicVector3 {
	
	// The actual values of the vector, an array containing 3 values of type 
	// Element
	Element _v[3];
  
public:

	/**
	 * Default constructor. Initialise Vector with all zeroes.
	 */
  	BasicVector3() {
		_v[0] = 0;
		_v[1] = 0;
		_v[2] = 0;  	
  	}

	// Templated copy constructor
	template<typename OtherElement>
	BasicVector3(const BasicVector3<OtherElement>& other) {
		x() = static_cast<Element>(other.x());
		y() = static_cast<Element>(other.y());
		z() = static_cast<Element>(other.z());
	}

    /** Construct a BasicVector3 with the 3 provided components.
     */
    BasicVector3(const Element& x_, const Element& y_, const Element& z_) {
        x() = x_;
        y() = y_;
        z() = z_;
    }

	/** Construct a BasicVector3 from a 3-element array. The array must be
	 * valid as no bounds checking is done.
	 */
	BasicVector3(const Element* array) {
		for (int i = 0; i < 3; ++i)
			_v[i] = array[i];
	}
	 
    /** Construct a BasicVector3 by parsing the supplied string. The string
     * must contain 3 numeric values separated by whitespace.
     *
     * @param str
     * The string from which component values are extracted.
     */
     
    BasicVector3(const std::string& str) {
    	// Initialise the vector with 0, in case the string parse fails
    	_v[0] = _v[1] = _v[2] = 0;
    	// Use a stringstream to parse the string
        std::stringstream strm(str);
        strm << std::skipws;
        strm >> x();
        strm >> y();
        strm >> z();
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
		return !isnan(_v[0]) && !isnan(_v[0]) && !isnan(_v[0]);		
	}

	// Return NON-CONSTANT references to the vector components
  	Element& x() { 
  		return _v[0]; 
  	}
  	Element& y() {
    	return _v[1];
  	}
  	Element& z() {
    	return _v[2];
  	}
  
  	// Return CONSTANT references to the vector components
  	const Element& x() const {  
  		return _v[0]; 
  	}  
  	const Element& y() const {
    	return _v[1];
  	}  
  	const Element& z() const {
    	return _v[2];
  	}
  
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
	
	/*	Define the negation operator - 
	 *  All the vector's components are negated
	 */
	BasicVector3<Element> operator- () const {
		return BasicVector3<Element>(
			-_v[0],
			-_v[1],
			-_v[2]
		);
	}
	
	/*	Define the addition operators + and += with any other BasicVector3 of type OtherElement
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
	
	/*	Define the substraction operators - and -= with any other BasicVector3 of type OtherElement
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
	
	
	/*	Define the multiplication operators * and *= with another Vector3 of type OtherElement
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
	
	
	/*	Define the multiplications * and *= with a scalar
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
	
	/*	Define the division operators / and /= with another Vector3 of type OtherElement
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
	
	/*	Define the scalar divisions / and /= 
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
  
	/** Cast to std::string, formats vector correctly for use as a
	 * keyval -- "x y z".
	 */
	 
	operator std::string() const {
		return (boost::format("%s %s %s")
				 % _v[0]
				 % _v[1]
				 % _v[2]).str();
	}

	/* 
	 * Mathematical operations on the BasicVector3 
	 */
	 
	/** Return the length of this vector.
     * 
     * @returns
     * The Pythagorean length of this vector.
     */
	double getLength() const {
		double lenSquared = getLengthSquared();
		return sqrt(lenSquared);
	}
	
	/** Return the squared length of this vector.          
     */
	double getLengthSquared() const {
		double lenSquared = double(_v[0]) * double(_v[0]) + 
							double(_v[1]) * double(_v[1]) + 
							double(_v[2]) * double(_v[2]);
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
	 */
	void normalise() {
		double inverseLength = 1/getLength();
		_v[0] *= inverseLength;
		_v[1] *= inverseLength;
		_v[2] *= inverseLength;
	}
	
	// Returns a vector with the reciprocal values of each component
	BasicVector3<Element> getInversed() {
  		return BasicVector3<Element>(
			1.0 / _v[0],
			1.0 / _v[1],
			1.0 / _v[2]
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
		return 	Element(_v[0] * other.x() +
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
	bool isParallel(const BasicVector3<OtherT>& other) const {
		return (float_equal_epsilon(angle(other), double(0.0f), double(0.001f)) 
			 || float_equal_epsilon(angle(other), c_pi, double(0.001f))); 
	}
};

/** Stream insertion operator for BasicVector3. Formats vector as "<x, y, z>".
 */
 
template<typename T>
std::ostream& operator<<(std::ostream& st, BasicVector3<T> vec) {
	st << "<" << vec.x() << ", " << vec.y() << ", " << vec.z() << ">";
	return st;
}

// ==========================================================================================

// A 3-element vector stored in double-precision floating-point.
typedef BasicVector3<double> Vector3;

// =============== Common Vector3 Methods ==================================================

const Vector3 g_vector3_identity(0, 0, 0);
const Vector3 g_vector3_max = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
const Vector3 g_vector3_axis_x(1, 0, 0);
const Vector3 g_vector3_axis_y(0, 1, 0);
const Vector3 g_vector3_axis_z(0, 0, 1);

const Vector3 g_vector3_axes[3] = { g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z };

template<typename Element, typename OtherElement>
inline void vector3_swap(BasicVector3<Element>& self, BasicVector3<OtherElement>& other) {
  std::swap(self.x(), other.x());
  std::swap(self.y(), other.y());
  std::swap(self.z(), other.z());
}

template<typename Element, typename OtherElement, typename Epsilon>
inline bool vector3_equal_epsilon(const BasicVector3<Element>& self, const BasicVector3<OtherElement>& other, Epsilon epsilon) {
  return float_equal_epsilon(self.x(), other.x(), epsilon)
    && float_equal_epsilon(self.y(), other.y(), epsilon)
    && float_equal_epsilon(self.z(), other.z(), epsilon);
}

template<typename Element>
inline BasicVector3<Element> vector3_mid(const BasicVector3<Element>& begin, const BasicVector3<Element>& end) {
  return (begin + end)*0.5;
}

template<typename Element>
inline Element float_divided(Element f, Element other) {
  return f / other;
}

template<typename Element>
inline void vector3_normalise(BasicVector3<Element>& self) {
  self = self.getNormalised();
}

template<typename Element>
inline BasicVector3<Element> vector3_snapped(const BasicVector3<Element>& self) {
  return BasicVector3<Element>(
    Element(float_to_integer(self.x())),
    Element(float_to_integer(self.y())),
    Element(float_to_integer(self.z()))
  );
}

template<typename Element>
inline void vector3_snap(BasicVector3<Element>& self) {
  self = vector3_snapped(self);
}

template<typename Element, typename OtherElement>
inline BasicVector3<Element> vector3_snapped(const BasicVector3<Element>& self, const OtherElement& snap) {
  return BasicVector3<Element>(
    Element(float_snapped(self.x(), snap)),
    Element(float_snapped(self.y(), snap)),
    Element(float_snapped(self.z(), snap))
  );
}
template<typename Element, typename OtherElement>
inline void vector3_snap(BasicVector3<Element>& self, const OtherElement& snap) {
  self = vector3_snapped(self, snap);
}

inline Vector3 vector3_for_spherical(double theta, double phi) {
  return Vector3(
    cos(theta) * cos(phi),
    sin(theta) * cos(phi),
    sin(phi)
  );
}


#endif /*VECTOR3_H_*/
