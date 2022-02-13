#ifndef CURVECATMULLROM_H_
#define CURVECATMULLROM_H_

#include "Curve.h"

namespace entity {

	namespace {
		const std::string curve_CatmullRomSpline = "curve_CatmullRomSpline";
	}

class CurveCatmullRom :
	public Curve
{
public:
	CurveCatmullRom(const IEntityNode& entity, const Callback& callback);

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
};

} // namespace entity

#endif /*CURVECATMULLROM_H_*/
