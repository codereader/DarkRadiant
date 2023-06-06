#pragma once

#include "iselectable.h"

namespace selection
{

/**
 * A simple implementation of the Selectable interface.
 * Behaves just as one would expect, keeping track of
 * the selected state by means of a boolean.
 */
class BasicSelectable: public ISelectable
{
    bool _selected = false;

public:

    void setSelected(bool select = true) override
    {
        _selected = select;
    }

    bool isSelected() const override
    {
        return _selected;
    }

    void invertSelected()
    {
        _selected = !_selected;
    }
};

/// Convenience function to call setSelected() on a number of items
template<typename... Item> void setSelected(bool select, Item&&... item)
{
    (item.setSelected(select), ...);
}

/// Convenience function to return true if any one of a number of items is selected
template<typename... Item> bool isAnySelected(const Item&... item)
{
    return (item.isSelected() || ...);
}

} // namespace
