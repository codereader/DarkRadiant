#ifndef CURVECATMULLROM_H_
#define CURVECATMULLROM_H_

#include "Curve.h"

namespace entity {

class CurveCatmullRom :
	public Curve
{
public:
	CurveCatmullRom(const Callback& callback);
	
	// Subdivides the segments between the control points
	virtual void tesselate();

	// Extends the algorithm of the base class by a few commands
	virtual void appendControlPoints(unsigned int numPoints);

private:
	// Clears the control points, weights and knots
	virtual void clearCurve();
};

} // namespace entity

#endif /*CURVECATMULLROM_H_*/
