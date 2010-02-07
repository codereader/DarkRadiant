#include "FacePlane.h"

inline Plane3 Plane3_applyTranslation(const Plane3& plane, const Vector3& translation) {
	Plane3 tmp = Plane3(plane.normal(), -plane.dist()).getTranslated(translation);
	return Plane3(tmp.a, tmp.b, tmp.c, -tmp.d);
}

// Constructor and copy constructor
FacePlane::FacePlane()
{}

FacePlane::FacePlane(const FacePlane& other)
{
	m_plane = other.m_plane;
	updateTranslated();
}

void FacePlane::reverse()
{
	m_planeCached = -m_plane;
	updateSource();
}

void FacePlane::transform(const Matrix4& matrix, bool mirror)
{
	// Prepare the plane to be transformed (negate the distance)
	m_planeCached.dist() = -m_planeCached.dist();
	
	// Transform the plane
	m_planeCached = matrix.transform(m_planeCached);
	
	// Re-negate the distance
	m_planeCached.dist() = -m_planeCached.dist();
	
	// Now normalise the plane, otherwise the next transformation will screw up
	m_planeCached = m_planeCached.getNormalised(); 
	
	updateSource();
}

void FacePlane::offset(float offset)
{
	m_planeCached.d += offset;
	updateSource();
}

void FacePlane::updateTranslated()
{
	m_planeCached = m_plane;
}

void FacePlane::updateSource()
{
	m_plane = m_planeCached;
}

const Plane3& FacePlane::plane3() const
{
	return m_planeCached;
}

void FacePlane::setDoom3Plane(const Plane3& plane)
{
	m_plane = plane;
	updateTranslated();
}

const Plane3& FacePlane::getDoom3Plane() const
{
	return m_plane;
}

void FacePlane::copy(const FacePlane& other)
{
	m_planeCached = other.m_plane;
	updateSource();
}

void FacePlane::copy(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	m_planeCached = Plane3(p2, p1, p0);
	updateSource();
}
