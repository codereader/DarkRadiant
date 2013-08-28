#pragma once

#include "icommandsystem.h"

namespace selection
{

namespace algorithm
{
	void createCurveNURBS(const cmd::ArgumentList& args);
	void createCurveCatmullRom(const cmd::ArgumentList& args);

	/** greebo: Appends a control point to the selected curves.
	 * 			This works for Doom3Group entities only that have
	 * 			a non-zero curve attached.
	 */
	void appendCurveControlPoint(const cmd::ArgumentList& args);

	/** greebo: Removes the selected control points from the selected curves.
	 * 			This works in ComponentSelection mode and for Doom3Group
	 * 			entities only that have	a non-zero curve attached.
	 */
	void removeCurveControlPoints(const cmd::ArgumentList& args);

	/** greebo: Inserts a new control points BEFORE the selected control points
	 * 			of the selected curves.	This works in ComponentSelection mode
	 * 			and for Doom3Group entities only that have a non-zero curve attached.
	 */
	void insertCurveControlPoints(const cmd::ArgumentList& args);

	/** greebo: Converts the curves of the selected entities from
	 * 			CatmullRom to NURBS and vice versa.
	 */
	void convertCurveTypes(const cmd::ArgumentList& args);

} // namespace algorithm

} // namespace selection
