#pragma once

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
class alignas(16) Matrix3
{
    // Underlying Eigen transform object (which can in turn be reduced to a 3x3
    // Matrix)
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
};

// Private constructor
Matrix3::Matrix3(double xx_, double xy_, double xz_,
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

/// Compare two matrices elementwise for equality
inline bool operator==(const Matrix3& l, const Matrix3& r)
{
    return l.eigen().matrix() == r.eigen().matrix();
}
