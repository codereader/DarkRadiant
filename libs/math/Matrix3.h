#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "eigen.h"

/**
 * \brief A 3x3 matrix stored in double-precision floating-point.
 *
 * The 3 columns may regarded as 3 vectors named x, y, z:
 *
 * | xx  yx  zx |
 * | xy  yy  zy |
 * | xz  yz  zz |
 *
 */
class Matrix3
{
    // Underlying Eigen transform object (which can in turn be reduced to a 3x3 Matrix)
    using Transform = Eigen::Projective2d;
    Transform _transform;

    // Initialising constructor, elements are passed in column-wise order
    Matrix3(double xx_, double xy_, double xz_,
        double yx_, double yy_, double yz_,
        double zx_, double zy_, double zz_);

public:
    /// Construct a matrix with uninitialised values.
    Matrix3() { }

    /// Construct from Eigen transform
    explicit Matrix3(const Eigen::Projective2d& t) :
        _transform(t)
    {}

    /// Get the underlying Eigen transform
    Eigen::Projective2d& eigen() { return _transform; }

    /// Get the underlying const Eigen transform
    const Eigen::Projective2d& eigen() const { return _transform; }

    /**
     * Return matrix elements
     * \{
     */
    double& xx() { return _transform.matrix()(0, 0); }
    const double& xx() const { return _transform.matrix()(0, 0); }
    double& xy() { return _transform.matrix()(1, 0); }
    const double& xy() const { return _transform.matrix()(1, 0); }
    double& xz() { return _transform.matrix()(2, 0); }
    const double& xz() const { return _transform.matrix()(2, 0); }

    double& yx() { return _transform.matrix()(0, 1); }
    const double& yx() const { return _transform.matrix()(0, 1); }
    double& yy() { return _transform.matrix()(1, 1); }
    const double& yy() const { return _transform.matrix()(1, 1); }
    double& yz() { return _transform.matrix()(2, 1); }
    const double& yz() const { return _transform.matrix()(2, 1); }

    double& zx() { return _transform.matrix()(0, 2); }
    const double& zx() const { return _transform.matrix()(0, 2); }
    double& zy() { return _transform.matrix()(1, 2); }
    const double& zy() const { return _transform.matrix()(1, 2); }
    double& zz() { return _transform.matrix()(2, 2); }
    const double& zz() const { return _transform.matrix()(2, 2); }
    /**
     * \}
     */

    /// Obtain the identity matrix.
    static Matrix3 getIdentity()
    {
        return Matrix3(Eigen::Projective2d::Identity());
    }

    /// Get a matrix representing the given 2D translation.
    static Matrix3 getTranslation(const Vector2& translation)
    {
        return Matrix3(Transform(Eigen::Translation2d(translation.x(), translation.y())));
    }

    /// Get a matrix representing the given 2D rotation (counter-clockwise, angle in radians)
    static Matrix3 getRotation(double angle)
    {
        return Matrix3(Transform(Eigen::Rotation2D(angle)));
    }

    /// Get a matrix representing the given 2D scale
    static Matrix3 getScale(const Vector2& scale)
    {
        return Matrix3(Transform(Eigen::Scaling(scale.x(), scale.y())));
    }

    /**
     * \brief
     * Construct a matrix containing the given elements.
     *
     * The elements are specified column-wise, starting with the left-most
     * column.
     */
    static Matrix3 byColumns(double xx, double xy, double xz,
        double yx, double yy, double yz,
        double zx, double zy, double zz);

    /**
     * \brief
     * Construct a matrix containing the given elements.
     *
     * The elements are specified row-wise, starting with the top row.
     */
    static Matrix3 byRows(double xx, double yx, double zx,
        double xy, double yy, double zy,
        double xz, double yz, double zz);

    /// Return the result of this matrix post-multiplied by another matrix.
    Matrix3 getMultipliedBy(const Matrix3& other) const
    {
        return Matrix3(_transform * other.eigen());
    }

    /// Post-multiply this matrix by another matrix, in-place.
    void multiplyBy(const Matrix3& other)
    {
        *this = getMultipliedBy(other);
    }

    /// Returns this matrix pre-multiplied by the other
    Matrix3 getPremultipliedBy(const Matrix3& other) const
    {
        return other.getMultipliedBy(*this);
    }

    /// Pre-multiplies this matrix by other in-place.
    void premultiplyBy(const Matrix3& other)
    {
        *this = getPremultipliedBy(other);
    }

    /// Return the full inverse of this matrix.
    Matrix3 getFullInverse() const
    {
        return Matrix3(_transform.inverse(Eigen::Projective));
    }

    /// Invert this matrix in-place.
    void invertFull()
    {
        *this = getFullInverse();
    }

    /**
     * \brief Returns the given 2-component point transformed by this matrix.
     *
     * The point is assumed to have a W component of 1, and no division by W is
     * performed before returning the 3-component vector.
     */
    template<typename T>
    BasicVector2<T> transformPoint(const BasicVector2<T>& point) const
    {
        auto transformed = transform(BasicVector3<T>(point.x(), point.y(), 1));
        return BasicVector2<T>(transformed.x(), transformed.y());
    }

    /// Return the given 3-component vector transformed by this matrix.
    template<typename T>
    BasicVector3<T> transform(const BasicVector3<T>& vector3) const;
};

// Private constructor
inline Matrix3::Matrix3(double xx_, double xy_, double xz_,
    double yx_, double yy_, double yz_,
    double zx_, double zy_, double zz_)
{
    xx() = xx_;
    xy() = xy_;
    xz() = xz_;
    yx() = yx_;
    yy() = yy_;
    yz() = yz_;
    zx() = zx_;
    zy() = zy_;
    zz() = zz_;
}

// Construct a matrix with given column elements
inline Matrix3 Matrix3::byColumns(double xx, double xy, double xz,
    double yx, double yy, double yz,
    double zx, double zy, double zz)
{
    return Matrix3(xx, xy, xz,
        yx, yy, yz,
        zx, zy, zz);
}

// Construct a matrix with given row elements
inline Matrix3 Matrix3::byRows(double xx, double yx, double zx,
    double xy, double yy, double zy,
    double xz, double yz, double zz)
{
    return Matrix3(xx, xy, xz,
        yx, yy, yz,
        zx, zy, zz);
}

template<typename T>
inline BasicVector3<T> Matrix3::transform(const BasicVector3<T>& vector3) const
{
    Eigen::Matrix<T, 3, 1> eVec(static_cast<const double*>(vector3));
    auto result = _transform * eVec;
    return BasicVector3<T>(result[0], result[1], result[2]);
}

/// Compare two matrices elementwise for equality
inline bool operator==(const Matrix3& l, const Matrix3& r)
{
    return l.eigen().matrix() == r.eigen().matrix();
}

/// Compare two matrices elementwise for inequality
inline bool operator!=(const Matrix3& l, const Matrix3& r)
{
    return !(l == r);
}

/// Multiply two matrices together
inline Matrix3 operator*(const Matrix3& m1, const Matrix3& m2)
{
    return m1.getMultipliedBy(m2);
}

/**
 * \brief Multiply a 3-component vector by this matrix.
 *
 * Equivalent to m.transform(v).
 */
template<typename T>
inline BasicVector3<T> operator*(const Matrix3& m, const BasicVector3<T>& v)
{
    return m.transform(v);
}

/**
 * \brief Multiply a 2-component vector by this matrix.
 *
 * The vector is upgraded to a 3-component vector with a Z (or W) component of 1, i.e.
 * equivalent to m.transformPoint(v).
 */
template<typename T>
inline BasicVector2<T> operator*(const Matrix3& m, const BasicVector2<T>& v)
{
    return m.transformPoint(v);
}
