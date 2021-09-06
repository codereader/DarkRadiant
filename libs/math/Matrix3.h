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

private:
    /// Construct a matrix with uninitialised values.
    Matrix3() { }

    /// Construct from Eigen transform
    explicit Matrix3(const Eigen::Projective2d& t) : 
        _transform(t)
    {}

    /// Get the underlying Eigen transform
    Transform& eigen() { return _transform; }

    /// Get the underlying const Eigen transform
    const Transform& eigen() const { return _transform; }

    /// Obtain the identity matrix.
    static Matrix3 getIdentity()
    {
        return Matrix3(Eigen::Projective2d::Identity());
    }
};
