#pragma once

/// \file
/// \brief Matrix data types and related operations.

#include "Vector3.h"
#include "Vector4.h"
#include "pi.h"
#include "eigen.h"

class Quaternion;

/**
 * \brief A 4x4 matrix stored in double-precision floating-point.
 *
 * The 4 columns may regarded as 4 vectors named x, y, z, t:
 *
 * | xx   yx   zx   tx |
 * | xy   yy   zy   ty |
 * | xz   yz   zz   tz |
 * | xw   yw   zw   tw |
 */
class alignas(16) Matrix4
{
    // Underlying Eigen transform object (which can in turn be reduced to a 4x4
    // Matrix)
    using Transform = Eigen::Projective3d;
    Transform _transform;

private:

    // Initialising constructor, elements are passed in column-wise order
    Matrix4(double xx_, double xy_, double xz_, double xw_,
            double yx_, double yy_, double yz_, double yw_,
            double zx_, double zy_, double zz_, double zw_,
            double tx_, double ty_, double tz_, double tw_);

    // Construct a pure-rotation matrix about the X axis from sin and cosine of
    // an angle in radians
    static Matrix4 getRotationAboutXForSinCos(double s, double c);

    // Construct a pure-rotation matrix about the Y axis from sin and cosine of
    // an angle in radians
    static Matrix4 getRotationAboutYForSinCos(double s, double c);

    // Construct a pure-rotation matrix about the Z axis from sin and cosine of
    // an angle in radians
    static Matrix4 getRotationAboutZForSinCos(double s, double c);

public:

    /// Construct a matrix with uninitialised values.
    Matrix4() { }

    /// Construct from Eigen transform
    explicit Matrix4(const Eigen::Projective3d& t): _transform(t)
    {}

    /// Get the underlying Eigen transform
    Eigen::Projective3d& eigen() { return _transform; }

    /// Get the underlying const Eigen transform
    const Eigen::Projective3d& eigen() const { return _transform; }

    /// Obtain the identity matrix.
    static Matrix4 getIdentity()
    {
        return Matrix4(Eigen::Projective3d::Identity());
    }

    /// Get a matrix representing the given 3D translation.
    static Matrix4 getTranslation(const Vector3& tr)
    {
        return Matrix4(Transform(Eigen::Translation3d(tr.x(), tr.y(), tr.z())));
    }

    /**
     * \brief Construct a rotation from one vector onto another vector.
     *
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
    static Matrix4 getRotation(const Vector3& axis, const double angle);

    /**
     * Constructs a pure-rotation matrix from the given quaternion.
     */
    static Matrix4 getRotation(const Quaternion& quaternion);

    /**
     * Constructs a pure-rotation matrix from the given quaternion, quantised.
     */
    static Matrix4 getRotationQuantised(const Quaternion& quaternion);

    /// Construct a rotation matrix about the Z axis for a given angle
    template<typename Unit_T> static Matrix4 getRotationAboutZ(Unit_T angle)
    {
        double radians = angle.asRadians();
        return getRotationAboutZForSinCos(sin(radians), cos(radians));
    }

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (x, y, z).
     */
    static Matrix4 getRotationForEulerXYZ(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (x, y, z).
     */
    static Matrix4 getRotationForEulerXYZDegrees(const Vector3& euler);

    /// Get a matrix representing the given scale in 3D space.
    static Matrix4 getScale(const Vector3& scale)
    {
        return Matrix4(
            Transform(Eigen::Scaling(scale.x(), scale.y(), scale.z()))
        );
    }

    /**
     * \brief Construct a matrix containing the given elements.
     *
     * The elements are specified column-wise, starting with the left-most
     * column.
     */
    static Matrix4 byColumns(double xx, double xy, double xz, double xw,
                             double yx, double yy, double yz, double yw,
                             double zx, double zy, double zz, double zw,
                             double tx, double ty, double tz, double tw);

    /**
     * \brief Construct a matrix containing the given elements.
     *
     * The elements are specified row-wise, starting with the top row.
     */
    static Matrix4 byRows(double xx, double yx, double zx, double tx,
                          double xy, double yy, double zy, double ty,
                          double xz, double yz, double zz, double tz,
                          double xw, double yw, double zw, double tw);

    enum Handedness
    {
        RIGHTHANDED = 0,
        LEFTHANDED = 1,
    };

    /**
     * Return matrix elements
     * \{
     */
    double& xx()             { return _transform.matrix()(0, 0); }
    const double& xx() const { return _transform.matrix()(0, 0); }
    double& xy()             { return _transform.matrix()(1, 0); }
    const double& xy() const { return _transform.matrix()(1, 0); }
    double& xz()             { return _transform.matrix()(2, 0); }
    const double& xz() const { return _transform.matrix()(2, 0); }
    double& xw()             { return _transform.matrix()(3, 0); }
    const double& xw() const { return _transform.matrix()(3, 0); }

    double& yx()             { return _transform.matrix()(0, 1); }
    const double& yx() const { return _transform.matrix()(0, 1); }
    double& yy()             { return _transform.matrix()(1, 1); }
    const double& yy() const { return _transform.matrix()(1, 1); }
    double& yz()             { return _transform.matrix()(2, 1); }
    const double& yz() const { return _transform.matrix()(2, 1); }
    double& yw()             { return _transform.matrix()(3, 1); }
    const double& yw() const { return _transform.matrix()(3, 1); }

    double& zx()             { return _transform.matrix()(0, 2); }
    const double& zx() const { return _transform.matrix()(0, 2); }
    double& zy()             { return _transform.matrix()(1, 2); }
    const double& zy() const { return _transform.matrix()(1, 2); }
    double& zz()             { return _transform.matrix()(2, 2); }
    const double& zz() const { return _transform.matrix()(2, 2); }
    double& zw()             { return _transform.matrix()(3, 2); }
    const double& zw() const { return _transform.matrix()(3, 2); }

    double& tx()             { return _transform.matrix()(0, 3); }
    const double& tx() const { return _transform.matrix()(0, 3); }
    double& ty()             { return _transform.matrix()(1, 3); }
    const double& ty() const { return _transform.matrix()(1, 3); }
    double& tz()             { return _transform.matrix()(2, 3); }
    const double& tz() const { return _transform.matrix()(2, 3); }
    double& tw()             { return _transform.matrix()(3, 3); }
    const double& tw() const { return _transform.matrix()(3, 3); }
    /**
     * \}
     */

    /**
     * Return columns of the matrix as vectors.
     * \{
     */
    Vector3 xCol3() const
    {
        return Vector3(_transform.matrix().col(0).head(3));
    }
    Vector3 yCol3() const
    {
        return Vector3(_transform.matrix().col(1).head(3));
    }
    Vector3 zCol3() const
    {
        return Vector3(_transform.matrix().col(2).head(3));
    }
    Vector4 tCol() const
    {
        return Vector4(translation(), tw());
    }
    /**
     * \}
     */

    /// Set the X column from a Vector3
    void setXCol(const Vector3& vec)
    {
        _transform.matrix().col(0).head(3) = vec.eigen();
    }

    /// Set the Y column from a Vector3
    void setYCol(const Vector3& vec)
    {
        _transform.matrix().col(1).head(3) = vec.eigen();
    }

    /// Set the Z column from a Vector3
    void setZCol(const Vector3& vec)
    {
        _transform.matrix().col(2).head(3) = vec.eigen();
    }

    /**
     * Cast to double* for use with GL functions that accept a double
     * array, also provides operator[].
     */
    operator double* ()
    {
        return _transform.matrix().data();
    }

    /**
     * Cast to const double* to provide operator[] for const objects.
     */
    operator const double* () const
    {
        return _transform.matrix().data();
    }

    /// Transpose this matrix in-place.
    void transpose()
    {
        _transform.matrix().transposeInPlace();
    }

    /// Return a transposed copy of this matrix.
    Matrix4 getTransposed() const
    {
        Matrix4 copy = *this;
        copy.transpose();
        return copy;
    }

    /// Return the affine inverse of this transformation matrix.
    Matrix4 getInverse() const
    {
        return Matrix4(_transform.inverse(Eigen::Affine));
    }

    /// Affine invert this matrix in-place.
    void invert()
    {
        *this = getInverse();
    }

    /// Return the full inverse of this matrix.
    Matrix4 getFullInverse() const
    {
        return Matrix4(_transform.inverse(Eigen::Projective));
    }

    /// Invert this matrix in-place.
    void invertFull()
    {
        *this = getFullInverse();
    }

    /**
     * \brief Returns the given 3-component point transformed by this matrix.
     *
     * The point is assumed to have a W component of 1, and no division by W is
     * performed before returning the 3-component vector.
     */
    template<typename T>
    BasicVector3<T> transformPoint(const BasicVector3<T>& point) const
    {
        return transform(BasicVector4<T>(point, 1)).getVector3();
    }

    /**
     * \brief Returns the given 3-component direction transformed by this
     * matrix.

     * The given vector is treated as direction so it won't receive a
     * translation, just like a 4-component vector with its w-component set to
     * 0 would be transformed.
     */
    template<typename T>
    BasicVector3<T> transformDirection(const BasicVector3<T>& direction) const
    {
        return transform(BasicVector4<T>(direction, 0)).getVector3();
    }

    /// Return the given 4-component vector transformed by this matrix.
    template<typename T>
    BasicVector4<T> transform(const BasicVector4<T>& vector4) const;

    /// Return the result of this matrix post-multiplied by another matrix.
    Matrix4 getMultipliedBy(const Matrix4& other) const
    {
        return Matrix4(_transform * other.eigen());
    }

    /// Post-multiply this matrix by another matrix, in-place.
    void multiplyBy(const Matrix4& other)
    {
        *this = getMultipliedBy(other);
    }

    /// Returns this matrix pre-multiplied by the other
    Matrix4 getPremultipliedBy(const Matrix4& other) const
    {
        return other.getMultipliedBy(*this);
    }

    /// Pre-multiplies this matrix by other in-place.
    void premultiplyBy(const Matrix4& other)
    {
        *this = getPremultipliedBy(other);
    }

    /**
     * \brief Add a translation component to the transformation represented by
     * this matrix, modifying in-place.
     *
     * Equivalent to multiplyBy(Matrix4::getTranslation(tr)); note that this is
     * a post-multiplication so the translation will be applied before (and be
     * affected by) any existing rotation/scale in the current matrix.
     */
    void translateBy(const Vector3& tr)
    {
        _transform *= Eigen::Translation3d(tr.x(), tr.y(), tr.z());
    }

    /**
     * \brief Add a translation component to the transformation represented by
     * this matrix and return the result.
     *
     * Equivalent to getMultipliedBy(Matrix4::getTranslation(tr));
     */
    Matrix4 getTranslatedBy(const Vector3& tr) const
    {
        return Matrix4(_transform
                       * Eigen::Translation3d(tr.x(), tr.y(), tr.z()));
    }

    /**
     * \brief
     * Add a scale component to the transformation represented by this matrix.
     *
     * Equivalent to multiplyBy(Matrix4::getScale(scale));
     */
    void scaleBy(const Vector3& scale);

    /**
     * \brief
     * Add a pivoted scale transformation to this matrix.
     */
    void scaleBy(const Vector3& scale, const Vector3& pivot);

    /// Compare the affine part of this matrix with another for equality
    bool isAffineEqual(const Matrix4& other) const
    {
        return eigen().affine() == other.eigen().affine();
    }

    /**
     * Returns RIGHTHANDED if this is right-handed, else returns LEFTHANDED.
     */
    Handedness getHandedness() const;

    /// Return the 3-element translation component of this matrix
    Vector3 translation() const
    {
        return Vector3(_transform.matrix().col(3).head(3));
    }

    /// Set the translation component of this matrix
    void setTranslation(const Vector3& translation)
    {
        _transform.matrix().col(3).head(3) = translation.eigen();
    }

    /**
     * Concatenates this with the rotation transform produced
     * by euler angles (degrees) in the order (x, y, z).
     * The concatenated rotation occurs before self.
     */
    void rotateByEulerXYZDegrees(const Vector3& euler)
    {
        multiplyBy(getRotationForEulerXYZDegrees(euler));
    }

    /**
     * Calculates and returns a set of euler angles in radians that produce
     * the rotation component of this matrix when applied in the order (x, y, z).
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesXYZ() const;

    /**
     * Calculates and returns a set of euler angles in degrees that produce
     * the rotation component of this matrix when applied in the order (x, y, z).
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesXYZDegrees() const;

    /**
     * Calculates and returns the (x, y, z) scale values that produce the scale component of this matrix.
     * This matrix must be affine and orthogonal to produce a meaningful result.
     */
    Vector3 getScale() const;
};

// ===========================================================================
// Operators
// ===========================================================================

/// Multiply two matrices together
inline Matrix4 operator* (const Matrix4& m1, const Matrix4& m2)
{
    return m1.getMultipliedBy(m2);
}

/// Subtract two matrices
inline Matrix4 operator- (const Matrix4& l, const Matrix4& r)
{
    return Matrix4(
        Eigen::Projective3d(l.eigen().matrix() - r.eigen().matrix())
    );
}

/**
 * \brief Multiply a 4-component vector by this matrix.
 *
 * Equivalent to m.transform(v).
 */
template<typename T>
BasicVector4<T> operator* (const Matrix4& m, const BasicVector4<T>& v)
{
    return m.transform(v);
}

/**
 * \brief Multiply a 3-component vector by this matrix.
 *
 * The vector is upgraded to a 4-component vector with a W component of 1, i.e.
 * equivalent to m.transformPoint(v).
 */
template<typename T>
BasicVector3<T> operator* (const Matrix4& m, const BasicVector3<T>& v)
{
    return m.transformPoint(v);
}

// =========================================================================================
// Inlined member definitions
// =========================================================================================

// Construct a matrix with given column elements
inline Matrix4 Matrix4::byColumns(double xx, double xy, double xz, double xw,
                                  double yx, double yy, double yz, double yw,
                                  double zx, double zy, double zz, double zw,
                                  double tx, double ty, double tz, double tw)
{
    return Matrix4(xx, xy, xz, xw,
                   yx, yy, yz, yw,
                   zx, zy, zz, zw,
                   tx, ty, tz, tw);
}

// Construct a matrix with given row elements
inline Matrix4 Matrix4::byRows(double xx, double yx, double zx, double tx,
                               double xy, double yy, double zy, double ty,
                               double xz, double yz, double zz, double tz,
                               double xw, double yw, double zw, double tw)
{
    return Matrix4(xx, xy, xz, xw,
                   yx, yy, yz, yw,
                   zx, zy, zz, zw,
                   tx, ty, tz, tw);
}

/// Compare two matrices elementwise for equality
inline bool operator==(const Matrix4& l, const Matrix4& r)
{
    return l.eigen().matrix() == r.eigen().matrix();
}

/// Compare two matrices elementwise for inequality
inline bool operator!=(const Matrix4& l, const Matrix4& r)
{
    return !(l == r);
}

inline Matrix4::Handedness Matrix4::getHandedness() const
{
    return (xCol3().cross(yCol3()).dot(zCol3()) < 0.0f) ? LEFTHANDED : RIGHTHANDED;
}

template<typename T>
BasicVector4<T> Matrix4::transform(const BasicVector4<T>& vector4) const
{
    return _transform * vector4.eigen();
}

inline Vector3 Matrix4::getEulerAnglesXYZ() const
{
    double a = asin(-xz());
    double ca = cos(a);

    if (fabs(ca) > 0.005f) // Gimbal lock?
    {
        return Vector3(
            atan2(yz() / ca, zz() / ca),
            a,
            atan2(xy() / ca, xx() / ca)
        );
    }
    else // Gimbal lock has occurred
    {
        return Vector3(
            atan2(-zy(), yy()),
            a,
            0
        );
    }
}

inline Vector3 Matrix4::getEulerAnglesXYZDegrees() const
{
    Vector3 eulerRad = getEulerAnglesXYZ();
    return Vector3(radians_to_degrees(eulerRad.x()), radians_to_degrees(eulerRad.y()), radians_to_degrees(eulerRad.z()));
}

inline Vector3 Matrix4::getScale() const
{
    return Vector3(
        xCol3().getLength(),
        yCol3().getLength(),
        zCol3().getLength()
    );
}

inline void Matrix4::scaleBy(const Vector3& scale, const Vector3& pivot)
{
    translateBy(pivot);
    scaleBy(scale);
    translateBy(-pivot);
}

/// Debug stream insertion operator for Matrix4
inline std::ostream& operator<<(std::ostream& st, const Matrix4& m)
{
    st << "[" << m[0] << " " << m[4] << " " << m[8] << " " << m[12] << "; ";
    st << m[1] << " " << m[5] << " " << m[9] << " " << m[13] << "; ";
    st << m[2] << " " << m[6] << " " << m[10] << " " << m[14] << "; ";
    st << m[3] << " " << m[7] << " " << m[11] << " " << m[15] << "]";
    return st;
}

namespace math
{

inline Vector3f transformVector3f(const Matrix4& transform, const Vector3f& point)
{
    auto transformed = transform * Vector3(point.x(), point.y(), point.z());
    return Vector3f(static_cast<float>(transformed.x()), static_cast<float>(transformed.y()), static_cast<float>(transformed.z()));
}

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
