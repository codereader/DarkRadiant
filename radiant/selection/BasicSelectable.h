#pragma once

#include "iselectable.h"

namespace selection
{

/**
 * A simple implementation of the Selectable interface.
 * Behaves just as one would expect, keeping track of
 * the selected state by means of a boolean.
 */
class BasicSelectable : 
	public Selectable
{
private:
	bool _selected;

public:
	BasicSelectable() :
		_selected(false)
	{}

	void setSelected(bool select = true)
	{
		_selected = select;
	}

	bool isSelected() const
	{
		return _selected;
	}

	void invertSelected()
	{
		_selected = !_selected;
	}
};

} // namespace
