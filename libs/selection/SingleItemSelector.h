#pragma once

#include "iselectiontest.h"

namespace selection
{

/**
 * Selector implementation accepting a single selectable item (or none at all).
 * In case mulitple selectables are considered, the one with the best intersection
 * value is chosen.
 *
 * The selectable in question has to be currently selected to be accepted.
 */
class SingleItemSelector : 
	public Selector
{
private:
	bool _selected;
	SelectionIntersection _intersection;
	ISelectable* _selectable;

public:
	SingleItemSelector() : 
		_selected(false), 
		_selectable(nullptr)
	{}

	void pushSelectable(ISelectable& selectable) override
	{
		_intersection = SelectionIntersection();
		_selectable = &selectable;
	}

	void popSelectable() override
	{
		if (_intersection.isValid())
		{
			_selected = true;
		}

		_intersection = SelectionIntersection();
	}

	void addIntersection(const SelectionIntersection& intersection) override
	{
		if (_selectable != nullptr && _selectable->isSelected())
		{
			_intersection.assignIfCloser(intersection);
		}
	}

	// Returns true if this selector accepted a single selectable with a valid intersection
	bool hasValidSelectable() const
	{
		return _selected;
	}

    ISelectable* getSelectable() const
    {
        return _selectable;
    }

    bool empty() const override
    {
        return !hasValidSelectable();
    }

    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override
	{
        if (!empty())
        {
            functor(_selectable);
        }
	}
};

}
