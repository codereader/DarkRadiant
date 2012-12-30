#pragma once

#include "math/Plane3.h"
#include "math/Vector3.h"
#include "PlanePoints.h"

class Matrix4;

/// Wrapper for a Plane3 providing certain face-related functions
class FacePlane
{
    Plane3 m_plane;

public:
    class SavedState
    {
    public:
        Plane3 m_plane;

        SavedState(const FacePlane& facePlane) :
            m_plane(facePlane.m_plane)
        {}

        void exportState(FacePlane& facePlane) const
        {
            facePlane.m_plane = m_plane;
        }
    }; // class SavedState

    /// Initialise internal plane from the given three points
    void initialiseFromPoints(const Vector3& p0,
                              const Vector3& p1,
                              const Vector3& p2);

    /// Set the internal plane from the given Plane3
    void setPlane(const Plane3& plane);

    /// Return the internal Plane3
    const Plane3& getPlane() const;

    void reverse();

    /// Translate the plane by a given vector
    void translate(const Vector3& translation);

    /// Transform the plane by an arbitrary matrix
    void transform(const Matrix4& matrix);

    void offset(float offset);

}; // class FacePlane
