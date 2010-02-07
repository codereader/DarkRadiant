#include "FacePlane.h"

inline Plane3 Plane3_applyTranslation(const Plane3& plane, const Vector3& translation) {
	Plane3 tmp = Plane3(plane.normal(), -plane.dist()).getTranslated(translation);
	return Plane3(tmp.a, tmp.b, tmp.c, -tmp.d);
}

// Constructor and copy constructor
FacePlane::FacePlane() : 
	m_funcStaticOrigin(0, 0, 0)
{}

FacePlane::FacePlane(const FacePlane& other) : 
	m_funcStaticOrigin(0, 0, 0)
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
	/*Plane3 tmp = Plane3(m_plane.normal(), -m_plane.dist()).getTranslated(m_funcStaticOrigin);
	m_planeCached = Plane3(tmp.a, tmp.b, tmp.c, -tmp.d);*/

	m_planeCached = Plane3_applyTranslation(m_plane, m_funcStaticOrigin);
}

void FacePlane::updateSource()
{
	m_plane = Plane3_applyTranslation(m_planeCached, -m_funcStaticOrigin);
}

PlanePoints& FacePlane::planePoints()
{
	return m_planepts;
}

const PlanePoints& FacePlane::planePoints() const
{
	return m_planepts;
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
