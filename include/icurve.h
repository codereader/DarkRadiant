#ifndef ICURVE_H_
#define ICURVE_H_

#include "scenelib.h"

class CurveInstance
{
public:
	/** greebo: Returns true if the curve has 0 control points.
	 */
	virtual bool hasEmptyCurve() = 0;
	
	/** greebo: Appends a control point at the end of the chain.
	 */
	virtual void appendControlPoints(unsigned int numPoints) = 0;
	
	/** greebo: As the name states, this removes the selected 
	 * 			control points from the curve.
	 */
	virtual void removeSelectedControlPoints() = 0;
	
	/** greebo: This inserts a control point BEFORE each
	 * 			selected control point of the curve.
	 * 			Naturally, this doesn't work if the first vertex
	 * 			is selected.
	 */
	virtual void insertControlPointsAtSelected() = 0;
};

inline CurveInstance* Instance_getCurveInstance(scene::Instance& instance) {
	return dynamic_cast<CurveInstance*>(&instance);
}

#endif /*ICURVE_H_*/
