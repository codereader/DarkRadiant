#ifndef FACEPLANE_H_
#define FACEPLANE_H_

#include "math/Plane3.h"
#include "math/Vector3.h"
#include "math/matrix.h"
#include "PlanePoints.h"

class FacePlane {
	
	PlanePoints m_planepts;
	Plane3 m_planeCached;
	Plane3 m_plane;

public:
	Vector3 m_funcStaticOrigin;

	static bool isDoom3Plane() {
		return true;
	}

	class SavedState {
		public:
			PlanePoints m_planepts;
			Plane3 m_plane;

		SavedState(const FacePlane& facePlane) {
			if (facePlane.isDoom3Plane()) {
				m_plane = facePlane.m_plane;
			}
			else {
				planepts_assign(m_planepts, facePlane.planePoints());
			}
		}

		void exportState(FacePlane& facePlane) const {
			if (facePlane.isDoom3Plane()) {
				facePlane.m_plane = m_plane;
				facePlane.updateTranslated();
			}
			else {
				planepts_assign(facePlane.planePoints(), m_planepts);
				facePlane.MakePlane();
			}
		}
	}; // class SavedState

	// Constructor and copy constructor
	FacePlane();
	FacePlane(const FacePlane& other);

	void MakePlane();
	void reverse();

	void transform(const Matrix4& matrix, bool mirror);
	
	void offset(float offset);

	void updateTranslated();
	
	void updateSource();

	PlanePoints& planePoints();
	const PlanePoints& planePoints() const;
	
	const Plane3& plane3() const;
	void setDoom3Plane(const Plane3& plane);
	const Plane3& getDoom3Plane() const;

	void copy(const FacePlane& other);
	void copy(const Vector3& p0, const Vector3& p1, const Vector3& p2);

}; // class FacePlane

#endif /*FACEPLANE_H_*/
