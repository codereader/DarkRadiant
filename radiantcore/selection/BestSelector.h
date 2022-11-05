#pragma once

#include <list>
#include "iselectiontest.h"

namespace selection
{

/**
 * Selector implementation picking the selectables by their intersection quality.
 * Only selectables with valid intersections that are better or equal than the
 * currently best value are considered and added to a list of "best" selectables. 
 * Selectables with closer (=better) intersection values cause all previously added
*  selectables to be discarded.
 * Selectables which have roughly equal intersection values as the currently best one
 * will get added to the end of the internal list.
 */
class BestSelector : 
	public Selector 
{
private:
	SelectionIntersection _curIntersection;
	ISelectable* _selectable;
	SelectionIntersection _bestIntersection;
	std::list<ISelectable*> _bestSelectables;

public:
	BestSelector() :
        _selectable(nullptr)
	{}

	void pushSelectable(ISelectable& selectable) override
	{
		_curIntersection = SelectionIntersection();
		_selectable = &selectable;
	}

	void popSelectable() override
	{
		if (_curIntersection.equalEpsilon(_bestIntersection, 0.25f, 0.001f))
		{
			_bestSelectables.push_back(_selectable);
			_bestIntersection = _curIntersection;
		}
		else if (_curIntersection < _bestIntersection)
		{
			_bestSelectables.clear();
			_bestSelectables.push_back(_selectable);
			_bestIntersection = _curIntersection;
		}

		_curIntersection = SelectionIntersection();
	}

	void addIntersection(const SelectionIntersection& intersection) override
	{
		_curIntersection.assignIfCloser(intersection);
	}

	const std::list<ISelectable*>& getBestSelectables() const
	{
		return _bestSelectables;
	}

    bool empty() const override
    {
        return _bestSelectables.empty();
    }

    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override
	{
	    for (auto selectable : _bestSelectables)
	    {
            functor(selectable);
	    }
	}
};

}
