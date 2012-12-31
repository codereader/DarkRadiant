#include "FacePlane.h"

#include "math/Matrix4.h"

void FacePlane::reverse()
{
    m_plane.reverse();
}

void FacePlane::translate(const Vector3& translation)
{
    // Translate without touching the normal vector, preserving the required
    // distance negation
    m_plane.dist() = -m_plane.dist();
    m_plane.translate(translation);
    m_plane.dist() = -m_plane.dist();
}

void FacePlane::transform(const Matrix4& matrix)
{
    // Prepare the plane to be transformed (negate the distance)
    m_plane.dist() = -m_plane.dist();

    // Transform the plane
    m_plane.transform(matrix);

    // Re-negate the distance
    m_plane.dist() = -m_plane.dist();

    // Now normalise the plane, otherwise the next transformation will screw up
    m_plane.normalise();
}

void FacePlane::offset(float offset)
{
    m_plane.dist() += offset;
}

void FacePlane::setPlane(const Plane3& plane)
{
    m_plane = plane;
}

const Plane3& FacePlane::getPlane() const
{
    return m_plane;
}

void FacePlane::initialiseFromPoints(const Vector3& p0,
                                     const Vector3& p1,
                                     const Vector3& p2)
{
    m_plane = Plane3(p2, p1, p0);
}
