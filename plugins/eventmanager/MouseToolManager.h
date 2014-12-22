#pragma once

#include <map>
#include "imousetoolmanager.h"

namespace ui
{

/**
* Implementation of the IMouseToolManager interface.
* This is used by the GlobalXYWnd and GlobalCamera instances.
*/
class MouseToolManager :
    public IMouseToolManager
{
protected:
    typedef std::map<IMouseToolGroup::Type, IMouseToolGroupPtr> GroupMap;
    GroupMap _mouseToolGroups;

public:
    // RegisterableModule implementation
    const std::string& getName() const;
    const StringSet& getDependencies() const;
    void initialiseModule(const ApplicationContext& ctx);
    void shutdownModule();

    // Get the group defined by the given enum. This always succeeds, if the group
    // is not existing yet, a new one will be created internally.
    IMouseToolGroup& getGroup(IMouseToolGroup::Type group);

    // Iterate over each group using the given visitor function
    void foreachGroup(const std::function<void(IMouseToolGroup&)>& functor);

    MouseToolStack getMouseToolStackForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev);
};

} // namespace
