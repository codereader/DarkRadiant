#pragma once

#include "iselectiontest.h"

namespace selection
{

class OccludeSelector : 
	public Selector
{
private:
	SelectionIntersection& _bestIntersection;
	bool& _occluded;

public:
	OccludeSelector(SelectionIntersection& bestIntersection, bool& occluded) :
		_bestIntersection(bestIntersection),
		_occluded(occluded)
	{
		_occluded = false;
	}

	void pushSelectable(ISelectable& selectable) {}
	void popSelectable() {}

	void addIntersection(const SelectionIntersection& intersection)
	{
		if (intersection.isCloserThan(_bestIntersection))
		{
			_bestIntersection = intersection;
			_occluded = true;
		}
	}
};

} // namespace
