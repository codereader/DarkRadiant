#pragma once

/// \file
/// \brief Matrix data types and related operations.

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Plane3.h"

/**
 * A 4x4 matrix stored in single-precision floating-point.
 *
 * The elements of this matrix are stored columnwise in memory:
 *
 * |  0    4    8   12 |
 * |  1    5    9   13 |
 * |  2    6   10   14 |
 * |  3    7   11   15 |
 *
 * or, alternatively, as the 4 columns are regarded as 4 vectors named x, y, z, t:
 *
 * | xx   yx   zx   tx |
 * | xy   yy   zy   ty |
 * | xz   yz   zz   tz |
 * | xw   yw   zw   tw |
 */
class Matrix4
{
private:
    // Elements of the 4x4 matrix. These appear to be treated COLUMNWISE, i.e.
    // elements [0] through [3] are the first column, [4] through [7] are the
    // second column, etc.
    float _m[16];

private:

    // Initialising constructor, elements are passed in column-wise order
    Matrix4(float xx_, float xy_, float xz_, float xw_,
            float yx_, float yy_, float yz_, float yw_,
            float zx_, float zy_, float zz_, float zw_,
            float tx_, float ty_, float tz_, float tw_);
public:

    /**
     * \brief
     * Default constructor.
     *
     * Constructs a matrix with uninitialised values.
     */
    Matrix4() { }

    /* NAMED CONSTRUCTORS FOR SPECIFIC MATRICES */

    /**
     * \brief
     * Obtain the identity matrix.
     */
    static const Matrix4& getIdentity();

	/**
     * \brief
     * Get a matrix representing the given 3D translation.
	 *
	 * @param translation
	 * Vector3 representing the translation in 3D space.
	 */
	static Matrix4 getTranslation(const Vector3& translation);

	/**
	 * greebo: Returns the rotation matrix defined by two three-component
	 * vectors.
	 * The rotational axis is defined by the normalised cross product of those
	 * two vectors, the angle can be retrieved from the dot product.
	 */
	static Matrix4 getRotation(const Vector3& a, const Vector3& b);

	/**
	 * greebo: Returns the rotation matrix defined by an arbitrary axis
	 * and an angle.
	 *
	 * Important: the axis vector must be normalised.
	 */
	static Matrix4 getRotation(const Vector3& axis, const float angle);

    /**
     * \brief
     * Get a matrix representing the given scale in 3D space.
     *
     * \param scale
     * Vector3 representing the scale.
     */
    static Matrix4 getScale(const Vector3& scale);

    /**
     * \brief
     * Construct a matrix containing the given elements.
     *
     * The elements are specified column-wise, starting with the left-most
     * column.
     */
    static Matrix4 byColumns(float xx, float xy, float xz, float xw,
                             float yx, float yy, float yz, float yw,
                             float zx, float zy, float zz, float zw,
                             float tx, float ty, float tz, float tw);

    /**
     * \brief
     * Construct a matrix containing the given elements.
     *
     * The elements are specified row-wise, starting with the top row.
     */
    static Matrix4 byRows(float xx, float yx, float zx, float tx,
                          float xy, float yy, float zy, float ty,
                          float xz, float yz, float zz, float tz,
                          float xw, float yw, float zw, float tw);

	enum Handedness
	{
		RIGHTHANDED = 0,
		LEFTHANDED = 1,
	};

    /**
     * Return matrix elements
     * \{
     */
    float& xx()             { return _m[0]; }
    const float& xx() const { return _m[0]; }
    float& xy()             { return _m[1]; }
    const float& xy() const { return _m[1]; }
    float& xz()             { return _m[2]; }
    const float& xz() const { return _m[2]; }
    float& xw()             { return _m[3]; }
    const float& xw() const { return _m[3]; }
    float& yx()             { return _m[4]; }
    const float& yx() const { return _m[4]; }
    float& yy()             { return _m[5]; }
    const float& yy() const { return _m[5]; }
    float& yz()             { return _m[6]; }
    const float& yz() const { return _m[6]; }
    float& yw()             { return _m[7]; }
    const float& yw() const { return _m[7]; }
    float& zx()             { return _m[8]; }
    const float& zx() const { return _m[8]; }
    float& zy()             { return _m[9]; }
    const float& zy() const { return _m[9]; }
    float& zz()             { return _m[10]; }
    const float& zz() const { return _m[10]; }
    float& zw()             { return _m[11]; }
    const float& zw() const { return _m[11]; }
    float& tx()             { return _m[12]; }
    const float& tx() const { return _m[12]; }
    float& ty()             { return _m[13]; }
    const float& ty() const { return _m[13]; }
    float& tz()             { return _m[14]; }
    const float& tz() const { return _m[14]; }
    float& tw()             { return _m[15]; }
    const float& tw() const { return _m[15]; }
    /**
     * \}
     */

    /**
     * Return columns of the matrix as vectors.
     * \{
     */
    Vector4& x()
    {
        return reinterpret_cast<Vector4&>(xx());
    }
    const Vector4& x() const
    {
        return reinterpret_cast<const Vector4&>(xx());
    }
    Vector4& y()
    {
        return reinterpret_cast<Vector4&>(yx());
    }
    const Vector4& y() const
    {
        return reinterpret_cast<const Vector4&>(yx());
    }
    Vector4& z()
    {
        return reinterpret_cast<Vector4&>(zx());
    }
    const Vector4& z() const
    {
        return reinterpret_cast<const Vector4&>(zx());
    }
    Vector4& t()
    {
        return reinterpret_cast<Vector4&>(tx());
    }
    const Vector4& t() const
    {
        return reinterpret_cast<const Vector4&>(tx());
    }
    /**
     * \}
     */

	const float& index(std::size_t i) const
	{
		return _m[i];
	}

	float& index(std::size_t i)
	{
		return _m[i];
	}

	const float& index(std::size_t r, std::size_t c) const
	{
		return _m[(r << 2) + c];
	}

	float& index(std::size_t r, std::size_t c)
	{
		return _m[(r << 2) + c];
	}

	/**
     * Cast to float* for use with GL functions that accept a float
	 * array, also provides operator[].
	 */
	operator float* ()
	{
		return _m;
	}

	/**
     * Cast to const float* to provide operator[] for const objects.
	 */
	operator const float* () const
	{
		return _m;
	}

	/**
     * \brief
     * Transpose this matrix in-place.
     */
	void transpose();

	/**
     * \brief
     * Return a transposed copy of this matrix.
     */
	Matrix4 getTransposed() const;

    /**
     * \brief
     * Return the affine inverse of this transformation matrix.
     */
    Matrix4 getInverse() const;

	/**
     * \brief
     * Use this matrix to transform the provided vector and return a new vector
     * containing the result.
	 *
	 * \param vector4
	 * The 4-element vector to transform.
	 */
	Vector4 transform(const Vector4& vector4) const;

	/**
     * \brief
     * Use this matrix to transform the provided 3-element vector, automatically
     * converting the vector to a 4-element homogeneous vector with w=1.
	 *
	 * \param vector3
	 * The Vector3 to transform.
	 *
	 * \returns
	 * A 4-element vector containing the result.
	 */
	Vector4 transform(const Vector3& vector3) const {
		return transform(Vector4(vector3, 1));
	}

	/** Use this matrix to transform the provided plane
	 *
	 * @param plane: The Plane to transform.
	 *
	 * @returns: the transformed plane.
	 */
	Plane3 transform(const Plane3& plane) const;

    /**
     * \brief
     * Inverse transform a plane.
     */
	Plane3 inverseTransform(const Plane3& plane) const;

    /**
     * \brief
     * Return the result of this matrix post-multiplied by another matrix.
     */
    Matrix4 getMultipliedBy(const Matrix4& other) const;

    /**
     * \brief
     * Post-multiply this matrix by another matrix, in-place.
     */
    void multiplyBy(const Matrix4& other);

    /**
     * Returns this matrix pre-multiplied by the other
     */
    Matrix4 getPremultipliedBy(const Matrix4& other) const;

	/**
	 * Pre-multiplies this matrix by other in-place.
	 */
	void premultiplyBy(const Matrix4& other);

    /**
     * \brief
     * Add a translation component to the transformation represented by this
     * matrix.
     *
     * Equivalent to multiplyBy(Matrix4::getTranslation(translation));
     */
    void translateBy(const Vector3& translation);

    /**
     * \brief
     * Add a scale component to the transformation represented by this matrix.
     *
     * Equivalent to multiplyBy(Matrix4::getScale(scale));
     */
    void scaleBy(const Vector3& scale);

	/**
	 * Equality operator, Returns true if this and the other are exactly element-wise equal.
	 */
	bool operator==(const Matrix4& other) const;

	/** 
	 * Inequality operator.
	 */
	bool operator!=(const Matrix4& other) const;

	/**
	 * Returns true if self and other are element-wise equal within epsilon.
	 */
	bool isEqual(const Matrix4& other, float epsilon) const;

	/**
	 * Returns true if this and the given matrix are exactly element-wise equal.
	 * This and the other matrix must be affine.
	 */
	bool isAffineEqual(const Matrix4& other) const;

	/**
	 * Returns RIGHTHANDED if this is right-handed, else returns LEFTHANDED.
	 */
	Handedness getHandedness() const;
};

// =========================================================================================
// Inlined member definitions 
// =========================================================================================

// Main explicit constructor (private)
inline Matrix4::Matrix4(float xx_, float xy_, float xz_, float xw_,
						float yx_, float yy_, float yz_, float yw_,
						float zx_, float zy_, float zz_, float zw_,
						float tx_, float ty_, float tz_, float tw_)
{
    xx() = xx_;
    xy() = xy_;
    xz() = xz_;
    xw() = xw_;
    yx() = yx_;
    yy() = yy_;
    yz() = yz_;
    yw() = yw_;
    zx() = zx_;
    zy() = zy_;
    zz() = zz_;
    zw() = zw_;
    tx() = tx_;
    ty() = ty_;
    tz() = tz_;
    tw() = tw_;
}

// Construct a matrix with given column elements
inline Matrix4 Matrix4::byColumns(float xx, float xy, float xz, float xw,
								  float yx, float yy, float yz, float yw,
								  float zx, float zy, float zz, float zw,
								  float tx, float ty, float tz, float tw)
{
    return Matrix4(xx, xy, xz, xw,
                   yx, yy, yz, yw,
                   zx, zy, zz, zw,
                   tx, ty, tz, tw);
}

// Construct a matrix with given row elements
inline Matrix4 Matrix4::byRows(float xx, float yx, float zx, float tx,
							   float xy, float yy, float zy, float ty,
							   float xz, float yz, float zz, float tz,
							   float xw, float yw, float zw, float tw)
{
    return Matrix4(xx, xy, xz, xw,
                   yx, yy, yz, yw,
                   zx, zy, zz, zw,
                   tx, ty, tz, tw);
}

// Post-multiply this with other
inline Matrix4 Matrix4::getMultipliedBy(const Matrix4& other) const
{
	return Matrix4::byColumns(
        other[0] * _m[0] + other[1] * _m[4] + other[2] * _m[8] + other[3] * _m[12],
        other[0] * _m[1] + other[1] * _m[5] + other[2] * _m[9] + other[3] * _m[13],
        other[0] * _m[2] + other[1] * _m[6] + other[2] * _m[10]+ other[3] * _m[14],
        other[0] * _m[3] + other[1] * _m[7] + other[2] * _m[11]+ other[3] * _m[15],
        other[4] * _m[0] + other[5] * _m[4] + other[6] * _m[8] + other[7] * _m[12],
        other[4] * _m[1] + other[5] * _m[5] + other[6] * _m[9] + other[7] * _m[13],
        other[4] * _m[2] + other[5] * _m[6] + other[6] * _m[10]+ other[7] * _m[14],
        other[4] * _m[3] + other[5] * _m[7] + other[6] * _m[11]+ other[7] * _m[15],
        other[8] * _m[0] + other[9] * _m[4] + other[10]* _m[8] + other[11]* _m[12],
        other[8] * _m[1] + other[9] * _m[5] + other[10]* _m[9] + other[11]* _m[13],
        other[8] * _m[2] + other[9] * _m[6] + other[10]* _m[10]+ other[11]* _m[14],
        other[8] * _m[3] + other[9] * _m[7] + other[10]* _m[11]+ other[11]* _m[15],
        other[12]* _m[0] + other[13]* _m[4] + other[14]* _m[8] + other[15]* _m[12],
        other[12]* _m[1] + other[13]* _m[5] + other[14]* _m[9] + other[15]* _m[13],
        other[12]* _m[2] + other[13]* _m[6] + other[14]* _m[10]+ other[15]* _m[14],
        other[12]* _m[3] + other[13]* _m[7] + other[14]* _m[11]+ other[15]* _m[15]
    );
}

inline Matrix4 Matrix4::getPremultipliedBy(const Matrix4& other) const
{
    return other.getMultipliedBy(*this);
}

inline bool Matrix4::operator==(const Matrix4& other) const
{
	return xx() == other.xx() && xy() == other.xy() && xz() == other.xz() && xw() == other.xw()
		&& yx() == other.yx() && yy() == other.yy() && yz() == other.yz() && yw() == other.yw()
		&& zx() == other.zx() && zy() == other.zy() && zz() == other.zz() && zw() == other.zw()
		&& tx() == other.tx() && ty() == other.ty() && tz() == other.tz() && tw() == other.tw();
}

inline bool Matrix4::operator!=(const Matrix4& other) const
{
	return !operator==(other);
}

inline bool Matrix4::isEqual(const Matrix4& other, float epsilon) const
{
	return float_equal_epsilon(xx(), other.xx(), epsilon)
		&& float_equal_epsilon(xy(), other.xy(), epsilon)
		&& float_equal_epsilon(xz(), other.xz(), epsilon)
		&& float_equal_epsilon(xw(), other.xw(), epsilon)
		&& float_equal_epsilon(yx(), other.yx(), epsilon)
		&& float_equal_epsilon(yy(), other.yy(), epsilon)
		&& float_equal_epsilon(yz(), other.yz(), epsilon)
		&& float_equal_epsilon(yw(), other.yw(), epsilon)
		&& float_equal_epsilon(zx(), other.zx(), epsilon)
		&& float_equal_epsilon(zy(), other.zy(), epsilon)
		&& float_equal_epsilon(zz(), other.zz(), epsilon)
		&& float_equal_epsilon(zw(), other.zw(), epsilon)
		&& float_equal_epsilon(tx(), other.tx(), epsilon)
		&& float_equal_epsilon(ty(), other.ty(), epsilon)
		&& float_equal_epsilon(tz(), other.tz(), epsilon)
		&& float_equal_epsilon(tw(), other.tw(), epsilon);
}

inline bool Matrix4::isAffineEqual(const Matrix4& other) const
{
	return xx() == other.xx() && 
			xy() == other.xy() && 
			xz() == other.xz() && 
			yx() == other.yx() && 
			yy() == other.yy() && 
			yz() == other.yz() && 
			zx() == other.zx() && 
			zy() == other.zy() && 
			zz() == other.zz() && 
			tx() == other.tx() && 
			ty() == other.ty() && 
			tz() == other.tz();
}

inline Matrix4::Handedness Matrix4::getHandedness() const
{
	return (x().getVector3().crossProduct(y().getVector3()).dot(z().getVector3()) < 0.0f) ? LEFTHANDED : RIGHTHANDED;
}

inline void Matrix4::premultiplyBy(const Matrix4& other)
{
	*this = getPremultipliedBy(other);
}

/// \brief returns true if \p transform is affine.
inline bool matrix4_is_affine(const Matrix4& transform)
{
  return transform[3] == 0 && transform[7] == 0 && transform[11] == 0 && transform[15] == 1;
}

/// \brief Returns \p self post-multiplied by \p other.
/// \p self and \p other must be affine.
inline Matrix4 matrix4_affine_multiplied_by_matrix4(const Matrix4& self, const Matrix4& other)
{
  return Matrix4::byColumns(
    other[0] * self[0] + other[1] * self[4] + other[2] * self[8],
    other[0] * self[1] + other[1] * self[5] + other[2] * self[9],
    other[0] * self[2] + other[1] * self[6] + other[2] * self[10],
    0,
    other[4] * self[0] + other[5] * self[4] + other[6] * self[8],
    other[4] * self[1] + other[5] * self[5] + other[6] * self[9],
    other[4] * self[2] + other[5] * self[6] + other[6] * self[10],
    0,
     other[8] * self[0] + other[9] * self[4] + other[10]* self[8],
    other[8] * self[1] + other[9] * self[5] + other[10]* self[9],
    other[8] * self[2] + other[9] * self[6] + other[10]* self[10],
    0,
    other[12]* self[0] + other[13]* self[4] + other[14]* self[8] + self[12],
    other[12]* self[1] + other[13]* self[5] + other[14]* self[9] + self[13],
    other[12]* self[2] + other[13]* self[6] + other[14]* self[10]+ self[14],
    1
  );
}

/// \brief Post-multiplies \p self by \p other in-place.
/// \p self and \p other must be affine.
inline void matrix4_affine_multiply_by_matrix4(Matrix4& self, const Matrix4& other)
{
  self = matrix4_affine_multiplied_by_matrix4(self, other);
}

/// \brief Returns \p self pre-multiplied by \p other.
/// \p self and \p other must be affine.
inline Matrix4 matrix4_affine_premultiplied_by_matrix4(const Matrix4& self, const Matrix4& other)
{
#if 1
  return matrix4_affine_multiplied_by_matrix4(other, self);
#else
  return Matrix4::byColumns(
    self[0] * other[0] + self[1] * other[4] + self[2] * other[8],
    self[0] * other[1] + self[1] * other[5] + self[2] * other[9],
    self[0] * other[2] + self[1] * other[6] + self[2] * other[10],
    0,
    self[4] * other[0] + self[5] * other[4] + self[6] * other[8],
    self[4] * other[1] + self[5] * other[5] + self[6] * other[9],
    self[4] * other[2] + self[5] * other[6] + self[6] * other[10],
    0,
    self[8] * other[0] + self[9] * other[4] + self[10]* other[8],
    self[8] * other[1] + self[9] * other[5] + self[10]* other[9],
    self[8] * other[2] + self[9] * other[6] + self[10]* other[10],
    0,
    self[12]* other[0] + self[13]* other[4] + self[14]* other[8] + other[12],
    self[12]* other[1] + self[13]* other[5] + self[14]* other[9] + other[13],
    self[12]* other[2] + self[13]* other[6] + self[14]* other[10]+ other[14],
    1
    )
  );
#endif
}

/// \brief Pre-multiplies \p self by \p other in-place.
/// \p self and \p other must be affine.
inline void matrix4_affine_premultiply_by_matrix4(Matrix4& self, const Matrix4& other)
{
  self = matrix4_affine_premultiplied_by_matrix4(self, other);
}

/// \brief Returns \p point transformed by \p self.
template<typename Element>
inline BasicVector3<Element> matrix4_transformed_point(const Matrix4& self, const BasicVector3<Element>& point)
{
  return BasicVector3<Element>(
    static_cast<Element>(self[0]  * point[0] + self[4]  * point[1] + self[8]  * point[2] + self[12]),
    static_cast<Element>(self[1]  * point[0] + self[5]  * point[1] + self[9]  * point[2] + self[13]),
    static_cast<Element>(self[2]  * point[0] + self[6]  * point[1] + self[10] * point[2] + self[14])
  );
}

/// \brief Transforms \p point by \p self in-place.
template<typename Element>
inline void matrix4_transform_point(const Matrix4& self, BasicVector3<Element>& point)
{
  point = matrix4_transformed_point(self, point);
}

/// \brief Returns \p direction transformed by \p self.
template<typename Element>
inline BasicVector3<Element> matrix4_transformed_direction(const Matrix4& self, const BasicVector3<Element>& direction)
{
  return BasicVector3<Element>(
    static_cast<Element>(self[0]  * direction[0] + self[4]  * direction[1] + self[8]  * direction[2]),
    static_cast<Element>(self[1]  * direction[0] + self[5]  * direction[1] + self[9]  * direction[2]),
    static_cast<Element>(self[2]  * direction[0] + self[6]  * direction[1] + self[10] * direction[2])
  );
}

/// \brief Transforms \p direction by \p self in-place.
template<typename Element>
inline void matrix4_transform_direction(const Matrix4& self, BasicVector3<Element>& normal)
{
  normal = matrix4_transformed_direction(self, normal);
}

/// \brief Returns \p vector4 transformed by \p self.
inline Vector4 matrix4_transformed_vector4(const Matrix4& self, const Vector4& vector4)
{
  return Vector4(
    self[0]  * vector4[0] + self[4]  * vector4[1] + self[8]  * vector4[2] + self[12] * vector4[3],
    self[1]  * vector4[0] + self[5]  * vector4[1] + self[9]  * vector4[2] + self[13] * vector4[3],
    self[2]  * vector4[0] + self[6]  * vector4[1] + self[10] * vector4[2] + self[14] * vector4[3],
    self[3]  * vector4[0] + self[7]  * vector4[1] + self[11] * vector4[2] + self[15] * vector4[3]
  );
}

/// \brief Transforms \p vector4 by \p self in-place.
inline void matrix4_transform_vector4(const Matrix4& self, Vector4& vector4)
{
  vector4 = matrix4_transformed_vector4(self, vector4);
}

inline void matrix4_affine_invert(Matrix4& self)
{
  self = self.getInverse();
}

/// \brief A compile-time-constant integer.
template<int VALUE_>
struct IntegralConstant
{
  enum unnamed_{ VALUE = VALUE_ };
};

/// \brief A compile-time-constant row/column index into a 4x4 matrix.
template<typename Row, typename Col>
class Matrix4Index
{
public:
  typedef IntegralConstant<Row::VALUE> r;
  typedef IntegralConstant<Col::VALUE> c;
  typedef IntegralConstant<(r::VALUE * 4) + c::VALUE> i;
};

/// \brief A functor which returns the cofactor of a 3x3 submatrix obtained by ignoring a given row and column of a 4x4 matrix.
/// \param Row Defines the compile-time-constant integers x, y and z with values corresponding to the indices of the three rows to use.
/// \param Col Defines the compile-time-constant integers x, y and z with values corresponding to the indices of the three columns to use.
template<typename Row, typename Col>
class Matrix4Cofactor
{
public:
  typedef typename Matrix4Index<typename Row::x, typename Col::x>::i xx;
  typedef typename Matrix4Index<typename Row::x, typename Col::y>::i xy;
  typedef typename Matrix4Index<typename Row::x, typename Col::z>::i xz;
  typedef typename Matrix4Index<typename Row::y, typename Col::x>::i yx;
  typedef typename Matrix4Index<typename Row::y, typename Col::y>::i yy;
  typedef typename Matrix4Index<typename Row::y, typename Col::z>::i yz;
  typedef typename Matrix4Index<typename Row::z, typename Col::x>::i zx;
  typedef typename Matrix4Index<typename Row::z, typename Col::y>::i zy;
  typedef typename Matrix4Index<typename Row::z, typename Col::z>::i zz;
  static float apply(const Matrix4& self)
  {
    return self[xx::VALUE] * ( self[yy::VALUE]*self[zz::VALUE] - self[zy::VALUE]*self[yz::VALUE] )
      - self[xy::VALUE] * ( self[yx::VALUE]*self[zz::VALUE] - self[zx::VALUE]*self[yz::VALUE] )
      + self[xz::VALUE] * ( self[yx::VALUE]*self[zy::VALUE] - self[zx::VALUE]*self[yy::VALUE] );
  }
};

/// \brief The cofactor element indices for a 4x4 matrix row or column.
/// \param Element The index of the element to ignore.
template<int Element>
class Cofactor4
{
public:
  typedef IntegralConstant<(Element <= 0) ? 1 : 0> x;
  typedef IntegralConstant<(Element <= 1) ? 2 : 1> y;
  typedef IntegralConstant<(Element <= 2) ? 3 : 2> z;
};

/// \brief Returns the determinant of \p self.
inline float matrix4_determinant(const Matrix4& self)
{
  return self.xx() * Matrix4Cofactor< Cofactor4<0>, Cofactor4<0> >::apply(self)
    - self.xy() * Matrix4Cofactor< Cofactor4<0>, Cofactor4<1> >::apply(self)
    + self.xz() * Matrix4Cofactor< Cofactor4<0>, Cofactor4<2> >::apply(self)
    - self.xw() * Matrix4Cofactor< Cofactor4<0>, Cofactor4<3> >::apply(self);
}

/// \brief Returns the inverse of \p self using the Adjoint method.
/// \todo Throw an exception if the determinant is zero.
inline Matrix4 matrix4_full_inverse(const Matrix4& self)
{
  float determinant = 1.0f / matrix4_determinant(self);

  return Matrix4::byColumns(
    static_cast<float>( Matrix4Cofactor< Cofactor4<0>, Cofactor4<0> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<1>, Cofactor4<0> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<2>, Cofactor4<0> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<3>, Cofactor4<0> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<0>, Cofactor4<1> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<1>, Cofactor4<1> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<2>, Cofactor4<1> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<3>, Cofactor4<1> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<0>, Cofactor4<2> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<1>, Cofactor4<2> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<2>, Cofactor4<2> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<3>, Cofactor4<2> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<0>, Cofactor4<3> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<1>, Cofactor4<3> >::apply(self) * determinant),
    static_cast<float>(-Matrix4Cofactor< Cofactor4<2>, Cofactor4<3> >::apply(self) * determinant),
    static_cast<float>( Matrix4Cofactor< Cofactor4<3>, Cofactor4<3> >::apply(self) * determinant)
  );
}

/// \brief Inverts \p self in-place using the Adjoint method.
inline void matrix4_full_invert(Matrix4& self)
{
  self = matrix4_full_inverse(self);
}


/// \brief Returns the translation part of \p self.
inline Vector3 matrix4_get_translation_vec3(const Matrix4& self)
{
  return self.t().getVector3();
}

/// \brief Returns \p self Concatenated with \p translation.
/// The concatenated translation occurs before \p self.
inline Matrix4 matrix4_translated_by_vec3(const Matrix4& self, const Vector3& translation)
{
	return self.getMultipliedBy(Matrix4::getTranslation(translation));
}


#include "math/pi.h"

/// \brief Returns \p angle modulated by the range [0, 360).
/// \p angle must be in the range [-360, 360).
inline float angle_modulate_degrees_range(float angle)
{
  return static_cast<float>(float_mod_range(angle, 360.0));
}

/// \brief Returns \p euler angles converted from radians to degrees.
inline Vector3 euler_radians_to_degrees(const Vector3& euler)
{
  return Vector3(
    static_cast<float>(radians_to_degrees(euler.x())),
    static_cast<float>(radians_to_degrees(euler.y())),
    static_cast<float>(radians_to_degrees(euler.z()))
  );
}

/// \brief Returns \p euler angles converted from degrees to radians.
inline Vector3 euler_degrees_to_radians(const Vector3& euler)
{
  return Vector3(
    static_cast<float>(degrees_to_radians(euler.x())),
    static_cast<float>(degrees_to_radians(euler.y())),
    static_cast<float>(degrees_to_radians(euler.z()))
  );
}



/// \brief Constructs a pure-rotation matrix about the x axis from sin \p s and cosine \p c of an angle.
inline Matrix4 matrix4_rotation_for_sincos_x(float s, float c)
{
  return Matrix4::byColumns(
    1, 0, 0, 0,
    0, c, s, 0,
    0,-s, c, 0,
    0, 0, 0, 1
  );
}

/// \brief Constructs a pure-rotation matrix about the x axis from an angle in radians.
inline Matrix4 matrix4_rotation_for_x(float x)
{
  return matrix4_rotation_for_sincos_x(sin(x), cos(x));
}

/// \brief Constructs a pure-rotation matrix about the x axis from an angle in degrees.
inline Matrix4 matrix4_rotation_for_x_degrees(float x)
{
  return matrix4_rotation_for_x(degrees_to_radians(x));
}

/// \brief Constructs a pure-rotation matrix about the y axis from sin \p s and cosine \p c of an angle.
inline Matrix4 matrix4_rotation_for_sincos_y(float s, float c)
{
  return Matrix4::byColumns(
    c, 0,-s, 0,
    0, 1, 0, 0,
    s, 0, c, 0,
    0, 0, 0, 1
  );
}

/// \brief Constructs a pure-rotation matrix about the y axis from an angle in radians.
inline Matrix4 matrix4_rotation_for_y(float y)
{
  return matrix4_rotation_for_sincos_y(sin(y), cos(y));
}

/// \brief Constructs a pure-rotation matrix about the y axis from an angle in degrees.
inline Matrix4 matrix4_rotation_for_y_degrees(float y)
{
  return matrix4_rotation_for_y(degrees_to_radians(y));
}

/// \brief Constructs a pure-rotation matrix about the z axis from sin \p s and cosine \p c of an angle.
inline Matrix4 matrix4_rotation_for_sincos_z(float s, float c)
{
  return Matrix4::byColumns(
    c, s, 0, 0,
   -s, c, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  );
}

/// \brief Constructs a pure-rotation matrix about the z axis from an angle in radians.
inline Matrix4 matrix4_rotation_for_z(float z)
{
  return matrix4_rotation_for_sincos_z(sin(z), cos(z));
}

/// \brief Constructs a pure-rotation matrix about the z axis from an angle in degrees.
inline Matrix4 matrix4_rotation_for_z_degrees(float z)
{
  return matrix4_rotation_for_z(degrees_to_radians(z));
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (x, y, z).
/*! \verbatim
clockwise rotation around X, Y, Z, facing along axis
 1  0   0    cy 0 -sy   cz  sz 0
 0  cx  sx   0  1  0   -sz  cz 0
 0 -sx  cx   sy 0  cy   0   0  1

rows of Z by cols of Y
 cy*cz -sy*cz+sz -sy*sz+cz
-sz*cy -sz*sy+cz

  .. or something like that..

final rotation is Z * Y * X
 cy*cz -sx*-sy*cz+cx*sz  cx*-sy*sz+sx*cz
-cy*sz  sx*sy*sz+cx*cz  -cx*-sy*sz+sx*cz
 sy    -sx*cy            cx*cy

transposed
cy.cz + 0.sz + sy.0            cy.-sz + 0 .cz +  sy.0          cy.0  + 0 .0  +   sy.1       |
sx.sy.cz + cx.sz + -sx.cy.0    sx.sy.-sz + cx.cz + -sx.cy.0    sx.sy.0  + cx.0  + -sx.cy.1  |
-cx.sy.cz + sx.sz +  cx.cy.0   -cx.sy.-sz + sx.cz +  cx.cy.0   -cx.sy.0  + 0 .0  +  cx.cy.1  |
\endverbatim */
inline Matrix4 matrix4_rotation_for_euler_xyz(const Vector3& euler)
{
#if 1

  float cx = cos(euler[0]);
  float sx = sin(euler[0]);
  float cy = cos(euler[1]);
  float sy = sin(euler[1]);
  float cz = cos(euler[2]);
  float sz = sin(euler[2]);

  return Matrix4::byColumns(
    cy*cz,
    cy*sz,
    -sy,
    0,
    sx*sy*cz + cx*-sz,
    sx*sy*sz + cx*cz,
    sx*cy,
    0,
    cx*sy*cz + sx*sz,
    cx*sy*sz + -sx*cz,
    cx*cy,
    0,
    0,
    0,
    0,
    1
  );

#else

  return matrix4_premultiply_by_matrix4(
    matrix4_premultiply_by_matrix4(
      matrix4_rotation_for_x(euler[0]),
      matrix4_rotation_for_y(euler[1])
    ),
    matrix4_rotation_for_z(euler[2])
  );

#endif
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (x, y, z).
inline Matrix4 matrix4_rotation_for_euler_xyz_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_xyz(euler_degrees_to_radians(euler));
}

/// \brief Concatenates \p self with the rotation transform produced by \p euler angles (degrees) in the order (x, y, z).
/// The concatenated rotation occurs before \p self.
inline void matrix4_rotate_by_euler_xyz_degrees(Matrix4& self, const Vector3& euler)
{
  self.multiplyBy(matrix4_rotation_for_euler_xyz_degrees(euler));
}


/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (y, z, x).
inline Matrix4 matrix4_rotation_for_euler_yzx(const Vector3& euler)
{
	return matrix4_rotation_for_y(euler[1]).getPremultipliedBy(
		matrix4_rotation_for_z(euler[2])).getPremultipliedBy(matrix4_rotation_for_x(euler[0])
	);
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (y, z, x).
inline Matrix4 matrix4_rotation_for_euler_yzx_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_yzx(euler_degrees_to_radians(euler));
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (x, z, y).
inline Matrix4 matrix4_rotation_for_euler_xzy(const Vector3& euler)
{
	return matrix4_rotation_for_x(euler[0]).getPremultipliedBy(
		matrix4_rotation_for_z(euler[2])).getPremultipliedBy(matrix4_rotation_for_y(euler[1])
	);
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (x, z, y).
inline Matrix4 matrix4_rotation_for_euler_xzy_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_xzy(euler_degrees_to_radians(euler));
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (y, x, z).
/*! \verbatim
|  cy.cz + sx.sy.-sz + -cx.sy.0   0.cz + cx.-sz + sx.0   sy.cz + -sx.cy.-sz + cx.cy.0 |
|  cy.sz + sx.sy.cz + -cx.sy.0    0.sz + cx.cz + sx.0    sy.sz + -sx.cy.cz + cx.cy.0  |
|  cy.0 + sx.sy.0 + -cx.sy.1      0.0 + cx.0 + sx.1      sy.0 + -sx.cy.0 + cx.cy.1    |
\endverbatim */
inline Matrix4 matrix4_rotation_for_euler_yxz(const Vector3& euler)
{
#if 1

  float cx = cos(euler[0]);
  float sx = sin(euler[0]);
  float cy = cos(euler[1]);
  float sy = sin(euler[1]);
  float cz = cos(euler[2]);
  float sz = sin(euler[2]);

  return Matrix4::byColumns(
    cy*cz + sx*sy*-sz,
    cy*sz + sx*sy*cz,
    -cx*sy,
    0,
    cx*-sz,
    cx*cz,
    sx,
    0,
    sy*cz + -sx*cy*-sz,
    sy*sz + -sx*cy*cz,
    cx*cy,
    0,
    0,
    0,
    0,
    1
  );

#else

  return matrix4_premultiply_by_matrix4(
    matrix4_premultiply_by_matrix4(
      matrix4_rotation_for_y(euler[1]),
      matrix4_rotation_for_x(euler[0])
    ),
    matrix4_rotation_for_z(euler[2])
  );

#endif
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (y, x, z).
inline Matrix4 matrix4_rotation_for_euler_yxz_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_yxz(euler_degrees_to_radians(euler));
}

/// \brief Returns \p self concatenated with the rotation transform produced by \p euler angles (degrees) in the order (y, x, z).
/// The concatenated rotation occurs before \p self.
inline Matrix4 matrix4_rotated_by_euler_yxz_degrees(const Matrix4& self, const Vector3& euler)
{
	return self.getMultipliedBy(matrix4_rotation_for_euler_yxz_degrees(euler));
}

/// \brief Concatenates \p self with the rotation transform produced by \p euler angles (degrees) in the order (y, x, z).
/// The concatenated rotation occurs before \p self.
inline void matrix4_rotate_by_euler_yxz_degrees(Matrix4& self, const Vector3& euler)
{
  self = matrix4_rotated_by_euler_yxz_degrees(self, euler);
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (z, x, y).
inline Matrix4 matrix4_rotation_for_euler_zxy(const Vector3& euler)
{
#if 1
	return matrix4_rotation_for_z(euler[2]).getPremultipliedBy(
		matrix4_rotation_for_x(euler[0])).getPremultipliedBy(matrix4_rotation_for_y(euler[1])
	);
#else
  float cx = cos(euler[0]);
  float sx = sin(euler[0]);
  float cy = cos(euler[1]);
  float sy = sin(euler[1]);
  float cz = cos(euler[2]);
  float sz = sin(euler[2]);

  return Matrix4::byColumns(
    cz * cy + sz * sx * sy,
    sz * cx,
    cz * -sy + sz * sx * cy,
    0,
    -sz * cy + cz * sx * sy,
    cz * cx,
    -sz * -sy + cz * cx * cy,
    0,
    cx* sy,
    -sx,
    cx* cy,
    0,
    0,
    0,
    0,
    1
  );
#endif
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degres=es) in the order (z, x, y).
inline Matrix4 matrix4_rotation_for_euler_zxy_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_zxy(euler_degrees_to_radians(euler));
}

/// \brief Returns \p self concatenated with the rotation transform produced by \p euler angles (degrees) in the order (z, x, y).
/// The concatenated rotation occurs before \p self.
inline Matrix4 matrix4_rotated_by_euler_zxy_degrees(const Matrix4& self, const Vector3& euler)
{
	return self.getMultipliedBy(matrix4_rotation_for_euler_zxy_degrees(euler));
}

/// \brief Concatenates \p self with the rotation transform produced by \p euler angles (degrees) in the order (z, x, y).
/// The concatenated rotation occurs before \p self.
inline void matrix4_rotate_by_euler_zxy_degrees(Matrix4& self, const Vector3& euler)
{
  self = matrix4_rotated_by_euler_zxy_degrees(self, euler);
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (z, y, x).
inline Matrix4 matrix4_rotation_for_euler_zyx(const Vector3& euler)
{
#if 1

  float cx = cos(euler[0]);
  float sx = sin(euler[0]);
  float cy = cos(euler[1]);
  float sy = sin(euler[1]);
  float cz = cos(euler[2]);
  float sz = sin(euler[2]);

  return Matrix4::byColumns(
    cy*cz,
    sx*sy*cz + cx*sz,
    cx*-sy*cz + sx*sz,
    0,
    cy*-sz,
    sx*sy*-sz + cx*cz,
    cx*-sy*-sz + sx*cz,
    0,
    sy,
    -sx*cy,
    cx*cy,
    0,
    0,
    0,
    0,
    1
  );

#else

  return matrix4_premultiply_by_matrix4(
    matrix4_premultiply_by_matrix4(
      matrix4_rotation_for_z(euler[2]),
      matrix4_rotation_for_y(euler[1])
    ),
    matrix4_rotation_for_x(euler[0])
  );

#endif
}

/// \brief Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (z, y, x).
inline Matrix4 matrix4_rotation_for_euler_zyx_degrees(const Vector3& euler)
{
  return matrix4_rotation_for_euler_zyx(euler_degrees_to_radians(euler));
}


/// \brief Calculates and returns a set of euler angles that produce the rotation component of \p self when applied in the order (x, y, z).
/// \p self must be affine and orthonormal (unscaled) to produce a meaningful result.
inline Vector3 matrix4_get_rotation_euler_xyz(const Matrix4& self)
{
  float a = asin(-self[2]);
  float ca = cos(a);

  if (fabs(ca) > 0.005) // Gimbal lock?
  {
    return Vector3(
      atan2(self[6] / ca, self[10] / ca),
      a,
      atan2(self[1] / ca, self[0]/ ca)
    );
  }
  else // Gimbal lock has occurred
  {
    return Vector3(
      atan2(-self[9], self[5]),
      a,
      0
    );
  }
}

/// \brief \copydoc matrix4_get_rotation_euler_xyz(const Matrix4&)
inline Vector3 matrix4_get_rotation_euler_xyz_degrees(const Matrix4& self)
{
  return euler_radians_to_degrees(matrix4_get_rotation_euler_xyz(self));
}

/// \brief Calculates and returns a set of euler angles that produce the rotation component of \p self when applied in the order (y, x, z).
/// \p self must be affine and orthonormal (unscaled) to produce a meaningful result.
inline Vector3 matrix4_get_rotation_euler_yxz(const Matrix4& self)
{
  float a = asin(self[6]);
  float ca = cos(a);

  if (fabs(ca) > 0.005) // Gimbal lock?
  {
    return Vector3(
      a,
      atan2(-self[2] / ca, self[10]/ ca),
      atan2(-self[4] / ca, self[5] / ca)
    );
  }
  else // Gimbal lock has occurred
  {
    return Vector3(
      a,
      atan2(self[8], self[0]),
      0
    );
  }
}

/// \brief \copydoc matrix4_get_rotation_euler_yxz(const Matrix4&)
inline Vector3 matrix4_get_rotation_euler_yxz_degrees(const Matrix4& self)
{
  return euler_radians_to_degrees(matrix4_get_rotation_euler_yxz(self));
}

/// \brief Calculates and returns a set of euler angles that produce the rotation component of \p self when applied in the order (z, x, y).
/// \p self must be affine and orthonormal (unscaled) to produce a meaningful result.
inline Vector3 matrix4_get_rotation_euler_zxy(const Matrix4& self)
{
  float a = asin(-self[9]);
  float ca = cos(a);

  if (fabs(ca) > 0.005) // Gimbal lock?
  {
    return Vector3(
      a,
      atan2(self[8] / ca, self[10] / ca),
      atan2(self[1] / ca, self[5]/ ca)
    );
  }
  else // Gimbal lock has occurred
  {
    return Vector3(
      a,
      0,
      atan2(-self[4], self[0])
    );
  }
}

/// \brief \copydoc matrix4_get_rotation_euler_zxy(const Matrix4&)
inline Vector3 matrix4_get_rotation_euler_zxy_degrees(const Matrix4& self)
{
  return euler_radians_to_degrees(matrix4_get_rotation_euler_zxy(self));
}

/// \brief Calculates and returns a set of euler angles that produce the rotation component of \p self when applied in the order (z, y, x).
/// \p self must be affine and orthonormal (unscaled) to produce a meaningful result.
inline Vector3 matrix4_get_rotation_euler_zyx(const Matrix4& self)
{
  float a = asin(self[8]);
  float ca = cos(a);

  if (fabs(ca) > 0.005) // Gimbal lock?
  {
    return Vector3(
      atan2(-self[9] / ca, self[10]/ ca),
      a,
      atan2(-self[4] / ca, self[0] / ca)
    );
  }
  else // Gimbal lock has occurred
  {
    return Vector3(
      0,
      a,
      atan2(self[1], self[5])
    );
  }
}

/// \brief \copydoc matrix4_get_rotation_euler_zyx(const Matrix4&)
inline Vector3 matrix4_get_rotation_euler_zyx_degrees(const Matrix4& self)
{
  return euler_radians_to_degrees(matrix4_get_rotation_euler_zyx(self));
}


/// \brief Rotate \p self by \p euler angles (degrees) applied in the order (x, y, z), using \p pivotpoint.
inline void matrix4_pivoted_rotate_by_euler_xyz_degrees(Matrix4& self, const Vector3& euler, const Vector3& pivotpoint)
{
  self.translateBy(pivotpoint);
  matrix4_rotate_by_euler_xyz_degrees(self, euler);
  self.translateBy(-pivotpoint);
}

/// \brief Calculates and returns the (x, y, z) scale values that produce the scale component of \p self.
/// \p self must be affine and orthogonal to produce a meaningful result.
inline Vector3 matrix4_get_scale_vec3(const Matrix4& self)
{
  return Vector3(
    self.x().getVector3().getLength(),
    self.y().getVector3().getLength(),
    self.z().getVector3().getLength()
  );
}

/// \brief Scales \p self by \p scale, using \p pivotpoint.
inline void matrix4_pivoted_scale_by_vec3(Matrix4& self, const Vector3& scale, const Vector3& pivotpoint)
{
  self.translateBy(pivotpoint);
  self.scaleBy(scale);
  self.translateBy(-pivotpoint);
}


/// \brief Transforms \p self by \p translation, \p euler and \p scale.
/// The transforms are combined in the order: scale, rotate-z, rotate-y, rotate-x, translate.
inline void matrix4_transform_by_euler_xyz_degrees(Matrix4& self, const Vector3& translation, const Vector3& euler, const Vector3& scale)
{
  self.translateBy(translation);
  matrix4_rotate_by_euler_xyz_degrees(self, euler);
  self.scaleBy(scale);
}

/// \brief Transforms \p self by \p translation, \p euler and \p scale, using \p pivotpoint.
inline void matrix4_pivoted_transform_by_euler_xyz_degrees(Matrix4& self, const Vector3& translation, const Vector3& euler, const Vector3& scale, const Vector3& pivotpoint)
{
  self.translateBy(pivotpoint + translation);
  matrix4_rotate_by_euler_xyz_degrees(self, euler);
  self.scaleBy(scale);
  self.translateBy(-pivotpoint);
}

/** Stream insertion operator for Matrix4.
 */
inline std::ostream& operator<<(std::ostream& st, const Matrix4& m)
{
	st << "|" << m[0] << ", " << m[4] << ", " << m[8] << ", " << m[12] << "|\n";
	st << "|" << m[1] << ", " << m[5] << ", " << m[9] << ", " << m[13] << "|\n";
	st << "|" << m[2] << ", " << m[6] << ", " << m[10] << ", " << m[14] << "|\n";
	st << "|" << m[3] << ", " << m[7] << ", " << m[11] << ", " << m[15] << "|\n";
	return st;
}
