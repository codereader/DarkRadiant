#include "FacePlane.h"

// Constructor and copy constructor
FacePlane::FacePlane()
{}

FacePlane::FacePlane(const FacePlane& other) :
	m_plane(other.m_plane)
{}

void FacePlane::reverse()
{
	m_plane.reverse();
}

void FacePlane::transform(const Matrix4& matrix, bool mirror)
{
	// Prepare the plane to be transformed (negate the distance)
	m_plane.dist() = -m_plane.dist();
	
	// Transform the plane
	m_plane = matrix.transform(m_plane);
	
	// Re-negate the distance
	m_plane.dist() = -m_plane.dist();
	
	// Now normalise the plane, otherwise the next transformation will screw up
	m_plane.normalise(); 
}

void FacePlane::offset(float offset)
{
	m_plane.d += offset;
}

void FacePlane::setPlane(const Plane3& plane)
{
	m_plane = plane;
}

const Plane3& FacePlane::getPlane() const
{
	return m_plane;
}

void FacePlane::copy(const FacePlane& other)
{
	m_plane = other.m_plane;
}

void FacePlane::copy(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	m_plane = Plane3(p2, p1, p0);
}
