#pragma once

#include "itexturetoolmodel.h"
#include "../BasicSelectable.h"

namespace textool
{

class NodeBase :
    public INode
{
private:
    selection::BasicSelectable _selectable;

public:
    virtual void setSelected(bool select) override
    {
        _selectable.setSelected(select);
    }

    virtual bool isSelected() const override
    {
        return _selectable.isSelected();
    }
};

}
