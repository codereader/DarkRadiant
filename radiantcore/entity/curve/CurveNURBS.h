#ifndef CURVENURBS_H_
#define CURVENURBS_H_

#include "Curve.h"

namespace entity {

	namespace {
		const std::string curve_Nurbs = "curve_Nurbs";
	}

class CurveNURBS :
	public Curve
{
	NURBSWeights _weights;
	Knots _knots;
public:
	CurveNURBS(const IEntityNode& entity, const Callback& callback);

	// Subdivides the segments between the control points
	virtual void tesselate();

	// Extends the algorithm of the base class by a few commands
	virtual void appendControlPoints(unsigned int numPoints);

	// Writes the control point coords to the entity
	virtual void saveToEntity(Entity& target);

	// Removes the given list of control points
	virtual void removeControlPoints(IteratorList iterators);

	// Inserts control points before the specified list of iterators.
	virtual void insertControlPointsAt(IteratorList iterators);

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
