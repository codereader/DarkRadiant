#ifndef CLIPPOINT_H_
#define CLIPPOINT_H_

#include "iclipper.h"

#include "math/Vector3.h"

//!\todo Rewrite.
class ClipPoint
{
public:
	Vector3 _coords;      // the 3d point
	bool m_bSet;

	// Constructor, sets the clip point coords to <0,0,0> and m_bSet to <false>
	ClipPoint();
	
	void reset();
	
	// Returns true if the clip point is set
	bool isSet() const;

	// Sets the clip point set status to <b>
	void Set(bool b);
	
	operator Vector3&();

	double intersect(const Vector3& point, EViewType viewtype, float scale);
	
	void testSelect(const Vector3& point, EViewType viewtype, float scale, double& bestDistance, ClipPoint*& bestClip);

	/*! Draw clip/path point with rasterized number label */
	void Draw(int num, float scale);
	
	/*! Draw clip/path point with rasterized string label */
	void Draw(const std::string& label, float scale);
}; // class ClipPoint

#endif /*CLIPPOINT_H_*/
