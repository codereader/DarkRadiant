#include "FacePlane.h"

inline Plane3 Plane3_applyTranslation(const Plane3& plane, const Vector3& translation) {
	Plane3 tmp = Plane3(plane.normal(), -plane.dist()).getTranslated(translation);
	return Plane3(tmp.normal(), -tmp.dist());
}

// Constructor and copy constructor
FacePlane::FacePlane() : m_funcStaticOrigin(0, 0, 0) {}

FacePlane::FacePlane(const FacePlane& other) : m_funcStaticOrigin(0, 0, 0) {
	if (!isDoom3Plane()) {
		planepts_assign(m_planepts, other.m_planepts);
		MakePlane();
	}
	else {
		m_plane = other.m_plane;
		updateTranslated();
	}
}

void FacePlane::MakePlane() {
	if (!isDoom3Plane()) {
		m_planeCached = Plane3(m_planepts);
	}
}

void FacePlane::reverse() {
	if (!isDoom3Plane()) {
		vector3_swap(m_planepts[0], m_planepts[2]);
		MakePlane();
	}
	else {
		m_planeCached = -m_plane;
		updateSource();
	}
}

void FacePlane::transform(const Matrix4& matrix, bool mirror) {
	if (!isDoom3Plane()) {
		matrix4_transform_point(matrix, m_planepts[0]);
		matrix4_transform_point(matrix, m_planepts[1]);
		matrix4_transform_point(matrix, m_planepts[2]);

		if (mirror) {
			reverse();
		}

		MakePlane();
	}
	else {
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
}

void FacePlane::offset(float offset) {
	if (!isDoom3Plane()) {
		Vector3 move(m_planeCached.normal()*(-offset));

		m_planepts[0] -= move;
		m_planepts[1] -= move;
		m_planepts[2] -= move;

		MakePlane();
	}
	else {
		m_planeCached.d += offset;
		updateSource();
	}
}

void FacePlane::updateTranslated() {
	m_planeCached = Plane3_applyTranslation(m_plane, m_funcStaticOrigin);
}

void FacePlane::updateSource() {
	m_plane = Plane3_applyTranslation(m_planeCached, -m_funcStaticOrigin);
}

PlanePoints& FacePlane::planePoints() {
	return m_planepts;
}
const PlanePoints& FacePlane::planePoints() const {
	return m_planepts;
}

const Plane3& FacePlane::plane3() const {
	return m_planeCached;
}
void FacePlane::setDoom3Plane(const Plane3& plane) {
	m_plane = plane;
	updateTranslated();
}
const Plane3& FacePlane::getDoom3Plane() const {
	return m_plane;
}

void FacePlane::copy(const FacePlane& other) {
	if (!isDoom3Plane()) {
		planepts_assign(m_planepts, other.m_planepts);
		MakePlane();
	}
	else {
		m_planeCached = other.m_plane;
		updateSource();
	}
}

void FacePlane::copy(const Vector3& p0, const Vector3& p1, const Vector3& p2) {
	if (!isDoom3Plane()) {
		m_planepts[0] = p0;
		m_planepts[1] = p1;
		m_planepts[2] = p2;
		MakePlane();
	}
	else {
		m_planeCached = Plane3(p2, p1, p0);
		updateSource();
	}
}
