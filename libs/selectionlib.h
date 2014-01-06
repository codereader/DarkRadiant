#pragma once

#include "iselection.h"
#include "iselectable.h"
#include "iselectiontest.h"
#include <stdlib.h>
#include <list>
#include <boost/bind.hpp>
#include "scene/Node.h"
#include "math/AABB.h"

/** greebo: A structure containing information about the current
 * Selection. An instance of this is maintained by the
 * RadiantSelectionSystem, and a const reference can be
 * retrieved via the according getSelectionInfo() method.
 */
class SelectionInfo {
public:
	int totalCount; 	// number of selected items
	int patchCount; 	// number of selected patches
	int brushCount; 	// -- " -- brushes
	int entityCount; 	// -- " -- entities
	int componentCount;	// -- " -- components (faces, edges, vertices)

	SelectionInfo() :
		totalCount(0),
		patchCount(0),
		brushCount(0),
		entityCount(0),
		componentCount(0)
	{}

	// Zeroes all the counters
	void clear() {
		totalCount = 0;
		patchCount = 0;
		brushCount = 0;
		entityCount = 0;
		componentCount = 0;
	}
};

namespace selection
{

/**
 * The selection "WorkZone" defines the bounds of the most
 * recent selection. On each selection, the workzone is
 * recalculated, nothing happens on deselection.
 */
struct WorkZone
{
	// The corner points defining the selection workzone
	Vector3 min;
	Vector3 max;

	// The bounds of the selection workzone (equivalent to min/max)
	AABB bounds;

	WorkZone() :
		min(-64,-64,-64),
		max(64,64,64),
		bounds(AABB::createFromMinMax(min, max))
	{}
};

} // namespace selection

class OccludeSelector : public Selector
{
	SelectionIntersection& _bestIntersection;
	bool& _occluded;
public:
	OccludeSelector(SelectionIntersection& bestIntersection, bool& occluded) :
		_bestIntersection(bestIntersection),
		_occluded(occluded)
	{
		_occluded = false;
	}

	void pushSelectable(Selectable& selectable) {}
	void popSelectable() {}

	void addIntersection(const SelectionIntersection& intersection) {
		if (SelectionIntersection_closer(intersection, _bestIntersection)) {
			_bestIntersection = intersection;
			_occluded = true;
		}
	}
}; // class OccludeSelector
