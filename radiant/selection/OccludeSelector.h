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

	void pushSelectable(Selectable& selectable) {}
	void popSelectable() {}

	void addIntersection(const SelectionIntersection& intersection)
	{
		if (SelectionIntersection_closer(intersection, _bestIntersection))
		{
			_bestIntersection = intersection;
			_occluded = true;
		}
	}
};

} // namespace
