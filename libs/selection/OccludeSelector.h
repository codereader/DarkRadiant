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

	void pushSelectable(ISelectable& selectable) override {}
	void popSelectable() override {}

	void addIntersection(const SelectionIntersection& intersection) override
	{
		if (intersection.isCloserThan(_bestIntersection))
		{
			_bestIntersection = intersection;
			_occluded = true;
		}
	}

    bool empty() const override
    {
        return true;
    }

    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override
    {}
};

} // namespace
