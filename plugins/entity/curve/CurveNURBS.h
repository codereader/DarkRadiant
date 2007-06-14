#ifndef CURVENURBS_H_
#define CURVENURBS_H_

#include "Curve.h"

namespace entity {

class CurveNURBS :
	public Curve
{
	NURBSWeights _weights;
	Knots _knots;
public:
	CurveNURBS(const Callback& callback);
	
	// Subdivides the segments between the control points
	virtual void tesselate();

	// Extends the algorithm of the base class by a few commands
	virtual void appendControlPoints(unsigned int numPoints);

private:
	// Clears the control points, weights and knots
	virtual void clearCurve();
	
	// Overrides the derived parseCurve method, to do the weighting
	virtual bool parseCurve(const std::string& value);
	
	// Helper method, performs the weighting
	void doWeighting();
};

} // namespace entity

#endif /*CURVENURBS_H_*/
