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
			facePlane.updateTranslated();
		}
	}; // class SavedState

	// Constructor and copy constructor
	FacePlane();
	FacePlane(const FacePlane& other);

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
