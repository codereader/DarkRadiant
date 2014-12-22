#pragma once

#include <map>
#include "imousetool.h"
#include "imousetoolmanager.h"
#include "xmlutil/Node.h"

namespace ui
{

class MouseToolGroup;

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
    void onRadiantStartup();

    // Get the group defined by the given enum. This always succeeds, if the group
    // is not existing yet, a new one will be created internally.
    IMouseToolGroup& getGroup(IMouseToolGroup::Type group);

    // Iterate over each group using the given visitor function
    void foreachGroup(const std::function<void(IMouseToolGroup&)>& functor);

    MouseToolStack getMouseToolsForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev);

private:
    void loadToolMappings();
    void loadGroupMapping(MouseToolGroup& group, const xml::Node& mappingNode);

    void saveToolMappings();
};

} // namespace
