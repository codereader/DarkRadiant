#pragma once

#include <map>
#include "imousetool.h"
#include "imousetoolmanager.h"
#include "xmlutil/Node.h"
#include "MouseToolGroup.h"

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
    typedef std::map<IMouseToolGroup::Type, MouseToolGroupPtr> GroupMap;
    GroupMap _mouseToolGroups;

    unsigned int _activeModifierState;

public:
    MouseToolManager();

    // RegisterableModule implementation
    const std::string& getName() const;
    const StringSet& getDependencies() const;
    void initialiseModule(const ApplicationContext& ctx);
    void shutdownModule();
    void onRadiantStartup();

    // Get the group defined by the given enum. This always succeeds, if the group
    // is not existing yet, a new one will be created internally.
    MouseToolGroup& getGroup(IMouseToolGroup::Type group);

    // Iterate over each group using the given visitor function
    void foreachGroup(const std::function<void(IMouseToolGroup&)>& functor);

    MouseToolStack getMouseToolsForEvent(IMouseToolGroup::Type group, unsigned int mouseState);

    void updateStatusbar(unsigned int newState);

    void resetBindingsToDefault();

private:
    void loadToolMappings();
	void loadGroupMapping(MouseToolGroup::Type type, const xml::NodeList& userMappings, const xml::NodeList& defaultMappings);

    void saveToolMappings();
};

} // namespace
