#ifndef CURVES_ALGORITHM_H_
#define CURVES_ALGORITHM_H_

namespace selection {
	namespace algorithm {
	
	/** greebo: Appends a control point to the selected curves.
	 * 			This works for Doom3Group entities only that have
	 * 			a non-zero curve attached.
	 */
	void appendCurveControlPoint();
	
	/** greebo: Removes the selected control points from the selected curves.
	 * 			This works in ComponentSelection mode and for Doom3Group 
	 * 			entities only that have	a non-zero curve attached.
	 */
	void removeCurveControlPoints();
	
	/** greebo: Inserts a new control points BEFORE the selected control points 
	 * 			of the selected curves.	This works in ComponentSelection mode 
	 * 			and for Doom3Group entities only that have a non-zero curve attached.
	 */
	void insertCurveControlPoints();
	
	/** greebo: Converts the curves of the selected entities from 
	 * 			CatmullRom to NURBS and vice versa.
	 */
	void convertCurveTypes();
	
	} // namespace algorithm
} // namespace selection

#endif /*CURVES_ALGORITHM_H_*/
