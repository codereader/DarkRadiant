#pragma once

#include "iselection.h"

namespace selection
{

// Keeps track of the entity selection in the scene,
// notifies the client code on selection changes.
class EntitySelection final :
    public SelectionSystem::Observer
{
private:


public:
    EntitySelection()
    {
        GlobalSelectionSystem().addObserver(this);
    }

    ~EntitySelection()
    {
        GlobalSelectionSystem().removeObserver(this);
    }

    void selectionChanged(const scene::INodePtr& node, bool isComponent) override
    {
        // TODO
    }
};

}
