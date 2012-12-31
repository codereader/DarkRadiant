#pragma once

/// \file
/// \brief Matrix data types and related operations.

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/pi.h"

class Quaternion;

typedef unsigned char ClipResult;

const ClipResult c_CLIP_PASS = 0x00; // 000000
const ClipResult c_CLIP_LT_X = 0x01; // 000001
const ClipResult c_CLIP_GT_X = 0x02; // 000010
const ClipResult c_CLIP_LT_Y = 0x04; // 000100
const ClipResult c_CLIP_GT_Y = 0x08; // 001000
const ClipResult c_CLIP_LT_Z = 0x10; // 010000
const ClipResult c_CLIP_GT_Z = 0x20; // 100000
const ClipResult c_CLIP_FAIL = 0x3F; // 111111

/**
 * A 4x4 matrix stored in double-precision floating-point.
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
    // Elements of the 4x4 matrix. These appear to be treated COLUMNWISE, i.e.
    // elements [0] through [3] are the first column, [4] through [7] are the
    // second column, etc.
    double _m[16];

private:

    // Initialising constructor, elements are passed in column-wise order
    Matrix4(double xx_, double xy_, double xz_, double xw_,
            double yx_, double yy_, double yz_, double yw_,
            double zx_, double zy_, double zz_, double zw_,
            double tx_, double ty_, double tz_, double tw_);
public:

    /// Construct a matrix with uninitialised values.
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
     * greebo: Attempts to parse the rotation from the given string, which is
     * a whitespace-separated chain of nine floating point values, as used 
     * in entity spawnargs.
     * 
     * Example: "0 1 0 -1 0 0 0 0 1"
     *
     * Returns: the parsed (translation-free) matrix. In case of parser errors
     * the identity matrix is returned.
     */
    static Matrix4 getRotation(const std::string& rotationString);

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
    static Matrix4 getRotation(const Vector3& axis, const double angle);

    /**
     * Constructs a pure-rotation matrix from the given quaternion.
     */
    static Matrix4 getRotation(const Quaternion& quaternion);

    /**
     * Constructs a pure-rotation matrix from the given quaternion, quantised.
     */
    static Matrix4 getRotationQuantised(const Quaternion& quaternion);

    /**
     * Constructs a pure-rotation matrix about the x axis from sin and cosine of an angle.
     */
    static Matrix4 getRotationAboutXForSinCos(double s, double c);

    /**
     * Constructs a pure-rotation matrix about the x axis from an angle in radians
     */
    static Matrix4 getRotationAboutX(double angle);

    /**
     * Constructs a pure-rotation matrix about the x axis from an angle in degrees.
     */
    static Matrix4 getRotationAboutXDegrees(double angle);

    /**
     * Constructs a pure-rotation matrix about the y axis from sin and cosine of an angle.
     */
    static Matrix4 getRotationAboutYForSinCos(double s, double c);

    /**
     * Constructs a pure-rotation matrix about the y axis from an angle in radians
     */
    static Matrix4 getRotationAboutY(double angle);

    /**
     * Constructs a pure-rotation matrix about the y axis from an angle in degrees.
     */
    static Matrix4 getRotationAboutYDegrees(double angle);

    /**
     * Constructs a pure-rotation matrix about the z axis from sin and cosine of an angle.
     */
    static Matrix4 getRotationAboutZForSinCos(double s, double c);

    /**
     * Constructs a pure-rotation matrix about the z axis from an angle in radians
     */
    static Matrix4 getRotationAboutZ(double angle);

    /**
     * Constructs a pure-rotation matrix about the z axis from an angle in degrees.
     */
    static Matrix4 getRotationAboutZDegrees(double angle);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (x, y, z).
     */
    static Matrix4 getRotationForEulerXYZ(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (x, y, z).
     */
    static Matrix4 getRotationForEulerXYZDegrees(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (y, z, x).
     */
    static Matrix4 getRotationForEulerYZX(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (y, z, x).
     */
    static Matrix4 getRotationForEulerYZXDegrees(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (x, z, y).
     */
    static Matrix4 getRotationForEulerXZY(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (x, z, y).
     */
    static Matrix4 getRotationForEulerXZYDegrees(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (y, x, z).
     */
    static Matrix4 getRotationForEulerYXZ(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (y, x, z).
     */
    static Matrix4 getRotationForEulerYXZDegrees(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (z, x, y).
     */
    static Matrix4 getRotationForEulerZXY(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (z, x, y).
     */
    static Matrix4 getRotationForEulerZXYDegrees(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (radians) in the order (z, y, x).
     */
    static Matrix4 getRotationForEulerZYX(const Vector3& euler);

    /**
     * Constructs a pure-rotation matrix from a set of euler angles (degrees) in the order (z, y, x).
     */
    static Matrix4 getRotationForEulerZYXDegrees(const Vector3& euler);

    /**
     * \brief
     * Get a matrix representing the given scale in 3D space.
     *
     * \param scale
     * Vector3 representing the scale.
     */
    static Matrix4 getScale(const Vector3& scale);

    /**
     * Returns a perspective projection matrix for the six given frustum planes. The result is the projection
     * matrix as constructed by openGL when  calling the glFrustum() function.
     */
    static Matrix4 getProjectionForFrustum(double left, double right, double bottom, double top, double nearval, double farval);

    /**
     * \brief
     * Construct a matrix containing the given elements.
     *
     * The elements are specified column-wise, starting with the left-most
     * column.
     */
    static Matrix4 byColumns(double xx, double xy, double xz, double xw,
                             double yx, double yy, double yz, double yw,
                             double zx, double zy, double zz, double zw,
                             double tx, double ty, double tz, double tw);

    /**
     * \brief
     * Construct a matrix containing the given elements.
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
    double& xx()             { return _m[0]; }
    const double& xx() const { return _m[0]; }
    double& xy()             { return _m[1]; }
    const double& xy() const { return _m[1]; }
    double& xz()             { return _m[2]; }
    const double& xz() const { return _m[2]; }
    double& xw()             { return _m[3]; }
    const double& xw() const { return _m[3]; }
    double& yx()             { return _m[4]; }
    const double& yx() const { return _m[4]; }
    double& yy()             { return _m[5]; }
    const double& yy() const { return _m[5]; }
    double& yz()             { return _m[6]; }
    const double& yz() const { return _m[6]; }
    double& yw()             { return _m[7]; }
    const double& yw() const { return _m[7]; }
    double& zx()             { return _m[8]; }
    const double& zx() const { return _m[8]; }
    double& zy()             { return _m[9]; }
    const double& zy() const { return _m[9]; }
    double& zz()             { return _m[10]; }
    const double& zz() const { return _m[10]; }
    double& zw()             { return _m[11]; }
    const double& zw() const { return _m[11]; }
    double& tx()             { return _m[12]; }
    const double& tx() const { return _m[12]; }
    double& ty()             { return _m[13]; }
    const double& ty() const { return _m[13]; }
    double& tz()             { return _m[14]; }
    const double& tz() const { return _m[14]; }
    double& tw()             { return _m[15]; }
    const double& tw() const { return _m[15]; }
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

    /**
     * Cast to double* for use with GL functions that accept a double
     * array, also provides operator[].
     */
    operator double* ()
    {
        return _m;
    }

    /**
     * Cast to const double* to provide operator[] for const objects.
     */
    operator const double* () const
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
     * Affine invert this matrix in-place.
     */
    void invert();

    /**
     * \brief
     * Return the full inverse of this matrix.
     */
    Matrix4 getFullInverse() const;

    /**
     * Invert this matrix in-place.
     */
    void invertFull();

    /** 
     * Returns the given 3-component point transformed by this matrix.
     */
    template<typename Element>
    BasicVector3<Element> transformPoint(const BasicVector3<Element>& point) const;

    /** 
     * Returns the given 3-component direction transformed by this matrix.
     * The given vector is treated as direction so it won't receive a translation, just like
     * a 4-component vector with its w-component set to 0 would be transformed.
     */
    template<typename Element>
    BasicVector3<Element> transformDirection(const BasicVector3<Element>& direction) const;

    /**
     * \brief
     * Use this matrix to transform the provided vector and return a new vector
     * containing the result.
     *
     * \param vector4
     * The 4-element vector to transform.
     */
    template<typename Element>
    BasicVector4<Element> transform(const BasicVector4<Element>& vector4) const;

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
     * Add a translation component to the transformation represented by this
     * matrix.
     *
     * Equivalent to getMultipliedBy(Matrix4::getTranslation(translation));
     */
    Matrix4 getTranslatedBy(const Vector3& translation) const;

    /**
     * Returns this matrix concatenated with the rotation transform produced by the given quat.
     * The concatenated rotation occurs before the transformation of this matrix.
     * 
     * Equivalent to getMultipliedBy(getRotation(rotation));
     */
    Matrix4 getRotatedBy(const Quaternion& rotation) const;

    /**
     * Concatenates this matrix with the rotation transform produced by the given quat.
     * The concatenated rotation occurs before the transformation of this matrix.
     */
    void rotateBy(const Quaternion& rotation);

    /**
     * Concatenates this matrix with the pivoted rotation transform produced by the given quat.
     * The concatenated rotation occurs before the transformation of this matrix.
     */
    void rotateBy(const Quaternion& rotation, const Vector3& pivot);

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
    bool isEqual(const Matrix4& other, double epsilon) const;

    /**
     * Returns true if this and the given matrix are exactly element-wise equal.
     * This and the other matrix must be affine.
     */
    bool isAffineEqual(const Matrix4& other) const;

    /**
     * Returns RIGHTHANDED if this is right-handed, else returns LEFTHANDED.
     */
    Handedness getHandedness() const;

    /**
     * Returns true if this matrix is affine.
     */
    bool isAffine() const;

    /**
     * Returns this matrix post-multiplied by the other.
     * This and the other matrix must be affine.
     */
    Matrix4 getAffineMultipliedBy(const Matrix4& other) const;

    /**
     * Post-multiplies this matrix by the other in-place.
     * This and the other matrix must be affine.
     */
    void affineMultiplyBy(const Matrix4& other);

    /**
     * Returns this matrix pre-multiplied by the other.
     * This matrix and the other must be affine.
     */
    Matrix4 getAffinePremultipliedBy(const Matrix4& other) const;

    /**
     * Pre-multiplies this matrix by the other in-place.
     * This and the other matrix must be affine.
     */
    void affinePremultiplyBy(const Matrix4& other);

    /**
     * Returns the determinant of this 4x4 matrix
     */
    double getDeterminant() const;

    /// Return the 3-element translation component of this matrix
    const Vector3& translation() const;

    /**
     * Concatenates this with the rotation transform produced 
     * by euler angles (degrees) in the order (x, y, z).
     * The concatenated rotation occurs before self.
     */
    void rotateByEulerXYZDegrees(const Vector3& euler);

    /**
     * Concatenates this with the pivoted rotation transform produced 
     * by euler angles (degrees) in the order (x, y, z).
     * The concatenated rotation occurs before self.
     */
    void rotateByEulerXYZDegrees(const Vector3& euler, const Vector3& pivot);

    /**
     * Returns this matrix concatenated with the rotation transform produced by the given
     * euler angles (degrees) in the order (y, x, z). The concatenated rotation occurs before this matrix.
     */
    Matrix4 getRotatedByEulerYXZDegrees(const Vector3& euler) const;

    /**
     * Concatenates this with the rotation transform produced 
     * by euler angles (degrees) in the order (y, x, z).
     * The concatenated rotation occurs before self.
     */
    void rotateByEulerYXZDegrees(const Vector3& euler);

    /**
     * Returns this matrix concatenated with the rotation transform produced by the given
     * euler angles (degrees) in the order (z, x, y). The concatenated rotation occurs before this matrix.
     */
    Matrix4 getRotatedByEulerZXYDegrees(const Vector3& euler) const;

    /**
     * Concatenates this with the rotation transform produced 
     * by euler angles (degrees) in the order (z, x, y).
     * The concatenated rotation occurs before self.
     */
    void rotateByEulerZXYDegrees(const Vector3& euler);

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
     * Calculates and returns a set of euler angles in radians that produce 
     * the rotation component of this matrix when applied in the order (y, x, z). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesYXZ() const;

    /**
     * Calculates and returns a set of euler angles in degrees that produce
     * the rotation component of this matrix when applied in the order (y, x, z). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesYXZDegrees() const;

    /**
     * Calculates and returns a set of euler angles in radians that produce 
     * the rotation component of this matrix when applied in the order (z, x, y). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesZXY() const;

    /**
     * Calculates and returns a set of euler angles in degrees that produce
     * the rotation component of this matrix when applied in the order (z, x, y). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesZXYDegrees() const;

    /**
     * Calculates and returns a set of euler angles in radians that produce 
     * the rotation component of this matrix when applied in the order (z, y, x). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesZYX() const;

    /**
     * Calculates and returns a set of euler angles in degrees that produce
     * the rotation component of this matrix when applied in the order (z, y, x). 
     * This matrix must be affine and orthonormal (unscaled) to produce a meaningful result.
     */
    Vector3 getEulerAnglesZYXDegrees() const;

    /**
     * Calculates and returns the (x, y, z) scale values that produce the scale component of this matrix.
     * This matrix must be affine and orthogonal to produce a meaningful result.
     */
    Vector3 getScale() const;

    /**
     * Transforms and clips the line formed by p0, p1 by this canonical matrix.
     * Stores the resulting line in clipped. 
     * 
     * @returns: the number of points in the resulting line.
     */
    std::size_t clipLine(const Vector3& p0, const Vector3& p1, Vector4 clipped[2]) const;

    /** 
     * Clips point by this canonical matrix and stores the result in clipped.
     * Returns a bitmask indicating which clip-planes the point was outside.
     */
    ClipResult clipPoint(const Vector3& point, Vector4& clipped) const;

    /** 
     * Transforms and clips the triangle formed by p0, p1, p2 by this canonical matrix.
     * Stores the resulting polygon in clipped.
     * Returns the number of points in the resulting polygon.
     */
    std::size_t clipTriangle(const Vector3& p0, const Vector3& p1, const Vector3& p2, Vector4 clipped[9]) const;
};

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

inline Matrix4 Matrix4::getRotationAboutX(double angle)
{
    return getRotationAboutXForSinCos(sin(angle), cos(angle));
}

inline Matrix4 Matrix4::getRotationAboutXDegrees(double angle)
{
    return getRotationAboutX(degrees_to_radians(angle));
}

inline Matrix4 Matrix4::getRotationAboutY(double angle)
{
    return getRotationAboutYForSinCos(sin(angle), cos(angle));
}

inline Matrix4 Matrix4::getRotationAboutYDegrees(double angle)
{
    return getRotationAboutY(degrees_to_radians(angle));
}

inline Matrix4 Matrix4::getRotationAboutZ(double angle)
{
    return getRotationAboutZForSinCos(sin(angle), cos(angle));
}

inline Matrix4 Matrix4::getRotationAboutZDegrees(double angle)
{
    return getRotationAboutZ(degrees_to_radians(angle));
}

inline Matrix4 Matrix4::getProjectionForFrustum(double left, double right, double bottom, double top, double nearval, double farval)
{
    return Matrix4::byColumns(
        (2*nearval) / (right-left),
        0,
        0,
        0,
        0,
        (2*nearval) / (top-bottom),
        0,
        0,
        (right+left) / (right-left),
        (top+bottom) / (top-bottom),
        -(farval+nearval) / (farval-nearval),
        -1,
        0,
        0,
        -(2*farval*nearval) / (farval-nearval),
        0
    );
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

inline bool Matrix4::isEqual(const Matrix4& other, double epsilon) const
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

inline bool Matrix4::isAffine() const
{
    return xw() == 0 && yw() == 0 && zw() == 0 && tw() == 1;
}

inline Matrix4 Matrix4::getAffineMultipliedBy(const Matrix4& other) const
{
    return Matrix4::byColumns(
        other.xx() * xx() + other.xy() * yx() + other.xz() * zx(),
        other.xx() * xy() + other.xy() * yy() + other.xz() * zy(),
        other.xx() * xz() + other.xy() * yz() + other.xz() * zz(),
        0,
        other.yx() * xx() + other.yy() * yx() + other.yz() * zx(),
        other.yx() * xy() + other.yy() * yy() + other.yz() * zy(),
        other.yx() * xz() + other.yy() * yz() + other.yz() * zz(),
        0,
        other.zx() * xx() + other.zy() * yx() + other.zz()* zx(),
        other.zx() * xy() + other.zy() * yy() + other.zz()* zy(),
        other.zx() * xz() + other.zy() * yz() + other.zz()* zz(),
        0,
        other.tx()* xx() + other.ty()* yx() + other.tz()* zx() + tx(),
        other.tx()* xy() + other.ty()* yy() + other.tz()* zy() + ty(),
        other.tx()* xz() + other.ty()* yz() + other.tz()* zz()+ tz(),
        1
    );
}

inline void Matrix4::affineMultiplyBy(const Matrix4& other)
{
    *this = getAffineMultipliedBy(other);
}

inline Matrix4 Matrix4::getAffinePremultipliedBy(const Matrix4& other) const
{
    return other.getAffineMultipliedBy(*this);
}

inline void Matrix4::affinePremultiplyBy(const Matrix4& other)
{
    *this = getAffinePremultipliedBy(other);
}

template<typename Element>
BasicVector3<Element> Matrix4::transformPoint(const BasicVector3<Element>& point) const
{
    return BasicVector3<Element>(
        static_cast<Element>(xx() * point[0] + yx() * point[1] + zx() * point[2] + tx()),
        static_cast<Element>(xy() * point[0] + yy() * point[1] + zy() * point[2] + ty()),
        static_cast<Element>(xz() * point[0] + yz() * point[1] + zz() * point[2] + tz())
    );
}

template<typename Element>
BasicVector3<Element> Matrix4::transformDirection(const BasicVector3<Element>& direction) const
{
    return BasicVector3<Element>(
        static_cast<Element>(xx() * direction[0] + yx() * direction[1] + zx() * direction[2]),
        static_cast<Element>(xy() * direction[0] + yy() * direction[1] + zy() * direction[2]),
        static_cast<Element>(xz() * direction[0] + yz() * direction[1] + zz() * direction[2])
    );
}

template<typename Element>
BasicVector4<Element> Matrix4::transform(const BasicVector4<Element>& vector4) const
{
    return BasicVector4<Element>(
        static_cast<Element>(_m[0] * vector4[0] + _m[4] * vector4[1] + _m[8]  * vector4[2] + _m[12] * vector4[3]),
        static_cast<Element>(_m[1] * vector4[0] + _m[5] * vector4[1] + _m[9]  * vector4[2] + _m[13] * vector4[3]),
        static_cast<Element>(_m[2] * vector4[0] + _m[6] * vector4[1] + _m[10] * vector4[2] + _m[14] * vector4[3]),
        static_cast<Element>(_m[3] * vector4[0] + _m[7] * vector4[1] + _m[11] * vector4[2] + _m[15] * vector4[3])
    );
}

inline void Matrix4::invert()
{
    *this = getInverse();
}

inline void Matrix4::invertFull()
{
    *this = getFullInverse();
}

inline double Matrix4::getDeterminant() const
{
    // greebo: This is following Laplace's formula by expanding it along the first column
    // It needs a couple of 2x2 minors (which are re-used two times each) and four 3x3 minors
    
    // The expanded formula is like this: det A = a11*M11 - a21*M21 + a31*M31 + a41*M41
    // where aij is a matrix element, and Mij is the minor leaving out the i-th row and j-th column

    // Six 2x2 minors, each is used two times
    double minor1 = zz() * tw() - zw() * tz();
    double minor2 = zy() * tw() - zw() * ty();
    double minor3 = zx() * tw() - zw() * tx();
    double minor4 = zy() * tz() - zz() * ty();
    double minor5 = zx() * tz() - zz() * tx();
    double minor6 = zx() * ty() - zy() * tx();

    // Four 3x3 minors
    double minor11 = yy() * minor1 - yz() * minor2 + yw() * minor4;
    double minor21 = yx() * minor1 - yz() * minor3 + yw() * minor5;
    double minor31 = yx() * minor2 - yy() * minor3 + yw() * minor6;
    double minor41 = yx() * minor4 - yy() * minor5 + yz() * minor6;
    
    // Assemble and return final determinant
    return xx() * minor11 - xy() * minor21 + xz() * minor31 - xw() * minor41;
}

inline const Vector3& Matrix4::translation() const
{
    return t().getVector3();
}

inline Matrix4 Matrix4::getTranslatedBy(const Vector3& translation) const
{
    return getMultipliedBy(Matrix4::getTranslation(translation));
}

inline Matrix4 Matrix4::getRotatedBy(const Quaternion& rotation) const
{
    return getMultipliedBy(getRotation(rotation));
}

inline void Matrix4::rotateBy(const Quaternion& rotation)
{
    *this = getRotatedBy(rotation);
}

inline void Matrix4::rotateBy(const Quaternion& rotation, const Vector3& pivot)
{
    translateBy(pivot);
    rotateBy(rotation);
    translateBy(-pivot);
}

inline void Matrix4::rotateByEulerXYZDegrees(const Vector3& euler)
{
    multiplyBy(getRotationForEulerXYZDegrees(euler));
}

inline void Matrix4::rotateByEulerXYZDegrees(const Vector3& euler, const Vector3& pivot)
{
    translateBy(pivot);
    rotateByEulerXYZDegrees(euler);
    translateBy(-pivot);
}

inline Matrix4 Matrix4::getRotatedByEulerYXZDegrees(const Vector3& euler) const
{
    return getMultipliedBy(getRotationForEulerYXZDegrees(euler));
}

/// \brief Concatenates \p self with the rotation transform produced by \p euler angles (degrees) in the order (y, x, z).
/// The concatenated rotation occurs before \p self.
inline void Matrix4::rotateByEulerYXZDegrees(const Vector3& euler)
{
    *this = getRotatedByEulerYXZDegrees(euler);
}

inline Matrix4 Matrix4::getRotatedByEulerZXYDegrees(const Vector3& euler) const
{
    return getMultipliedBy(getRotationForEulerZXYDegrees(euler));
}

inline void Matrix4::rotateByEulerZXYDegrees(const Vector3& euler)
{
    *this = getRotatedByEulerZXYDegrees(euler);
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

inline Vector3 Matrix4::getEulerAnglesYXZ() const
{
    double a = asin(yz());
    double ca = cos(a);

    if (fabs(ca) > 0.005f) // Gimbal lock?
    {
        return Vector3(
            a,
            atan2(-xz() / ca, zz() / ca),
            atan2(-yx() / ca, yy() / ca)
        );
    }
    else // Gimbal lock has occurred
    {
        return Vector3(
            a,
            atan2(zx(), xx()),
            0
        );
    }
}

inline Vector3 Matrix4::getEulerAnglesYXZDegrees() const
{
    Vector3 eulerRad = getEulerAnglesYXZ();
    return Vector3(radians_to_degrees(eulerRad.x()), radians_to_degrees(eulerRad.y()), radians_to_degrees(eulerRad.z()));
}

inline Vector3 Matrix4::getEulerAnglesZXY() const
{
    double a = asin(-zy());
    double ca = cos(a);

    if (fabs(ca) > 0.005f) // Gimbal lock?
    {
        return Vector3(
            a,
            atan2(zx() / ca, zz() / ca),
            atan2(xy() / ca, yy()/ ca)
        );
    }
    else // Gimbal lock has occurred
    {
        return Vector3(
            a,
            0,
            atan2(-yx(), xx())
        );
    }
}

inline Vector3 Matrix4::getEulerAnglesZXYDegrees() const
{
    Vector3 eulerRad = getEulerAnglesZXY();
    return Vector3(radians_to_degrees(eulerRad.x()), radians_to_degrees(eulerRad.y()), radians_to_degrees(eulerRad.z()));
}

inline Vector3 Matrix4::getEulerAnglesZYX() const
{
    double a = asin(zx());
    double ca = cos(a);

    if (fabs(ca) > 0.005f) // Gimbal lock?
    {
        return Vector3(
            atan2(-zy() / ca, zz()/ ca),
            a,
            atan2(-yx() / ca, xx() / ca)
        );
    }
    else // Gimbal lock has occurred
    {
        return Vector3(
            0,
            a,
            atan2(xy(), yy())
        );
    }
}

inline Vector3 Matrix4::getEulerAnglesZYXDegrees() const
{
    Vector3 eulerRad = getEulerAnglesZYX();
    return Vector3(radians_to_degrees(eulerRad.x()), radians_to_degrees(eulerRad.y()), radians_to_degrees(eulerRad.z()));
}

inline Vector3 Matrix4::getScale() const
{
    return Vector3(
        x().getVector3().getLength(),
        y().getVector3().getLength(),
        z().getVector3().getLength()
    );
}

inline void Matrix4::scaleBy(const Vector3& scale, const Vector3& pivot)
{
    translateBy(pivot);
    scaleBy(scale);
    translateBy(-pivot);
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
