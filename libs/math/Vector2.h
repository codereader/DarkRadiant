#ifndef VECTOR2_H_
#define VECTOR2_H_

/* greebo: This file contains the templated class definition of the three-component vector
 * 
 * BasicVector2: A vector with three components of type <Element>
 * 
 * The BasicVector2 is equipped with the most important operators like *, *= and so on.
 * 
 * Note: The most commonly used Vector2 is a BasicVector2<float>, this is also defined in this file 
 *  
 * Note: that the multiplication of a Vector2 with another one (Vector2*Vector2) does NOT
 * result in an inner product but in a component-wise scaling. Use the .dot() method to
 * execute an inner product of two vectors. 
 */
 
#include "lrint.h"
#include <cmath>
#include <boost/format.hpp>

template <typename Element>
class BasicVector2 {
	
	// This is where the components of the vector are stored.
	Element _v[2];
	
public:
	// Constructor with no arguments
  	BasicVector2() {
  		_v[0] = 0;
  		_v[1] = 0;
  	}
  	
    /** Construct a BasicVector2 out of 2 elements of type OtherType
     */
    template <typename OtherType>
  	BasicVector2(OtherType x, OtherType y) {
    	_v[0] = static_cast<Element>(x);
    	_v[1] = static_cast<Element>(y);
    }
    
    // Return NON-CONSTANT references to the vector components
    Element& x() {
    	return _v[0];
    }
    Element& y() {
    	return _v[1];
  	}
  	
  	// Return CONSTANT references to the vector components
  	const Element& x() const {
    	return _v[0];
  	}
  	const Element& y() const {
    	return _v[1];
  	}

  	Element& operator[] (std::size_t i) {
    	return _v[i];
  	}
  	const Element& operator[] (std::size_t i) const {
    	return _v[i];
  	}

  	Element* data() {
    	return _v;
  	}  	
  	const Element* data() const {
    	return _v;
  	}
  	
  	/** Compare this BasicVector2 against another for equality.
	 */
	bool operator== (const BasicVector2& other) const {
		return (other.x() == x() 
				&& other.y() == y());
	}
	
	/** Compare this BasicVector2 against another for inequality.
	 */
	bool operator!= (const BasicVector2& other) const {
		return !(*this == other);
	}
	
	/*	Define the negation operator - 
	 *  All the vector's components are negated
	 */
	BasicVector2<Element> operator- () const {
		return BasicVector2<Element>(
			-_v[0],
			-_v[1]
		);
	}
	
	/*	Define the addition operators + and += with any other BasicVector2 of type OtherElement
	 *  The vectors are added to each other element-wise
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator+ (const BasicVector2<OtherElement>& other) const {
		return BasicVector2<Element>(
			_v[0] + static_cast<Element>(other.x()),
			_v[1] + static_cast<Element>(other.y())
		);
	}
	
	template<typename OtherElement>
	void operator+= (const BasicVector2<OtherElement>& other) {
		_v[0] += static_cast<Element>(other.x());
		_v[1] += static_cast<Element>(other.y());
	}
	
	/*	Define the substraction operators - and -= with any other BasicVector2 of type OtherElement
	 *  The vectors are substracted from each other element-wise
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator- (const BasicVector2<OtherElement>& other) const {
		return BasicVector2<Element>(
			_v[0] - static_cast<Element>(other.x()),
			_v[1] - static_cast<Element>(other.y())
		);
	}
	
	template<typename OtherElement>
	void operator-= (const BasicVector2<OtherElement>& other) {
		_v[0] -= static_cast<Element>(other.x());
		_v[1] -= static_cast<Element>(other.y());
	}
	
	/*	Define the multiplication operators * and *= with another Vector2 of type OtherElement
	 * 
	 *  The vectors are multiplied element-wise
	 * 
	 *  greebo: This is mathematically kind of senseless, as this is a mixture of 
	 *  a dot product and scalar multiplication. It can be used to scale each 
	 *  vector component by a different factor, so maybe this comes in handy.  
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator* (const BasicVector2<OtherElement>& other) const {
		return BasicVector2<Element>(
			_v[0] * static_cast<Element>(other.x()),
			_v[1] * static_cast<Element>(other.y())
		);
	}
	
	template<typename OtherElement>
	void operator*= (const BasicVector2<OtherElement>& other) {
		_v[0] *= static_cast<Element>(other.x());
		_v[1] *= static_cast<Element>(other.y());
	}
	
	
	/*	Define the multiplications * and *= with a scalar
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator* (const OtherElement& other) const {
		Element factor = static_cast<Element>(other);
		return BasicVector2<Element>(
			_v[0] * factor,
			_v[1] * factor
		);
	}
	
	template<typename OtherElement>
	void operator*= (const OtherElement& other) {
		Element factor = static_cast<Element>(other);
		_v[0] *= factor;
		_v[1] *= factor;
	}
	
	/*	Define the division operators / and /= with another Vector2 of type OtherElement
	 *  The vectors are divided element-wise   
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator/ (const BasicVector2<OtherElement>& other) const {
		return BasicVector2<Element>(
			_v[0] / static_cast<Element>(other.x()),
			_v[1] / static_cast<Element>(other.y())
		);
	}
	
	template<typename OtherElement>
	void operator/= (const BasicVector2<OtherElement>& other) {
		_v[0] /= static_cast<Element>(other.x());
		_v[1] /= static_cast<Element>(other.y());
	}
	
	/*	Define the scalar divisions / and /= 
	 */
	template<typename OtherElement>
	BasicVector2<Element> operator/ (const OtherElement& other) const {
		Element divisor = static_cast<Element>(other);
		return BasicVector2<Element>(
			_v[0] / divisor,
			_v[1] / divisor
		);
	}
	
	template<typename OtherElement>
	void operator/= (const OtherElement& other) {
		Element divisor = static_cast<Element>(other);
		_v[0] /= divisor;
		_v[1] /= divisor;
	}
	
	/** Cast to std::string
	 */
	operator std::string() const {
		return (boost::format("%s %s")
				 % _v[0]
				 % _v[1]).str();
	}
	
	/** Return the length of this vector.
     * 
     * @returns
     * The Pythagorean length of this vector.
     */
	double getLength() const {
		double lenSquared = _v[0]*_v[0] + 
							_v[1]*_v[1];
		return sqrt(lenSquared);
	}
	
	/** Return the squared length of this vector.          
     */
	double getLengthSquared() const {
		double lenSquared = _v[0]*_v[0] + 
							_v[1]*_v[1];
		return lenSquared;
	}
	
	/* Scalar product this vector with another Vector2, 
	 * returning the projection of <self> onto <other>
	 * 
	 * @param other
	 * The Vector2 to dot-product with this Vector2.
	 * 
	 * @returns
	 * The inner product (a scalar): a[0]*b[0] + a[1]*b[1]
	 */	 
	template<typename OtherT>
	Element dot(const BasicVector2<OtherT>& other) const {
		return 	Element(_v[0] * other.x() +
				 		_v[1] * other.y());
	}

	/* Cross-product this vector with another Vector2, returning the scalar result
	 * 
	 * @param other
	 * The Vector2 to cross-product with this Vector2.
	 * 
	 * @returns
	 * The cross-product of the two vectors, a scalar: a[0]*b[1] - b[0]*a[1]
	 */
	template<typename OtherT>
	Element crossProduct(const BasicVector2<OtherT>& other) const {
		return Element(_v[0] * other.y() - _v[1] * other.x());
	}
};

// ==========================================================================================

// A 2-element vector stored in single-precision floating-point.
typedef BasicVector2<float> Vector2;

// Stream insertion operator for a BasicVector2
template<typename T>
std::ostream& operator<<(std::ostream& st, BasicVector2<T> vec) {
	st << "<" << vec.x() << ", " << vec.y() << ">";
	return st;
}

#endif /*VECTOR2_H_*/
