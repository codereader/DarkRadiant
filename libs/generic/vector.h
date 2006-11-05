
#if !defined(INCLUDED_VECTOR_H)
#define INCLUDED_VECTOR_H

#include <cstddef>
#include <cmath>
#include <sstream>
#include <string>

#include <boost/format.hpp>

template <typename Element>
class BasicVector2
{
  Element m_elements[2];
public:
  BasicVector2()
  {
  }
  BasicVector2(const Element& x_, const Element& y_)
  {
    x() = x_;
    y() = y_;
  }

  Element& x()
  {
    return m_elements[0];
  }
  const Element& x() const
  {
    return m_elements[0];
  }
  Element& y()
  {
    return m_elements[1];
  }
  const Element& y() const
  {
    return m_elements[1];
  }

  const Element& operator[](std::size_t i) const
  {
    return m_elements[i];
  }
  Element& operator[](std::size_t i)
  {
    return m_elements[i];
  }

  Element* data()
  {
    return m_elements;
  }
  const Element* data() const
  {
    return m_elements;
  }
};

/// A 3-element vector.
template<typename Element>
class BasicVector3 {
	
	// The actual values of the vector, an array containing 3 values of type Element
	Element m_elements[3];
  
public:

  	BasicVector3()
  	{
  	}

	// Templated copy constructor
	template<typename OtherElement>
	BasicVector3(const BasicVector3<OtherElement>& other)
	{
		x() = static_cast<Element>(other.x());
		y() = static_cast<Element>(other.y());
		z() = static_cast<Element>(other.z());
	}

    /** Construct a BasicVector3 with the 3 provided components.
     */
    BasicVector3(const Element& x_, const Element& y_, const Element& z_)
    {
        x() = x_;
        y() = y_;
        z() = z_;
    }

	/** Construct a BasicVector3 from a 3-element array. The array must be
	 * valid as no bounds checking is done.
	 */
	BasicVector3(const Element* array) {
		for (int i = 0; i < 3; ++i)
			m_elements[i] = array[i];
	}
	 
    /** Construct a BasicVector3 by parsing the supplied string. The string
     * must contain 3 numeric values separated by whitespace.
     *
     * @param str
     * The string from which component values are extracted.
     */
     
    BasicVector3(const std::string& str) {
    	// Initialise the vector with 0, in case the string parse fails
    	m_elements[0] = m_elements[1] = m_elements[2] = 0;
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
		m_elements[0] = x;
		m_elements[1] = y;
		m_elements[2] = z;
	}

	// Return NON-CONSTANT references to the vector components
  	Element& x() { 
  		return m_elements[0]; 
  	}
  	Element& y() {
    	return m_elements[1];
  	}
  	Element& z() {
    	return m_elements[2];
  	}
  
  	// Return CONSTANT references to the vector components
  	const Element& x() const {  
  		return m_elements[0]; 
  	}  
  	const Element& y() const {
    	return m_elements[1];
  	}  
  	const Element& z() const {
    	return m_elements[2];
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
	inline BasicVector3<Element> operator- () const {
		return BasicVector3<Element>(
			-m_elements[0],
			-m_elements[1],
			-m_elements[2]
		);
	}
	
	/*	Define the addition operators + and += with any other BasicVector3 of type OtherElement
	 *  The vectors are added to each other element-wise
	 */
	template<typename OtherElement>
	inline BasicVector3<Element> operator+ (const BasicVector3<OtherElement>& other) const {
		return BasicVector3<Element>(
			m_elements[0] + static_cast<Element>(other.x()),
			m_elements[1] + static_cast<Element>(other.y()),
			m_elements[2] + static_cast<Element>(other.z())
		);
	}
	
	template<typename OtherElement>
	inline void operator+= (const BasicVector3<OtherElement>& other) {
		m_elements[0] += static_cast<Element>(other.x());
		m_elements[1] += static_cast<Element>(other.y());
		m_elements[2] += static_cast<Element>(other.z());
	}
	
	/*	Define the substraction operators - and -= with any other BasicVector3 of type OtherElement
	 *  The vectors are substracted from each other element-wise
	 */
	template<typename OtherElement>
	inline BasicVector3<Element> operator- (const BasicVector3<OtherElement>& other) const {
		return BasicVector3<Element>(
			m_elements[0] - static_cast<Element>(other.x()),
			m_elements[1] - static_cast<Element>(other.y()),
			m_elements[2] - static_cast<Element>(other.z())
		);
	}
	
	template<typename OtherElement>
	inline void operator-= (const BasicVector3<OtherElement>& other) {
		m_elements[0] -= static_cast<Element>(other.x());
		m_elements[1] -= static_cast<Element>(other.y());
		m_elements[2] -= static_cast<Element>(other.z());
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
	inline BasicVector3<Element> operator* (const BasicVector3<OtherElement>& other) const {
		return BasicVector3<Element>(
			m_elements[0] * static_cast<Element>(other.x()),
			m_elements[1] * static_cast<Element>(other.y()),
			m_elements[2] * static_cast<Element>(other.z())
		);
	}
	
	template<typename OtherElement>
	inline void operator*= (const BasicVector3<OtherElement>& other) {
		m_elements[0] *= static_cast<Element>(other.x());
		m_elements[1] *= static_cast<Element>(other.y());
		m_elements[2] *= static_cast<Element>(other.z());		
	}
	
	
	/*	Define the multiplications * and *= with a scalar
	 */
	template<typename OtherElement>
	inline BasicVector3<Element> operator* (const OtherElement& other) const {
		Element factor = static_cast<Element>(other);
		return BasicVector3<Element>(
			m_elements[0] * factor,
			m_elements[1] * factor,
			m_elements[2] * factor
		);
	}
	
	template<typename OtherElement>
	inline void operator*= (const OtherElement& other) {
		Element factor = static_cast<Element>(other);
		m_elements[0] *= factor;
		m_elements[1] *= factor;
		m_elements[2] *= factor;		
	}
	
	/*	Define the multiplication operators / and /= with another Vector3 of type OtherElement
	 *  The vectors are divided element-wise   
	 */
	template<typename OtherElement>
	inline BasicVector3<Element> operator/ (const BasicVector3<OtherElement>& other) const {
		return BasicVector3<Element>(
			m_elements[0] / static_cast<Element>(other.x()),
			m_elements[1] / static_cast<Element>(other.y()),
			m_elements[2] / static_cast<Element>(other.z())
		);
	}
	
	template<typename OtherElement>
	inline void operator/= (const BasicVector3<OtherElement>& other) {
		m_elements[0] /= static_cast<Element>(other.x());
		m_elements[1] /= static_cast<Element>(other.y());
		m_elements[2] /= static_cast<Element>(other.z());		
	}
	
	/*	Define the scalar divisions / and /= 
	 */
	template<typename OtherElement>
	inline BasicVector3<Element> operator/ (const OtherElement& other) const {
		Element divisor = static_cast<Element>(other);
		return BasicVector3<Element>(
			m_elements[0] / divisor,
			m_elements[1] / divisor,
			m_elements[2] / divisor
		);
	}
	
	template<typename OtherElement>
	inline void operator/= (const OtherElement& other) {
		Element divisor = static_cast<Element>(other);
		m_elements[0] /= divisor;
		m_elements[1] /= divisor;
		m_elements[2] /= divisor;		
	}
  
	/** Cast to std::string, formats vector correctly for use as a
	 * keyval -- "x y z".
	 */
	 
	operator std::string() const {
		return (boost::format("%s %s %s")
				 % m_elements[0]
				 % m_elements[1]
				 % m_elements[2]).str();
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
		double lenSquared = m_elements[0]*m_elements[0] + 
							m_elements[1]*m_elements[1] + 
							m_elements[2]*m_elements[2];
		return sqrt(lenSquared);
	}
	
	/** Return the squared length of this vector.          
     */
	double getLengthSquared() const {
		double lenSquared = m_elements[0]*m_elements[0] + 
							m_elements[1]*m_elements[1] + 
							m_elements[2]*m_elements[2];
		return lenSquared;
	}

	// Return a new BasicVector3 equivalent to the normalised
	// version of this BasicVector3 (scaled by the inverse of its size)
	BasicVector3<Element> getNormalised() const {
		return (*this)/getLength();
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
		return 	Element(m_elements[0] * other.x() +
				 		m_elements[1] * other.y() +
				 		m_elements[2] * other.z());
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
			m_elements[1] * other.z() - m_elements[2] * other.y(),
			m_elements[2] * other.x() - m_elements[0] * other.z(),
			m_elements[0] * other.y() - m_elements[1] * other.x());
	}

	/** Implicit cast to C-style array. This allows a Vector3 to be
	 * passed directly to GL functions that expect an array (e.g.
	 * glFloat3fv()). These functions implicitly provide operator[]
	 * as well, since the C-style array provides this function.
	 */
	 
	operator const Element* () const {
		return m_elements;
	}

	operator Element* () {
		return m_elements;
	}

};

/** Stream insertion operator for BasicVector3. Formats vector as "<x, y, z>".
 */
 
template<typename T>
std::ostream& operator<<(std::ostream& st, BasicVector3<T> vec) {
	st << "<" << vec.x() << ", " << vec.y() << ", " << vec.z() << ">";
	return st;
}

/// \brief A 4-element vector.
template<typename Element>
class BasicVector4
{
  Element m_elements[4];
public:

  BasicVector4()
  {
  }
  BasicVector4(Element x_, Element y_, Element z_, Element w_)
  {
    x() = x_;
    y() = y_;
    z() = z_;
    w() = w_;
  }
  BasicVector4(const BasicVector3<Element>& self, Element w_)
  {
    x() = self.x();
    y() = self.y();
    z() = self.z();
    w() = w_;
  }

  Element& x()
  {
    return m_elements[0];
  }
  const Element& x() const
  {
    return m_elements[0];
  }
  Element& y()
  {
    return m_elements[1];
  }
  const Element& y() const
  {
    return m_elements[1];
  }
  Element& z()
  {
    return m_elements[2];
  }
  const Element& z() const
  {
    return m_elements[2];
  }
  Element& w()
  {
    return m_elements[3];
  }
  const Element& w() const
  {
    return m_elements[3];
  }

  Element index(std::size_t i) const
  {
    return m_elements[i];
  }
  Element& index(std::size_t i)
  {
    return m_elements[i];
  }
  Element operator[](std::size_t i) const
  {
    return m_elements[i];
  }
  Element& operator[](std::size_t i)
  {
    return m_elements[i];
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
			m_elements[0] / m_elements[3],
			m_elements[1] / m_elements[3],
			m_elements[2] / m_elements[3]);
	}

  Element* data()
  {
    return m_elements;
  }
  const Element* data() const
  {
    return m_elements;
  }
};

template<typename Element>
inline BasicVector3<Element> vector3_from_array(const Element* array)
{
  return BasicVector3<Element>(array[0], array[1], array[2]);
}

template<typename Element>
inline Element* vector3_to_array(BasicVector3<Element>& self)
{
  return static_cast<Element*>(self);
}
template<typename Element>
inline const Element* vector3_to_array(const BasicVector3<Element>& self)
{
  return static_cast<const Element*>(self);
}

template<typename Element>
inline Element* vector4_to_array(BasicVector4<Element>& self)
{
  return self.data();
}
template<typename Element>
inline const Element* vector4_to_array(const BasicVector4<Element>& self)
{
  return self.data();
}

template<typename Element>
inline BasicVector3<Element>& vector4_to_vector3(BasicVector4<Element>& self)
{
  return *reinterpret_cast<BasicVector3<Element>*>(vector4_to_array(self));
}
template<typename Element>
inline const BasicVector3<Element>& vector4_to_vector3(const BasicVector4<Element>& self)
{
  return *reinterpret_cast<const BasicVector3<Element>*>(vector4_to_array(self));
}

/// \brief A 2-element vector stored in single-precision floating-point.
typedef BasicVector2<float> Vector2;

/// \brief A 3-element vector stored in single-precision floating-point.
typedef BasicVector3<float> Vector3;

/// \brief A 4-element vector stored in single-precision floating-point.
typedef BasicVector4<float> Vector4;

#endif
