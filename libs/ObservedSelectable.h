#pragma once

#include "iselectable.h"
#include "iselection.h"

namespace selection
{

/**
 * \brief
 * Implementation of the Selectable interface which invokes a user-specified
 * callback function when the selection state is changed.
 */
class ObservedSelectable : 
	public ISelectable
{
    // Callback to invoke on selection changed
    SelectionChangedSlot _onchanged;

    // Current selection state
    bool _selected;

public:

    /**
     * \brief
     * Construct an ObservedSelectable with the given callback function.
     */
    ObservedSelectable(const SelectionChangedSlot& onchanged) : 
		_onchanged(onchanged), 
		_selected(false)
    { }

    /**
     * \brief
     * Copy constructor.
     */
    ObservedSelectable(const ObservedSelectable& other) : 
		ISelectable(other), 
		_onchanged(other._onchanged), 
		_selected(false)
    {
        setSelected(other.isSelected());
    }

	ObservedSelectable& operator=(const ObservedSelectable& other)
	{
		setSelected(other.isSelected());
		return *this;
	}

	virtual ~ObservedSelectable()
	{
		setSelected(false);
	}

    /**
     * \brief
     * Set the selection state.
     */
    virtual void setSelected(bool select) override
    {
        // Change state and invoke callback only if the new state is different
        // from the current state
        if (select ^ _selected)
        {
            _selected = select;

			if (_onchanged)
			{
				_onchanged(*this);
			}
        }
    }

	virtual bool isSelected() const override
	{
		return _selected;
	}
};

} // namespace
