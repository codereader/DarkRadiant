#ifndef ICURVE_H_
#define ICURVE_H_

#include "inode.h"

class CurveNode
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
	
	/** greebo: Converts the type of the curve from CatmullRom
	 * 			to NURBS and vice versa.
	 */
	virtual void convertCurveType() = 0;
};
typedef boost::shared_ptr<CurveNode> CurveNodePtr;

inline CurveNodePtr Node_getCurve(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<CurveNode>(node);
}

#endif /*ICURVE_H_*/
