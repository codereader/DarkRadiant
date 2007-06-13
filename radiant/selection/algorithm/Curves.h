#ifndef CURVES_ALGORITHM_H_
#define CURVES_ALGORITHM_H_

namespace selection {
	namespace algorithm {
	
	/** greebo: Appends a control point to the selected curves.
	 * 			This works for Doom3Group entities only that have
	 * 			a non-zero curve attached.
	 */
	void appendCurveControlPoint();
	
	} // namespace algorithm
} // namespace selection

#endif /*CURVES_ALGORITHM_H_*/
