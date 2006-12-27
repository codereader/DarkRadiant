#ifndef CLIPPOINT_H_
#define CLIPPOINT_H_

#include "qerplugin.h"

#include "math/Vector3.h"

//!\todo Rewrite.
class ClipPoint
{
public:
	Vector3 m_ptClip;      // the 3d point
	bool m_bSet;

	// Constructor
	ClipPoint();
	
	void Reset();
	
	bool Set();
	void Set(bool b);
	
	operator Vector3&();

	double intersect(const Vector3& point, VIEWTYPE viewtype, float scale);

	/*! Draw clip/path point with rasterized number label */
	void Draw(int num, float scale);
	
	/*! Draw clip/path point with rasterized string label */
	void Draw(const std::string& label, float scale);
}; // class ClipPoint

#endif /*CLIPPOINT_H_*/
